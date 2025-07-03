// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include <boost/process.hpp>
#include <vincentlaucsb-csv-parser/csv.hpp>
#include "../PresentMonAPI2Loader/Loader.h"
#include "../PresentMonAPI2/Internal.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../CommonUtilities/IntervalWaiter.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include <string>
#include <iostream>
#include <format>
#include <fstream>
#include <filesystem>
#include <array>
#include <ranges>
#include <cmath>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process;
namespace fs = std::filesystem;
namespace rn = std::ranges;
namespace vi = rn::views;
using namespace std::literals;
using namespace pmon;

namespace PacedPollingTests
{
	std::vector<double> ExtractColumn(const std::vector<std::vector<double>>& mat, std::size_t i)
	{
		return mat | vi::transform([i](auto const& row) { return row[i]; })
			| rn::to<std::vector>();
	}

	class BlobReader
	{
		struct LookupInfo_
		{
			uint64_t offset;
			PM_DATA_TYPE type;
		};
	public:
		BlobReader(std::span<const PM_QUERY_ELEMENT> qels, std::shared_ptr<pmapi::intro::Root> pIntro)
		{
			for (auto& q : qels) {
				qInfo_.push_back({ q.dataOffset, pIntro->FindMetric(q.metric).GetDataTypeInfo().GetPolledType() });
			}
		}
		void Target(const pmapi::BlobContainer& blobs, uint32_t iBlob = 0)
		{
			pFirstByteTarget_ = blobs[iBlob];
		}
		template<typename T>
		T At(size_t iElement)
		{
			const auto off = qInfo_[iElement].offset;
			switch (qInfo_[iElement].type) {
			case PM_DATA_TYPE_BOOL:   return (T)reinterpret_cast<const bool&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_DOUBLE: return    reinterpret_cast<const double&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_ENUM:   return (T)reinterpret_cast<const int&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_INT32:  return (T)reinterpret_cast<const int32_t&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_STRING: return (T)-1;
			case PM_DATA_TYPE_UINT32: return (T)reinterpret_cast<const uint32_t&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_UINT64: return (T)reinterpret_cast<const uint64_t&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_VOID:   return (T)-1;
			}
			return (T)-1;
		}
	private:
		const uint8_t* pFirstByteTarget_ = nullptr;
		std::vector<LookupInfo_> qInfo_;
	};

	struct Mismatch
	{
		size_t sampleIndex;
		double val0;
		double val1;
	};

	struct MetricCompareResult
	{
		std::vector<Mismatch> mismatches;
		double meanSquareError;
	};

	std::pair<double, double> CalculateDynamicRange(
		const std::vector<double>& run0,
		const std::vector<double>& run1)
	{
		const auto [minIt0, maxIt0] = rn::minmax_element(run0);
		const auto [minIt1, maxIt1] = rn::minmax_element(run1);
		double lo = std::min(*minIt0, *minIt1);
		double hi = std::max(*maxIt0, *maxIt1);
		return { lo, hi };
	}

	MetricCompareResult CompareMetricRuns(
		const std::vector<double>& run0,
		const std::vector<double>& run1,
		double toleranceFactor)
	{
		// 1) compute dynamic range & tolerance
		auto [lo, hi] = CalculateDynamicRange(run0, run1);
		double tolerance = (hi - lo) * toleranceFactor;

		// 2) loop over corresponding samples and compare for individual mismatch and mse
		MetricCompareResult result;
		double sumSq = 0.0;
		for (auto&& [i, v0, v1] : vi::zip(vi::iota(0ull), run0, run1)) {
			const auto diff = v0 - v1;
			sumSq += diff * diff;
			if (std::abs(diff) > tolerance) {
				result.mismatches.push_back({ i, v0, v1 });
			}
		}

		// 3) finish computing MSE
		result.meanSquareError = sumSq / static_cast<double>(run0.size());
		return result;
	}

	TEST_CLASS(PacedPollingTests)
	{
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
		}
		TEST_METHOD(PollDynamic)
		{
			const uint32_t targetPid = 4136;
			const auto recordingStart = 6.7s;
			const auto recordingStop = 20s;

			const auto pipeName = R"(\\.\pipe\pm-poll-test-act)"s;
			const auto etlName = "hea-win.etl"s;
			const auto goldCsvName = "polled_gold.csv"s;

			// setup gold standard dataset
			bool hasGold = false;


			pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
			pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
			
			for (int x = 0; x < 1; x++) {
				if (x > 0) {
					std::this_thread::sleep_for(50ms);
				}

				bp::child svc{ "PresentMonService.exe"s,
					"--control-pipe"s, pipeName,
					"--nsm-prefix"s, "pmon_poll_test_nsm"s,
					"--intro-nsm"s, "pm_poll_test_intro"s,
					"--etl-test-file"s, etlName,
					"--pace-playback" };

				Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
					L"Timeout waiting for service control pipe");

				pmapi::Session api{ pipeName };

				auto pIntro = api.GetIntrospectionRoot();

				std::vector<PM_QUERY_ELEMENT> qels;
				for (const auto& m : pIntro->GetMetrics()) {
					// there is no reliable way of distinguishing CPU telemetry metrics from PresentData-based metrics via introspection
					// adding CPU device type is an idea, however that would require changing device id of the cpu metrics from 0 to
					// whatever id is assigned to cpu (probably an upper range like 1024+) and this might break existing code that just
					// hardcodes device id for the CPU metrics; for the time being use a hardcoded blacklist here
					if (rn::contains(std::array{
						PM_METRIC_CPU_UTILIZATION,
						PM_METRIC_CPU_POWER_LIMIT,
						PM_METRIC_CPU_POWER,
						PM_METRIC_CPU_TEMPERATURE,
						PM_METRIC_CPU_FREQUENCY,
						PM_METRIC_CPU_CORE_UTILITY,
						}, m.GetId())) {
						continue;
					}
					if (m.GetType() != PM_METRIC_TYPE_DYNAMIC && m.GetType() != PM_METRIC_TYPE_DYNAMIC_FRAME) {
						continue;
					}
					auto dmi = m.GetDeviceMetricInfo();
					if (dmi.size() != 1) {
						continue;
					}
					if (!dmi.front().IsAvailable() || dmi.front().GetDevice().GetId() != 0) {
						continue;
					}
					if (m.GetDataTypeInfo().GetPolledType() == PM_DATA_TYPE_STRING) {
						continue;
					}
					for (const auto& s : m.GetStatInfo()) {
						qels.push_back(PM_QUERY_ELEMENT{ m.GetId(), s.GetStat()});
					}
				}

				auto query = api.RegisterDyanamicQuery(qels, 1000., 64.);
				auto blobs = query.MakeBlobContainer(1);

				std::ofstream csvStream{ std::format("polled_{}.csv", x) };
				auto csvWriter = csv::make_csv_writer(csvStream);
				std::vector<std::string> headerColumns{ "poll-time"s };
				for (auto& qel : qels) {
					headerColumns.push_back(std::format("{}({})",
						pIntro->FindMetric(qel.metric).Introspect().GetSymbol(),
						pIntro->FindEnum(PM_ENUM_STAT).FindKey((int)qel.stat).GetShortName()
					));
				}
				csvWriter << headerColumns;

				auto tracker = api.TrackProcess(targetPid);

				using Clock = std::chrono::high_resolution_clock;
				const auto startTime = Clock::now();
				util::IntervalWaiter waiter{ 0.1, 0.001 };

				std::vector<std::vector<double>> rows;
				std::vector<double> cells;
				BlobReader br{ qels, pIntro };
				br.Target(blobs);
				for (auto now = Clock::now(), start = Clock::now();
					now - start <= recordingStop; now = Clock::now()) {
					if (now - start >= recordingStart) {
						cells.reserve(qels.size() + 1);
						query.Poll(tracker, blobs);
						cells.push_back(std::chrono::duration<double>(now - start).count());
						for (size_t i = 0; i < qels.size(); i++) {
							cells.push_back(br.At<double>(i));
						}
						csvWriter << cells;
						rows.push_back(std::move(cells));
					}
					waiter.Wait();
				}

				// load gold csv to vector of rows
				csv::CSVReader gold{ goldCsvName };
				std::vector<std::vector<double>> goldRows;
				for (auto& row : gold) {
					std::vector<double> rowData;
					rowData.reserve(row.size());
					for (auto& field : row) {
						rowData.push_back(field.get<double>());
					}
					goldRows.push_back(std::move(rowData));
				}

				// compare all columns of run to gold
				std::vector<MetricCompareResult> results;
				for (size_t i = 0; i < goldRows[0].size(); i++) {
					results.push_back(CompareMetricRuns(
						ExtractColumn(rows, i),
						ExtractColumn(goldRows, i),
						0.01
					));
				}

				// output results to csv
				std::ofstream resStream{ std::format("polled_results.csv", x) };
				auto resWriter = csv::make_csv_writer(resStream);
				resWriter << std::array{ "metric"s, "n-miss"s, "mse"s };
				const auto cols = gold.get_col_names();
				for (auto&& [i, res] : vi::enumerate(results)) {
					auto mse = std::isinf(res.meanSquareError) ? -1.0 : res.meanSquareError;
					resWriter << std::make_tuple(cols[i], res.mismatches.size(), mse);
				}
			}


			// compare csv to gold
			// attempt 1 quantization step phase shift matching (+/-)
			// perform majority fallback 3x w/ shift matching for each

			// if it doesn't exist, fail (instruct to examine and gild)
		}
	};
}
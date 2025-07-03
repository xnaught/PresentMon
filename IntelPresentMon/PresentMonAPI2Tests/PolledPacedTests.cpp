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

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process;
namespace fs = std::filesystem;
namespace rn = std::ranges;
namespace vi = rn::views;
using namespace std::literals;
using namespace pmon;

namespace PacedPollingTests
{
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
			
			for (int x = 0; x < 5; x++) {
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

				pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
				pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
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

				std::vector<double> cells;
				cells.reserve(qels.size() + 1);
				for (auto now = Clock::now(), start = Clock::now();
					now - start <= recordingStop; now = Clock::now()) {
					if (now - start >= recordingStart) {
						query.Poll(tracker, blobs);
						cells.push_back(std::chrono::duration<double>(now - start).count());
						for (auto& qel : qels) {
							cells.push_back(reinterpret_cast<const double&>(blobs[0][qel.dataOffset]));
						}
						csvWriter << cells;
						cells.clear();
					}
					waiter.Wait();
				}
			}


			// compare csv to gold
			// attempt 1 quantization step phase shift matching (+/-)
			// perform majority fallback 3x w/ shift matching for each

			// if it doesn't exist, fail (instruct to examine and gild)
		}
	};
}
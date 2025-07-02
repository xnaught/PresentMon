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
using namespace std::literals;
using namespace pmon;

namespace PacedPollingTests
{
	TEST_CLASS(PacedPollingTests)
	{
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
		}
		TEST_METHOD(PollDynamic)
		{
			const uint32_t targetPid = 4136;
			const auto recordingStart = 0s;
			const auto recordingStop = 20s;

			const auto pipeName = R"(\\.\pipe\pm-poll-test-act)"s;
			const auto etlName = "hea-win.etl"s;
			const auto goldCsvName = L"golds\\gold.csv"s;

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
					for (const auto& s : m.GetStatInfo()) {
						qels.push_back(PM_QUERY_ELEMENT{ m.GetId(), s.GetStat() });
					}
				}

				auto query = api.RegisterDyanamicQuery(qels, 1000., 64.);
				auto blobs = query.MakeBlobContainer(1);

				const auto dispFpsOffs = rn::find(qels, PM_METRIC_DISPLAYED_FPS, &PM_QUERY_ELEMENT::metric)->dataOffset;
				const auto presFpsOffs = rn::find(qels, PM_METRIC_PRESENTED_FPS, &PM_QUERY_ELEMENT::metric)->dataOffset;

				std::ofstream csvStream{ std::format("polled_{}.csv", x) };
				auto csvWriter = csv::make_csv_writer(csvStream);
				csvWriter << std::array{ "poll-time"s, "disp-fps"s, "pres-fps"s };

				auto tracker = api.TrackProcess(targetPid);

				using Clock = std::chrono::high_resolution_clock;
				const auto startTime = Clock::now();
				util::IntervalWaiter waiter{ 0.1, 0.001 };

				std::vector<double> procTimes;

				for (auto now = Clock::now(), start = Clock::now();
					(now - start) <= recordingStop; now = Clock::now()) {
					query.Poll(tracker, blobs);
					procTimes.push_back(std::chrono::duration<double>(Clock::now() - now).count());
					csvWriter << std::make_tuple(
						std::chrono::duration<double>(now - start).count(),
						reinterpret_cast<const double&>(blobs[0][dispFpsOffs]),
						reinterpret_cast<const double&>(blobs[0][presFpsOffs])
					);
					waiter.Wait();
				}

				for (auto& t : procTimes) {
					Logger::WriteMessage(std::format("{}\n", t).c_str());
				}
			}


			// compare csv to gold
			// attempt 1 quantization step phase shift matching (+/-)
			// perform majority fallback 3x w/ shift matching for each

			// if it doesn't exist, fail (instruct to examine and gild)
		}
	};
}
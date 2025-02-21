// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include <fstream>
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "BoostProcess.h"
#include "CsvHelper.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include <string>
#include <iostream>
#include <windows.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EtlTests
{
	std::string TranslatePresentMode(PM_PRESENT_MODE present_mode) {
		switch (present_mode) {
		case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP:
			return "Hardware: Legacy Flip";
		case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER:
			return "Hardware: Legacy Copy to front buffer";
		case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP:
			return "Hardware: Independent Flip";
		case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_FLIP:
			return "Composed: Flip";
		case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_COMPOSED_INDEPENDENT_FLIP:
			return "Hardware Composed: Independent Flip";
		case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_COPY_WITH_GPU_GDI:
			return "Composed: Copy with GPU GDI";
		case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_COPY_WITH_CPU_GDI:
			return "Composed: Copy with CPU GDI";
		default:
			return("Other");
		}
	}

	std::string TranslateGraphicsRuntime(PM_GRAPHICS_RUNTIME graphicsRuntime) {
		switch (graphicsRuntime) {
		case PM_GRAPHICS_RUNTIME_UNKNOWN:
			return "UNKNOWN";
		case PM_GRAPHICS_RUNTIME_DXGI:
			return "DXGI";
		case PM_GRAPHICS_RUNTIME_D3D9:
			return "D3D9";
		default:
			return "UNKNOWN";
		}
	}

	std::optional<std::ofstream> CreateCsvFile(std::string& output_dir, std::string& processName)
	{
		// Setup csv file
		time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		tm local_time;
		localtime_s(&local_time, &now);
		std::ofstream csvFile;
		std::string csvFileName = output_dir + processName;
		try {
			csvFile.open(csvFileName);
			csvFile << "Application,ProcessID,SwapChainAddress,Runtime,"
				"SyncInterval,PresentFlags,AllowsTearing,PresentMode,"
				"CPUStartQPC,CPUBusy,CPUWait,"
				"GPULatency,GPUBusy,GPUWait,VideoBusy,DisplayLatency,"
				"DisplayedTime,AllInputToPhotonLatency,ClickToPhotonLatency";
			csvFile << std::endl;
			return csvFile;
		}
		catch (const std::exception& e) {
			std::cout
				<< "a standard exception was caught, with message '"
				<< e.what() << "'" << std::endl;
			return std::nullopt;
		}
		catch (...) {
			std::cout << "Unknown Error" << std::endl;
			return std::nullopt;
		}
	}

	void WriteToCSV(std::ofstream& csvFile, const std::string& processName, const unsigned int& processId,
		PM_QUERY_ELEMENT(&queryElements)[16], pmapi::BlobContainer& blobs)
	{

		try {
			for (auto pBlob : blobs) {
				const auto appName = *reinterpret_cast<const char*>(&pBlob[queryElements[0].dataOffset]);
				const auto swapChain = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[1].dataOffset]);
				const auto graphicsRuntime = *reinterpret_cast<const PM_GRAPHICS_RUNTIME*>(&pBlob[queryElements[2].dataOffset]);
				const auto syncInterval = *reinterpret_cast<const int32_t*>(&pBlob[queryElements[3].dataOffset]);
				const auto presentFlags = *reinterpret_cast<const uint32_t*>(&pBlob[queryElements[4].dataOffset]);
				const auto allowsTearing = *reinterpret_cast<const bool*>(&pBlob[queryElements[5].dataOffset]);
				const auto presentMode = *reinterpret_cast<const PM_PRESENT_MODE*>(&pBlob[queryElements[6].dataOffset]);
				const auto cpuFrameQpc = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[7].dataOffset]);
				const auto cpuDuration = *reinterpret_cast<const double*>(&pBlob[queryElements[8].dataOffset]);
				const auto cpuFramePacingStall = *reinterpret_cast<const double*>(&pBlob[queryElements[9].dataOffset]);
				const auto gpuLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[10].dataOffset]);
				const auto gpuDuration = *reinterpret_cast<const double*>(&pBlob[queryElements[11].dataOffset]);
				const auto gpuBusyTime = *reinterpret_cast<const double*>(&pBlob[queryElements[12].dataOffset]);
				const auto gpuDisplayLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[13].dataOffset]);
				const auto gpuDisplayDuration = *reinterpret_cast<const double*>(&pBlob[queryElements[14].dataOffset]);
				const auto inputLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[15].dataOffset]);
				csvFile << appName << ",";
				// csvFile << "TestApp" << ",";
				csvFile << processId << ",";
				csvFile << std::hex << "0x" << std::dec << swapChain << ",";
				csvFile << TranslateGraphicsRuntime(graphicsRuntime) << ",";
				csvFile << syncInterval << ",";
				csvFile << presentFlags << ",";
				csvFile << allowsTearing << ",";
				csvFile << TranslatePresentMode(presentMode) << ",";
				csvFile << cpuFrameQpc << ",";
				csvFile << cpuDuration << ",";
				csvFile << cpuFramePacingStall << ",";
				csvFile << gpuLatency << ",";
				csvFile << gpuDuration << ",";
				csvFile << gpuBusyTime << ",";
				csvFile << 0 << ",";
				csvFile << gpuDisplayLatency << ",";
				csvFile << gpuDisplayDuration << ",";
				csvFile << inputLatency << "\n";
			}
		}
		catch (const std::exception& e) {
			std::cout
				<< "a standard exception was caught, with message '"
				<< e.what() << "'" << std::endl;
			return;
		}
		catch (...) {
			std::cout << "Unknown Error" << std::endl;
			return;
		}

	}

	void RunTestCaseV2(std::unique_ptr<pmapi::Session>&& pSession,
		const uint32_t& processId, const std::string& processName, CsvParser& goldCsvFile) {
		using namespace std::chrono_literals;
		pmapi::ProcessTracker processTracker;
		static constexpr uint32_t numberOfBlobs = 4000u;

		PM_QUERY_ELEMENT queryElements[]{
			//{ PM_METRIC_APPLICATION, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_SWAP_CHAIN_ADDRESS, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_SYNC_INTERVAL, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_PRESENT_FLAGS, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_ALLOWS_TEARING, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_FRAME_TYPE, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_CPU_START_QPC, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_CPU_FRAME_TIME, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_CPU_BUSY, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_CPU_WAIT, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_GPU_LATENCY, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_GPU_TIME, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_GPU_BUSY, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_GPU_WAIT, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_DISPLAY_LATENCY, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_DISPLAYED_TIME, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_ANIMATION_ERROR, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_ANIMATION_TIME, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_CLICK_TO_PHOTON_LATENCY, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_INSTRUMENTED_LATENCY, PM_STAT_NONE, 0, 0 }
		};

		auto frameQuery = pSession->RegisterFrameQuery(queryElements);
		auto blobs = frameQuery.MakeBlobContainer(numberOfBlobs);

		processTracker = pSession->TrackProcess(processId);

		while (1) {
			uint32_t numFrames = numberOfBlobs;
			try {
				frameQuery.Consume(processTracker, blobs);
			}
			catch (...) {
				// When processing ETL files an exception is generated when the
				// middleware discovers the ETL file is done being processed by
				// service and the client has consumed all produced frames. This
				// is the way.
				break;
			}

			if (blobs.GetNumBlobsPopulated() == 0) {
				std::this_thread::sleep_for(200ms);
			}
			else {
				try {
					goldCsvFile.VerifyBlobAgainstCsv(processName, processId, queryElements, blobs);
				}
				catch (const std::runtime_error& e) {
					std::cout << "Error: " << e.what() << std::endl;
					break;
				}
			}
		}

		processTracker.Reset();
	}

	TEST_CLASS(GoldEtlCsvTests)
	{
		std::optional<boost::process::child> oChild;
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
			if (oChild) {
				oChild->terminate();
				oChild->wait();
				oChild.reset();
			}
		}

		TEST_METHOD(OpenCsvTest)
		{
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";
			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, 1268);
			goldCsvFile.Close();
		}

		TEST_METHOD(OpenServiceTest)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);
		}
		TEST_METHOD(OpenMockSessionTest)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);
			
			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}
		}

		TEST_METHOD(TrackProcessTest)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			pmapi::ProcessTracker processTracker;
			processTracker.Reset();
		}

		TEST_METHOD(ConsumeBlobsTest)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 1268;
			const std::string processName = "dwm.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			pmapi::ProcessTracker processTracker;
			PM_QUERY_ELEMENT queryElements[]{
				//{ PM_METRIC_APPLICATION, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_SWAP_CHAIN_ADDRESS, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_SYNC_INTERVAL, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_FLAGS, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_ALLOWS_TEARING, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_START_QPC, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_FRAME_TIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_BUSY, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_WAIT, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_GPU_LATENCY, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_GPU_TIME, PM_STAT_NONE, 0, 0},
				{ PM_METRIC_GPU_BUSY, PM_STAT_NONE, 0, 0},
				{ PM_METRIC_GPU_WAIT, PM_STAT_NONE, 0, 0},
				{ PM_METRIC_DISPLAY_LATENCY, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_DISPLAYED_TIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY, PM_STAT_NONE, 0, 0},
				{ PM_METRIC_CLICK_TO_PHOTON_LATENCY, PM_STAT_NONE, 0, 0}
			};

			static constexpr uint32_t numberOfBlobs = 150u;

			auto frameQuery = pSession->RegisterFrameQuery(queryElements);
			auto blobs = frameQuery.MakeBlobContainer(numberOfBlobs);

			processTracker = pSession->TrackProcess(processId);

			while (1) {
				uint32_t numFrames = numberOfBlobs;
				try {
					frameQuery.Consume(processTracker, blobs);
				}
				catch (...) {
					// When processing ETL files an exception is generated when the
					// middleware discovers the ETL file is done being processed by
					// service and the client has consumed all produced frames. This
					// is the way.
					break;
				}

				if (blobs.GetNumBlobsPopulated() == 0) {
					std::this_thread::sleep_for(200ms);
				}
			}

			processTracker.Reset();
		}

		TEST_METHOD(Tc0v2Presenter10792)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10792;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				//"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2DWM1268)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 1268;
			const std::string processName = "dwm.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2Presenter8320)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 8320;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2Presenter11648)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11648;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2Presenter3976)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 3976;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2Presenter11112)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11112;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2Presenter2032)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 2032;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2Presenter5988)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5988;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2Presenter12268)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 12268;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc0v2Presenter11100)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11100;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}

		TEST_METHOD(Tc1v2Dwm1564)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11100;
			const std::string processName = "dwm.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc1v2Presenter24560)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 24560;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc1v2devenv24560)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 24944;
			const std::string processName = "devenv.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc2v2Dwm1300)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 1300;
			const std::string processName = "dwm.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc2v2Presenter10016)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10016;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc2v2Presenter5348)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5348;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc2v2Presenter5220)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5220;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc3v2Dwm1252)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 1252;
			const std::string processName = "dwm.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc3v2Presenter5892)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5892;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc3v2Presenter10112)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10112;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc3v2Presenter12980)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 12980;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc4v2Presenter5192)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5192;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc4v2Presenter5236)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5236;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc4v2Presenter8536)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 8536;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc4v2Presenter9620)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 9620;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc4v2Dwm10376)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10376;
			const std::string processName = "dwm.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc5v2PresentBench24892)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 24892;
			const std::string processName = "PresentBench.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_5.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_5.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc6v2CPXellOn10796Ext)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10796;
			const std::string processName = "cpLauncher.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_6.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_6.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc7v2CPXellOnFgOn11320Ext)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11320;
			const std::string processName = "cpLauncher.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_7.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_7.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc8v2ACSXellOnFgOn6920Ext)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 6920;
			const std::string processName = "scimitar_engine_win64_vs2022_llvm_fusion_dx12_px.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_7.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_7.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName.c_str(), introName.c_str());
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile);
			goldCsvFile.Close();
		}
	};

}
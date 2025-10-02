// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include <string>
#include <iostream>
#include <format>
#include <boost/process.hpp>
#include <cereal/archives/json.hpp>
#include <sstream>
#include <filesystem>
#include "ServiceTestCommands.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process;
namespace as = boost::asio;
namespace fs = std::filesystem;
using namespace std::literals;
using namespace pmon;

namespace MultiClientTests
{
	static constexpr const char* controlPipe_ = R"(\\.\pipe\pm-multi-test-ctrl)";
	static constexpr const char* introNsm_ = "pm_multi_test_intro";
	static constexpr const char* logFolder_ = "TestLogs\\MultiClient";
	static constexpr const char* logLevel_ = "info";

	TEST_MODULE_INITIALIZE(ModuleInit)
	{
		// Wipe the log folder before any tests run
		try {
			if (fs::exists(logFolder_)) {
				fs::remove_all(logFolder_);
			}
			fs::create_directories(logFolder_);
		}
		catch (const std::exception& ex) {
			Logger::WriteMessage(std::format("Failed to wipe log folder: {}\n", ex.what()).c_str());
			throw; // let MSTest see this as a test infrastructure error
		}
	}

	class JobManager
	{
	public:
		JobManager()
			:
			hJob_{ ::CreateJobObjectA(nullptr, nullptr) }
		{
			if (!hJob_) ThrowLastError_("CreateJobObject");

			JOBOBJECT_EXTENDED_LIMIT_INFORMATION li{};
			li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			if (!::SetInformationJobObject(
				hJob_, JobObjectExtendedLimitInformation, &li, sizeof(li))) {
				ThrowLastError_("SetInformationJobObject");
			}
		}
		// Attach a child process HANDLE to the job.
		void Attach(HANDLE process_handle) const
		{
			if (!::AssignProcessToJobObject(hJob_, process_handle)) {
				ThrowLastError_("AssignProcessToJobObject");
			}
		}
	private:
		static void ThrowLastError_(const char* where)
		{
			throw std::system_error(::GetLastError(), std::system_category(), where);
		}

		util::win::Handle hJob_;
	};

	class TestProcess
	{
	public:
		TestProcess(as::io_context& ioctx, JobManager& jm, const std::string& executable, const std::vector<std::string>& args)
			:
			pipeFrom_{ ioctx },
			pipeTo_{ ioctx },
			process_{ ioctx, executable, args,
				bp::process_stdio{ pipeTo_, pipeFrom_, nullptr } }
		{
			jm.Attach(process_.native_handle());
			Logger::WriteMessage(std::format(" - Launched process {{{}}} [{}]\n",
				executable, process_.id()).c_str());
			Assert::AreEqual("ping-ok"s, Command("ping"));
		}
		~TestProcess()
		{
			if (process_.running()) {
				Quit();
			}
		}

		TestProcess(const TestProcess&) = delete;
		TestProcess & operator=(const TestProcess&) = delete;
		TestProcess(TestProcess&&) = delete;
		TestProcess & operator=(TestProcess&&) = delete;

		void Quit()
		{
			Assert::IsTrue(process_.running());
			Assert::AreEqual("quit-ok"s, Command("quit"));
			process_.wait();
		}
		void Murder()
		{
			Assert::IsTrue(process_.running());
			::TerminateProcess(process_.native_handle(), 0xDEAD);
			process_.wait();
		}
		std::string Command(const std::string command)
		{
			// send command
			as::write(pipeTo_, as::buffer(std::format("%{}\n", command)));

			// read through the start marker and drop it (and any leading junk)
			const auto n = as::read_until(pipeFrom_, readBufferFrom_, preamble_);
			readBufferFrom_.consume(n);

			// read through the end marker, m counts bytes up to and including postamble
			const auto m = as::read_until(pipeFrom_, readBufferFrom_, postamble_);

			// size string to accept payload
			constexpr auto postambleSize = std::size(postamble_) - 1;
			std::string payload;
			payload.resize(m - postambleSize);

			// read into sized string using stream wrapper and discard postamble
			std::istream is(&readBufferFrom_);
			is.read(&payload[0], static_cast<std::streamsize>(payload.size()));
			readBufferFrom_.consume(postambleSize);

			return payload;
		}
	private:
		constexpr static const char preamble_[] = "%%{";
		constexpr static const char postamble_[] = "}%%\r\n";
		as::readable_pipe pipeFrom_;
		as::streambuf readBufferFrom_;
		as::writable_pipe pipeTo_;
		bp::process process_;
	};

	class ServiceProcess : public TestProcess
	{
	public:
		ServiceProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs = {})
			:
			TestProcess{ ioctx, jm, "PresentMonService.exe"s, MakeArgs_(customArgs) }
		{}
		test::service::Status QueryStatus()
		{
			test::service::Status status;
			std::istringstream is{ Command("status") };
			cereal::JSONInputArchive{ is }(status);
			return status;
		}
	private:
		std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs)
		{
			std::vector<std::string> allArgs{
				"--control-pipe"s, controlPipe_,
				"--nsm-prefix"s, "pm_multi_test_nsm"s,
				"--intro-nsm"s, introNsm_,
				"--enable-test-control"s,
				"--log-dir"s, logFolder_,
				"--log-name-pid"s,
				"--log-level"s, std::string(logLevel_),
			};
			allArgs.append_range(customArgs);
			return allArgs;
		}
	};

	class ClientProcess : public TestProcess
	{
	public:
		ClientProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs = {})
			:
			TestProcess{ ioctx, jm, "SampleClient.exe"s, MakeArgs_(customArgs) }
		{}
	private:
		std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs)
		{
			std::vector<std::string> allArgs{
				"--control-pipe"s, controlPipe_,
				"--intro-nsm"s, introNsm_,
				"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
				"--log-folder"s, std::string(logFolder_),
				"--log-name-pid"s,
				"--log-level"s, std::string(logLevel_),
				"--mode"s, "MultiClient"s,
			};
			allArgs.append_range(customArgs);
			return allArgs;
		}
	};

	struct CommonTestFixture
	{
		JobManager jobMan;
		std::thread ioctxRunThread;
		as::io_context ioctx;
		std::optional<ServiceProcess> service;

		void Setup()
		{
			service.emplace(ioctx, jobMan);
			ioctxRunThread = std::thread{ [&] {pmquell(ioctx.run()); } };
		}
		void Cleanup()
		{
			service.reset();
			ioctxRunThread.join();
			// sleep after every test to ensure that named pipe is no longer available
			std::this_thread::sleep_for(50ms);
		}
		ClientProcess LaunchClient(std::vector<std::string> args = {})
		{
			return ClientProcess{ ioctx, jobMan, std::move(args) };
		}
	};

	TEST_CLASS(CommonFixtureTests)
	{
		CommonTestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// verify service lifetime and status command functionality
		TEST_METHOD(ServiceStatusTest)
		{
			// verify initial status
			const auto status = fixture_.service->QueryStatus();
			Assert::AreEqual(0ull, status.nsmStreamedPids.size());
			Assert::AreEqual(16u, status.telemetryPeriodMs);
			Assert::IsTrue((bool)status.etwFlushPeriodMs);
			Assert::AreEqual(1000u, *status.etwFlushPeriodMs);
		}
		// verify client lifetime
		TEST_METHOD(ClientLaunchTest)
		{
			auto client = fixture_.LaunchClient();
		}
	};

	TEST_CLASS(TelemetryPeriodTests)
	{
		CommonTestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// basic test to see single client changing telemetry
		TEST_METHOD(OneClientSetting)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}
		}
		// two client test, 2nd client has superceded period
		TEST_METHOD(SecondClientSuperseded)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "135"s,
			});
			// check that telemetry period has not changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}
		}
		// two client test, 2nd client overrides
		TEST_METHOD(SecondClientOverrides)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "36"s,
			});
			// check that telemetry period has been overrided
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(36u, status.telemetryPeriodMs);
			}
		}
		// two client test, verify override and then reversion when clients disconnect
		TEST_METHOD(TwoClientReversion)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "36"s,
			});
			// check that telemetry period has been overrided
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(36u, status.telemetryPeriodMs);
			}

			// kill client 2
			client2.Quit();
			// verify reversion to client 1's request
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// kill client 1
			client1.Quit();
			// verify reversion to default
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(16u, status.telemetryPeriodMs);
			}
		}
		// verify reversion on sudden client death
		TEST_METHOD(ClientMurderReversion)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "36"s,
			});
			// check that telemetry period has been overrided
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(36u, status.telemetryPeriodMs);
			}

			// murder client 2
			client2.Murder();
			// there is a lag between when a process is abruptly terminated and when the pipe ruptures
			// causing the Service session to be disposed; tolerate max 5ms
			std::this_thread::sleep_for(5ms);
			// verify reversion to client 1's request
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// murder client 1
			client1.Murder();
			// there is a lag between when a process is abruptly terminated and when the pipe ruptures
			// causing the Service session to be disposed; tolerate max 5ms
			std::this_thread::sleep_for(5ms);
			// verify reversion to default
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(16u, status.telemetryPeriodMs);
			}
		}
		// verify range check error low
		TEST_METHOD(OutOfRangeLow)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "3"s,
				"--test-expect-error"s,
			});
			// check for expected error
			Assert::AreEqual("err-check-ok:PM_STATUS_OUT_OF_RANGE"s, client.Command("err-check"));
		}
	};

	TEST_CLASS(EtwFlushPeriodTests)
	{
		CommonTestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// basic test to see single client changing flush
		TEST_METHOD(OneClientSetting)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}
		}
		// two client test, 2nd client has superceded period
		TEST_METHOD(SecondClientSuperseded)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "65"s,
			});
			// check that flush period has not changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}
		}
		// two client test, 2nd client overrides (smaller value wins)
		TEST_METHOD(SecondClientOverrides)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// launch a second client with a smaller period (should override)
			auto client2 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "35"s,
			});
			// check that flush period has been overridden
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(35u, *status.etwFlushPeriodMs);
			}
		}
		// two client test, verify override and then reversion when clients disconnect
		TEST_METHOD(TwoClientReversion)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// launch a second client with a smaller period (override)
			auto client2 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "35"s,
			});
			// verify overridden to smaller value
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(35u, *status.etwFlushPeriodMs);
			}

			// kill client 2; should revert to client 1's request
			client2.Quit();
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// kill client 1; should revert to default (1000 ms per ServiceStatusTest)
			client1.Quit();
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(1000u, *status.etwFlushPeriodMs);
			}
		}
		// verify reversion on sudden client death
		TEST_METHOD(ClientMurderReversion)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// launch a second client with a smaller period (override)
			auto client2 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "35"s,
			});
			// verify overridden value
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(35u, *status.etwFlushPeriodMs);
			}

			// murder client 2; allow brief lag for pipe/session cleanup
			client2.Murder();
			std::this_thread::sleep_for(5ms);
			// verify reversion to client 1's request
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// murder client 1; allow brief lag for pipe/session cleanup
			client1.Murder();
			std::this_thread::sleep_for(5ms);
			// verify reversion to default
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(1000u, *status.etwFlushPeriodMs);
			}
		}
	};
}
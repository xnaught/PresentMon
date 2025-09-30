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
#include "ServiceTestCommands.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process;
namespace as = boost::asio;
using namespace std::literals;
using namespace pmon;

namespace MultiClientTests
{
	static constexpr const char* controlPipe_ = R"(\\.\pipe\pm-multi-test-ctrl)";
	static constexpr const char* introNsm_ = "pm_multi_test_intro";

	class TestProcess
	{
	public:
		TestProcess(as::io_context& ioctx, const std::string& executable, const std::vector<std::string>& args)
			:
			pipeFrom_{ ioctx },
			pipeTo_{ ioctx },
			process_{ ioctx, executable, args,
				bp::process_stdio{ pipeTo_, pipeFrom_, nullptr } }
		{
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
		ServiceProcess(as::io_context& ioctx, const std::vector<std::string>& customArgs = {})
			:
			TestProcess{ ioctx, "PresentMonService.exe"s, MakeArgs_(customArgs) }
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
			};
			allArgs.append_range(customArgs);
			return allArgs;
		}
	};

	class ClientProcess : public TestProcess
	{
	public:
		ClientProcess(as::io_context& ioctx, const std::vector<std::string>& customArgs = {})
			:
			TestProcess{ ioctx, "SampleClient.exe"s, MakeArgs_(customArgs) }
		{}
	private:
		std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs)
		{
			std::vector<std::string> allArgs{
				"--control-pipe"s, controlPipe_,
				"--intro-nsm"s, introNsm_,
				"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
				"--mode"s, "MultiClient"s,
			};
			allArgs.append_range(customArgs);
			return allArgs;
		}
	};

	struct CommonTestFixture
	{
		std::thread ioctxRunThread;
		as::io_context ioctx;
		std::optional<ServiceProcess> service;

		void Setup()
		{
			service.emplace(ioctx);
			ioctxRunThread = std::thread{ [&] {pmquell(ioctx.run()); } };
		}
		void Cleanup()
		{
			service.reset();
			ioctxRunThread.join();
			// sleep after every test to ensure that named pipe is no longer available
			std::this_thread::sleep_for(50ms);
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
			Assert::IsFalse((bool)status.etwFlushPeriodMs);
		}
		// verify client lifetime
		TEST_METHOD(ClientLaunchTest)
		{
			ClientProcess client{
				fixture_.ioctx,
			};
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
			ClientProcess client{
				fixture_.ioctx, {
					"--telemetry-period-ms"s, "63"s,
				},
			};
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
			ClientProcess client1{
				fixture_.ioctx, {
					"--telemetry-period-ms"s, "63"s,
				},
			};
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			ClientProcess client2{
				fixture_.ioctx, {
					"--telemetry-period-ms"s, "135"s,
				},
			};
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
			ClientProcess client1{
				fixture_.ioctx, {
					"--telemetry-period-ms"s, "63"s,
				},
			};
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			ClientProcess client2{
				fixture_.ioctx, {
					"--telemetry-period-ms"s, "36"s,
				},
			};
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
			ClientProcess client1{
				fixture_.ioctx, {
					"--telemetry-period-ms"s, "63"s,
				},
			};
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			ClientProcess client2{
				fixture_.ioctx, {
					"--telemetry-period-ms"s, "36"s,
				},
			};
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
	};
}
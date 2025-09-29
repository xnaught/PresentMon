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
	class ClientProcess
	{
	public:
		ClientProcess(as::io_context& ioctx_, std::vector<std::string> args)
			:
			pipeFromClient_{ ioctx_ },
			pipeToClient_{ ioctx_ },
			client_{ ioctx_, "SampleClient.exe"s, std::move(args),
				bp::process_stdio{ pipeToClient_, pipeFromClient_, nullptr } }
		{
			Assert::AreEqual("ping-ok"s, Command("ping"));
		}
		~ClientProcess()
		{
			if (client_.is_open()) {
				Assert::AreEqual( "quit-ok"s, Quit());
			}
		}
		std::string Quit()
		{
			auto ret = Command("quit");
			client_.wait();
			return ret;
		}
		std::string Command(const std::string command)
		{
			// write command string
			as::write(pipeToClient_, as::buffer(std::format("%{}\n", command)));
			// read response with special terminator sequence
			std::size_t n = as::read_until(pipeFromClient_, readBufferFromClient_, "%%}\n");
			
			std::string payload;
			payload.resize(n - 5);

			std::istream is(&readBufferFromClient_);
			is.read(&payload[0], static_cast<std::streamsize>(payload.size()));
			readBufferFromClient_.consume(5); // remove terminator sequence

			return payload;
		}
	private:
		as::readable_pipe pipeFromClient_;   // client's stdout
		as::streambuf readBufferFromClient_; // client's stdout
		as::writable_pipe pipeToClient_;     // client's stdin
		bp::process client_;
	};

	TEST_CLASS(MultiClientTests)
	{
		static constexpr const char* controlPipe_ = R"(\\.\pipe\pm-multi-test-ctrl)";
		static constexpr const char* introNsm_ = "pm_multi_test_intro";
		std::thread ioctxRunThread_;
		as::io_context ioctx_;
		bp::process service_{ ioctx_ };
		as::readable_pipe pipeFromSvc_{ ioctx_ };	// service's stdout
		as::streambuf readBufferFromSvc_;			// service's stdout
		as::writable_pipe pipeToSvc_{ ioctx_ };     // service's stdin
		as::streambuf writeBufferToSvc_;			// service's stdin

		std::string CommandService(const std::string& command)
		{
			// send command
			as::write(pipeToSvc_, as::buffer(std::format("%{}\n", command)));

			// 1) Read through the start marker and drop it (and any leading junk)
			std::size_t n = as::read_until(pipeFromSvc_, readBufferFromSvc_, "{%");
			readBufferFromSvc_.consume(n); // discard up to and including "{%"

			// 2) Read through the end marker
			std::size_t m = as::read_until(pipeFromSvc_, readBufferFromSvc_, "%}");

			// m counts bytes up to and including "%}". Extract payload (m-2 bytes), then drop "%}"
			std::string payload;
			payload.resize(m - 2);

			std::istream is(&readBufferFromSvc_);
			is.read(&payload[0], static_cast<std::streamsize>(payload.size()));
			readBufferFromSvc_.consume(2); // remove "%}"

			return payload;
		}
		test::service::Status CommandServiceStatus()
		{
			test::service::Status status;
			std::istringstream is{ CommandService("status") };
			cereal::JSONInputArchive{ is }(status);
			return status;
		}

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			service_ = bp::process{
				ioctx_,
				"PresentMonService.exe"s,
				std::vector<std::string>{
					"--control-pipe"s, controlPipe_,
					"--nsm-prefix"s, "pm_multi_test_nsm"s,
					"--intro-nsm"s, introNsm_,
					"--enable-test-control"s,
				},
				bp::process_stdio{ pipeToSvc_, pipeFromSvc_, nullptr }
			};
			ioctxRunThread_ = std::thread{ [&] {pmquell(ioctx_.run()); } };
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			Assert::AreEqual("QUIT_OK"s, CommandService("quit"));
			service_.terminate();
			service_.wait();
			ioctx_.stop();
			ioctxRunThread_.join();
			// sleep after every test to ensure that named pipe is no longer available
			std::this_thread::sleep_for(50ms);
		}
		TEST_METHOD(ServiceCommandTest)
		{
			// verify initial status
			const auto status = CommandServiceStatus();
			Assert::AreEqual(0ull, status.nsmStreamedPids.size());
			Assert::AreEqual(16u, status.telemetryPeriodMs);
			Assert::IsFalse((bool)status.etwFlushPeriodMs);
		}
		// basic test to see single client changing telemetry
		TEST_METHOD(TelemetryPeriodTest1)
		{
			// verify initial status
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(0ull, status.nsmStreamedPids.size());
				Assert::AreEqual(16u, status.telemetryPeriodMs);
				Assert::IsFalse((bool)status.etwFlushPeriodMs);
			}
			// launch a client
			//ClientProcess client{
			//	ioctx_,
			//	std::vector<std::string>{
			//		"--control-pipe"s, controlPipe_,
			//		"--intro-nsm"s, introNsm_,
			//		"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
			//		"--mode"s, "MultiClient"s,
			//		"--telemetry-period-ms"s, "63"s
			//	},
			//};
			bp::process client1{
				ioctx_,
				"SampleClient.exe"s,
				std::vector<std::string>{
					"--control-pipe"s, controlPipe_,
					"--intro-nsm"s, introNsm_,
					"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
					"--mode"s, "MultiClient"s,
					"--telemetry-period-ms"s, "63"s
				}
			};			
			// grace period
			std::this_thread::sleep_for(350ms);
			// check that telemetry period has changed
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}
		}
		// two client test, 2nd client has superceded period
		TEST_METHOD(TelemetryPeriodTest2)
		{
			// verify initial status
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(0ull, status.nsmStreamedPids.size());
				Assert::AreEqual(16u, status.telemetryPeriodMs);
				Assert::IsFalse((bool)status.etwFlushPeriodMs);
			}
			// launch a client
			bp::process client1{
				ioctx_,
				"SampleClient.exe"s,
				std::vector<std::string>{
					"--control-pipe"s, controlPipe_,
					"--intro-nsm"s, introNsm_,
					"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
					"--mode"s, "MultiClient"s,
					"--telemetry-period-ms"s, "63"s
				}
			};
			// grace period
			std::this_thread::sleep_for(350ms);
			// check that telemetry period has changed
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			bp::process client2{
				ioctx_,
				"SampleClient.exe"s,
				std::vector<std::string>{
					"--control-pipe"s, controlPipe_,
					"--intro-nsm"s, introNsm_,
					"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
					"--mode"s, "MultiClient"s,
					"--telemetry-period-ms"s, "163"s
				}
			};
			// grace period
			std::this_thread::sleep_for(350ms);
			// verify that telemetry period remains the same
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}
		}
		// two client test, 2nd client overrides
		TEST_METHOD(TelemetryPeriodTest3)
		{
			// verify initial status
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(0ull, status.nsmStreamedPids.size());
				Assert::AreEqual(16u, status.telemetryPeriodMs);
				Assert::IsFalse((bool)status.etwFlushPeriodMs);
			}
			// launch a client
			bp::process client1{
				ioctx_,
				"SampleClient.exe"s,
				std::vector<std::string>{
					"--control-pipe"s, controlPipe_,
					"--intro-nsm"s, introNsm_,
					"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
					"--mode"s, "MultiClient"s,
					"--telemetry-period-ms"s, "63"s
				}
			};
			// grace period
			std::this_thread::sleep_for(350ms);
			// check that telemetry period has changed
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			bp::process client2{
				ioctx_,
				"SampleClient.exe"s,
				std::vector<std::string>{
					"--control-pipe"s, controlPipe_,
					"--intro-nsm"s, introNsm_,
					"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
					"--mode"s, "MultiClient"s,
					"--telemetry-period-ms"s, "36"s
				}
			};
			// grace period
			std::this_thread::sleep_for(350ms);
			// check that telemetry period has changed
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(36u, status.telemetryPeriodMs);
			}
		}
		// two client test, verify override and reversion
		TEST_METHOD(TelemetryPeriodTest4)
		{
			// verify initial status
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(0ull, status.nsmStreamedPids.size());
				Assert::AreEqual(16u, status.telemetryPeriodMs);
				Assert::IsFalse((bool)status.etwFlushPeriodMs);
			}
			// launch a client
			bp::process client1{
				ioctx_,
				"SampleClient.exe"s,
				std::vector<std::string>{
					"--control-pipe"s, controlPipe_,
					"--intro-nsm"s, introNsm_,
					"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
					"--mode"s, "MultiClient"s,
					"--telemetry-period-ms"s, "63"s
				}
			};
			// grace period
			std::this_thread::sleep_for(350ms);
			// check that telemetry period has changed
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			bp::process client2{
				ioctx_,
				"SampleClient.exe"s,
				std::vector<std::string>{
					"--control-pipe"s, controlPipe_,
					"--intro-nsm"s, introNsm_,
					"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
					"--mode"s, "MultiClient"s,
					"--telemetry-period-ms"s, "36"s
				}
			};
			// grace period
			std::this_thread::sleep_for(350ms);
			// check that telemetry period has changed
			{
				const auto status = CommandServiceStatus();
				Assert::AreEqual(36u, status.telemetryPeriodMs);
			}
		}
	};
}
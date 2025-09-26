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
	TEST_CLASS(MultiClientTests)
	{
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
					"--control-pipe"s, R"(\\.\pipe\pm-multi-test-ctrl)"s,
					"--nsm-prefix"s, "pm_multi_test_nsm"s,
					"--intro-nsm"s, "pm_multi_test_intro"s,
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
			const auto status = CommandServiceStatus();
			Assert::IsTrue(!status.etwFlushPeriodMs);
			Assert::AreEqual(0ull, status.nsmStreamedPids.size());
		}
	};
}
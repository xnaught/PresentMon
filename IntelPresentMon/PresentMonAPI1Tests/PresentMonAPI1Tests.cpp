#include "CppUnitTest.h"
#include "../PresentMonAPI2Tests/BoostProcess.h"
#include "../PresentMonAPI/PresentMonAPI.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI1Tests
{
	TEST_CLASS(PresentMonAPI1Tests)
	{
	public:
		TEST_METHOD(LaunchServiceAsConsole)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			bp::child process("PresentMonService.exe"s,
				"--timed-stop"s, "100"s,
				"--control-pipe"s, R"(\\.\pipe\test-pipe-pmsvca)"s,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				bp::std_out > out, bp::std_in < in);

			process.wait();
			Assert::AreEqual(0, process.exit_code());
		}
		TEST_METHOD(ConnectToServicePipe)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvcb)"s;

			bp::child process("PresentMonService.exe"s,
				"--timed-stop"s, "2000"s,
				"--control-pipe"s, pipeName.c_str(),
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(10ms);

			{
				const auto sta = pmInitialize(pipeName.c_str());
				Assert::AreEqual(int(PM_STATUS_SUCCESS), int(sta), L"*** Connecting to service via named pipe");
			}

			process.terminate();
			process.wait();
		}
	};
}

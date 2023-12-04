#include "CppUnitTest.h"
#include <boost/process.hpp>

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
				"--control-pipe"s, R"(\\.\pipe\test-pipe-pmsvc)"s,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				bp::std_out > out, bp::std_in < in);

			process.wait();
			Assert::AreEqual(0, process.exit_code());
		}
	};
}

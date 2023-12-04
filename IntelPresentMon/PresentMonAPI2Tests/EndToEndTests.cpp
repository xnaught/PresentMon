#include "CppUnitTest.h"
#include <boost/process.hpp>
#include "../PresentMonAPI2/source/Internal.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EndToEndTests
{
	TEST_CLASS(CAPIToServiceTests)
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
		TEST_METHOD(ConnectDisconnectToServicePipe)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-1)"s;

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "4000"s,
				"--control-pipe"s, pipeName.c_str(),
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(10ms);

			{
				const auto sta = pmOpenSession_(pipeName.c_str(), nullptr);
				Assert::AreEqual(int(PM_STATUS_SUCCESS), int(sta), L"Connecting to service via named pipe");
			}
			{
				const auto sta = pmCloseSession();
				Assert::AreEqual(int(PM_STATUS_SUCCESS), int(sta));
			}
		}
		TEST_METHOD(AcquireIntrospectionData)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "pm_intro_test_nsm_1"s;

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "400000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(10ms);

			{
				const auto sta = pmOpenSession_(pipeName.c_str(), introName.c_str());
				Assert::AreEqual(int(PM_STATUS_SUCCESS), int(sta), L"Connecting to service via named pipe");
			}

			{
				const PM_INTROSPECTION_ROOT* pRoot = nullptr;
				const auto sta = pmEnumerateInterface(&pRoot);
				Assert::AreEqual(int(PM_STATUS_SUCCESS), int(sta));

				Assert::IsNotNull(pRoot);
				Assert::AreEqual(12ull, pRoot->pEnums->size);
				Assert::AreEqual(14ull, pRoot->pMetrics->size);

				Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeInterface(pRoot));
			}

			{
				const auto sta = pmCloseSession();
				Assert::AreEqual(int(PM_STATUS_SUCCESS), int(sta));
			}
		}
	};
}
#include "CppUnitTest.h"
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include "../PresentMonAPI2/source/Internal.h"
#include <cstring>
#include <vector>
#include <optional>
#include "Utilities.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2
{
	TEST_CLASS(CAPIStaticQueryTests)
	{
	public:
		TEST_METHOD_INITIALIZE(BeforeEachTestMethod)
		{
			pmSetMiddlewareAsMock_(true, true);
			pmOpenSession();
		}
		TEST_METHOD_CLEANUP(AfterEachTestMethod)
		{
			pmCloseSession();
		}
		TEST_METHOD(PollStaticMetricString)
		{
			PM_QUERY_ELEMENT element{ .metric = PM_METRIC_PROCESS_NAME, .deviceId = 0, .arrayIndex = 0 };
			auto pBlob = std::make_unique<uint8_t[]>(260);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmPollStaticQuery(&element, pBlob.get()));
			Assert::AreEqual("dota2.exe", (const char*)pBlob.get());
		}
		TEST_METHOD(FailToPollDynamicMetricAsStatic)
		{
			PM_QUERY_ELEMENT element{ .metric = PM_METRIC_CPU_UTILIZATION, .deviceId = 0, .arrayIndex = 0 };
			auto pBlob = std::make_unique<uint8_t>(8);
			Assert::AreEqual((int)PM_STATUS_FAILURE, (int)pmPollStaticQuery(&element, pBlob.get()));
		}
	};
}

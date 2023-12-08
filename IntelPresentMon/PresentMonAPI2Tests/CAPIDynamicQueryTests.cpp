#include "CppUnitTest.h"
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include "../PresentMonAPI2/source/Internal.h"
#include <cstring>
#include <vector>
#include <optional>
#include "Utilities.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2Mock
{
	TEST_CLASS(CAPIDynamicQueryTests)
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
		TEST_METHOD(CreateAndFreeQuery)
		{
			const auto heapBefore = pmCreateHeapCheckpoint_();

			PM_DYNAMIC_QUERY_HANDLE q = nullptr;
			PM_QUERY_ELEMENT elements[2]{
				PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_UTILIZATION, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_POWER, .deviceId = 1, .arrayIndex = 0},
			};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmRegisterDynamicQuery(&q, elements, std::size(elements), 4004, 1000.));
			Assert::IsNotNull(q);

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeDynamicQuery(q));

			const auto heapAfter = pmCreateHeapCheckpoint_();
			// not sure why moving introspection root creation causes this leak
			Assert::IsFalse(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
		TEST_METHOD(VerifyOffsetsAndSizes)
		{
			PM_DYNAMIC_QUERY_HANDLE q = nullptr;
			PM_QUERY_ELEMENT elements[]{
				PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_UTILIZATION, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENT_MODE, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_POWER, .deviceId = 1, .arrayIndex = 0},
			};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmRegisterDynamicQuery(&q, elements, std::size(elements), 4004, 1000.));
			Assert::IsNotNull(q);

			Assert::AreEqual(0ull, elements[0].dataOffset);
			Assert::AreEqual(8ull, elements[0].dataSize);
			Assert::AreEqual(8ull, elements[1].dataOffset);
			Assert::AreEqual(4ull, elements[1].dataSize);
			Assert::AreEqual(12ull, elements[2].dataOffset);
			Assert::AreEqual(8ull, elements[2].dataSize);

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeDynamicQuery(q));
		}
		TEST_METHOD(FailToRegisterStaticQuery)
		{
			PM_DYNAMIC_QUERY_HANDLE q = nullptr;
			PM_QUERY_ELEMENT elements[]{
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PROCESS_NAME, .deviceId = 0, .arrayIndex = 0},
			};
			Assert::AreEqual((int)PM_STATUS_FAILURE, (int)pmRegisterDynamicQuery(&q, elements, std::size(elements), 4004, 1000.));
			Assert::IsNull(q);
		}
		TEST_METHOD(PollValuesTimeZero)
		{
			PM_DYNAMIC_QUERY_HANDLE q = nullptr;
			PM_QUERY_ELEMENT elements[]{
				PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_UTILIZATION, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENT_MODE, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_POWER, .deviceId = 1, .arrayIndex = 0},
			};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmRegisterDynamicQuery(&q, elements, std::size(elements), 4004, 1000.));
			Assert::IsNotNull(q);

			auto pBlob = std::make_unique<uint8_t[]>(elements[2].dataOffset + elements[2].dataSize);
			uint32_t numSwapChains = 1;

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmPollDynamicQuery(q, 4004, pBlob.get(), &numSwapChains));
			Assert::AreEqual((double)PM_METRIC_CPU_UTILIZATION, reinterpret_cast<double&>(pBlob[elements[0].dataOffset]));
			Assert::AreEqual((int)PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP, reinterpret_cast<int&>(pBlob[elements[1].dataOffset]));
			Assert::AreEqual((double)PM_METRIC_GPU_POWER, reinterpret_cast<double&>(pBlob[elements[2].dataOffset]));

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeDynamicQuery(q));
		}
		TEST_METHOD(PollValuesOverTime)
		{
			PM_DYNAMIC_QUERY_HANDLE q = nullptr;
			PM_QUERY_ELEMENT elements[]{
				PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_UTILIZATION, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENT_MODE, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_POWER, .deviceId = 1, .arrayIndex = 0},
			};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmRegisterDynamicQuery(&q, elements, std::size(elements), 4004, 1000.));
			Assert::IsNotNull(q);

			auto pBlob = std::make_unique<uint8_t[]>(elements[2].dataOffset + elements[2].dataSize);
			uint32_t numSwapChains = 1;

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmPollDynamicQuery(q, 4004, pBlob.get(), &numSwapChains));
			Assert::AreEqual((double)PM_METRIC_CPU_UTILIZATION, reinterpret_cast<double&>(pBlob[elements[0].dataOffset]));
			Assert::AreEqual((int)PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP, reinterpret_cast<int&>(pBlob[elements[1].dataOffset]));
			Assert::AreEqual((double)PM_METRIC_GPU_POWER, reinterpret_cast<double&>(pBlob[elements[2].dataOffset]));

			pmMiddlewareAdvanceTime_(1);

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmPollDynamicQuery(q, 4004, pBlob.get(), &numSwapChains));
			Assert::AreEqual(0., reinterpret_cast<double&>(pBlob[elements[0].dataOffset]));
			Assert::AreEqual((int)PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP, reinterpret_cast<int&>(pBlob[elements[1].dataOffset]));
			Assert::AreEqual(0., reinterpret_cast<double&>(pBlob[elements[2].dataOffset]));

			pmMiddlewareAdvanceTime_(1);

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmPollDynamicQuery(q, 4004, pBlob.get(), &numSwapChains));
			Assert::AreEqual((double)PM_METRIC_CPU_UTILIZATION, reinterpret_cast<double&>(pBlob[elements[0].dataOffset]));
			Assert::AreEqual((int)PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP, reinterpret_cast<int&>(pBlob[elements[1].dataOffset]));
			Assert::AreEqual((double)PM_METRIC_GPU_POWER, reinterpret_cast<double&>(pBlob[elements[2].dataOffset]));

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeDynamicQuery(q));
		}

		TEST_METHOD(UnsupportedMetric)
		{
			PM_DYNAMIC_QUERY_HANDLE q = nullptr;
			int PM_METRIC_GPU_VOLTAGE = PM_METRIC_CPU_FREQUENCY + 10;
			PM_QUERY_ELEMENT elements[]{
				PM_QUERY_ELEMENT{.metric = (PM_METRIC)PM_METRIC_GPU_VOLTAGE, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
			};
			Assert::AreEqual((int)PM_STATUS_FAILURE, (int)pmRegisterDynamicQuery(&q, elements, std::size(elements), 4004, 1000.));
		}
		
		TEST_METHOD(FpsMetricSize)
		{
			PM_DYNAMIC_QUERY_HANDLE q = nullptr;
			PM_QUERY_ELEMENT elements[7]{
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
				PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
			};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmRegisterDynamicQuery(&q, elements, std::size(elements), 4004, 1000.));
			Assert::IsNotNull(q);

			Assert::AreEqual(0ull, elements[0].dataOffset);
			Assert::AreEqual(8ull, elements[0].dataSize);
			Assert::AreEqual(8ull, elements[1].dataOffset);
			Assert::AreEqual(8ull, elements[1].dataSize);
			Assert::AreEqual(16ull, elements[2].dataOffset);
			Assert::AreEqual(8ull, elements[2].dataSize);

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeDynamicQuery(q));
		}
	};
}

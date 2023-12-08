#include "CppUnitTest.h"
#include "../PresentMonUtils/PresentMonNamedPipe.h"
#include "../PresentMonMiddleware/source/FrameEventQuery.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2Mock
{
	TEST_CLASS(MiddlewareTests)
	{
	public:
		TEST_METHOD(TestQuery)
		{
			const PmNsmFrameData frame{
				.power_telemetry = {
					.gpu_power_w = 420.,
				},
				.cpu_telemetry = {
					.cpu_utilization = 30.,
				},
			};
			PM_QUERY_ELEMENT queryElements[]{
				{ PM_METRIC_GPU_POWER, PM_STAT_NONE, 1, 0 },
				{ PM_METRIC_CPU_UTILIZATION, PM_STAT_NONE, 0, 0 }
			};
			PM_FRAME_EVENT_QUERY query{ queryElements };
			auto pBlob = std::make_unique<uint8_t[]>(16);
			query.GatherToBlob((const uint8_t*)&frame, pBlob.get());
			Assert::AreEqual(420., *(double*)pBlob.get());
			Assert::AreEqual(30., *(double*)&pBlob.get()[8]);
		}
		TEST_METHOD(TestQueryEnum)
		{
			const PmNsmFrameData frame{
				.present_event = {
					.PresentMode = PresentMode::Composed_Flip,
				},
				.power_telemetry = {
					.gpu_power_w = 420.,
				},
				.cpu_telemetry = {
					.cpu_utilization = 30.,
				},
			};
			PM_QUERY_ELEMENT queryElements[]{
				{ PM_METRIC_GPU_POWER, PM_STAT_NONE, 1, 0 },
				{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_UTILIZATION, PM_STAT_NONE, 0, 0 }
			};
			PM_FRAME_EVENT_QUERY query{ queryElements };
			auto pBlob = std::make_unique<uint8_t[]>(24);
			query.GatherToBlob((const uint8_t*)&frame, pBlob.get());
			Assert::AreEqual(420., *(double*)&pBlob.get()[0]);
			Assert::AreEqual((int)PM_PRESENT_MODE_COMPOSED_FLIP, *(int*)&pBlob.get()[8]);
			Assert::AreEqual(30., *(double*)&pBlob.get()[16]);
		}
		TEST_METHOD(TestQueryRuntime)
		{
			const PmNsmFrameData frame{
				.present_event = {
					.Runtime = Runtime::DXGI,
					.PresentMode = PresentMode::Composed_Flip,
				},
				.power_telemetry = {
					.gpu_power_w = 420.,
				},
				.cpu_telemetry = {
					.cpu_utilization = 30.,
				},
			};
			PM_QUERY_ELEMENT queryElements[]{
				{ PM_METRIC_GPU_POWER, PM_STAT_NONE, 1, 0 },
				{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_RUNTIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_UTILIZATION, PM_STAT_NONE, 0, 0 }
			};
			PM_FRAME_EVENT_QUERY query{ queryElements };
			auto pBlob = std::make_unique<uint8_t[]>(24);
			query.GatherToBlob((const uint8_t*)&frame, pBlob.get());
			Assert::AreEqual(420., *(double*)&pBlob.get()[0]);
			Assert::AreEqual((int)PM_PRESENT_MODE_COMPOSED_FLIP, *(int*)&pBlob.get()[8]);
			Assert::AreEqual((int)PM_GRAPHICS_RUNTIME_DXGI, *(int*)&pBlob.get()[12]);
			Assert::AreEqual(30., *(double*)&pBlob.get()[16]);
		}
		TEST_METHOD(TestQueryQpc)
		{
			const PmNsmFrameData frame{
				.present_event = {
					.PresentStartTime = 69420ull,
					.Runtime = Runtime::DXGI,
					.PresentMode = PresentMode::Composed_Flip,
				},
				.power_telemetry = {
					.gpu_power_w = 420.,
				},
				.cpu_telemetry = {
					.cpu_utilization = 30.,
				},
			};
			PM_QUERY_ELEMENT queryElements[]{
				{ PM_METRIC_GPU_POWER, PM_STAT_NONE, 1, 0 },
				{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_RUNTIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_UTILIZATION, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_QPC, PM_STAT_NONE, 0, 0 },
			};
			PM_FRAME_EVENT_QUERY query{ queryElements };
			auto pBlob = std::make_unique<uint8_t[]>(32);
			query.GatherToBlob((const uint8_t*)&frame, pBlob.get());
			Assert::AreEqual(420., *(double*)&pBlob.get()[0]);
			Assert::AreEqual((int)PM_PRESENT_MODE_COMPOSED_FLIP, *(int*)&pBlob.get()[8]);
			Assert::AreEqual((int)PM_GRAPHICS_RUNTIME_DXGI, *(int*)&pBlob.get()[12]);
			Assert::AreEqual(30., *(double*)&pBlob.get()[16]);
			Assert::AreEqual(69420ull, *(uint64_t*)&pBlob.get()[24]);
		}
	};
}
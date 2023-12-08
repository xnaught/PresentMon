#include "CppUnitTest.h"
#include "../PresentMonUtils/PresentMonNamedPipe.h"
#include "../PresentMonMiddleware/source/FrameEventQuery.h"
#include "../PresentMonMiddleware/source/MockMiddleware.h"

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
			Assert::AreEqual(16ull, query.GetBlobSize());
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
			Assert::AreEqual(24ull, query.GetBlobSize());
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
				{ PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_UTILIZATION, PM_STAT_NONE, 0, 0 }
			};
			PM_FRAME_EVENT_QUERY query{ queryElements };
			auto pBlob = std::make_unique<uint8_t[]>(24);
			Assert::AreEqual(24ull, query.GetBlobSize());
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
				{ PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
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
			Assert::AreEqual(32ull, query.GetBlobSize());
		}
		TEST_METHOD(TestQueryArray)
		{
			const PmNsmFrameData frame{
				.present_event = {
					.PresentStartTime = 69420ull,
					.Runtime = Runtime::DXGI,
					.PresentMode = PresentMode::Composed_Flip,
				},
				.power_telemetry = {
					.gpu_power_w = 420.,
					.fan_speed_rpm = { 1.1, 2.2, 3.3, 4.4, 5.5 },
				},
				.cpu_telemetry = {
					.cpu_utilization = 30.,
				},
			};
			PM_QUERY_ELEMENT queryElements[]{
				{ PM_METRIC_GPU_POWER, PM_STAT_NONE, 1, 0 },
				{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_UTILIZATION, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_QPC, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_GPU_FAN_SPEED, PM_STAT_NONE, 1, 1 },
				{ PM_METRIC_GPU_FAN_SPEED, PM_STAT_NONE, 1, 4 },
			};
			PM_FRAME_EVENT_QUERY query{ queryElements };
			auto pBlob = std::make_unique<uint8_t[]>(48);
			query.GatherToBlob((const uint8_t*)&frame, pBlob.get());
			Assert::AreEqual(420., *(double*)&pBlob.get()[0]);
			Assert::AreEqual((int)PM_PRESENT_MODE_COMPOSED_FLIP, *(int*)&pBlob.get()[8]);
			Assert::AreEqual((int)PM_GRAPHICS_RUNTIME_DXGI, *(int*)&pBlob.get()[12]);
			Assert::AreEqual(30., *(double*)&pBlob.get()[16]);
			Assert::AreEqual(69420ull, *(uint64_t*)&pBlob.get()[24]);
			Assert::AreEqual(2.2, *(double*)&pBlob.get()[32]);
			Assert::AreEqual(5.5, *(double*)&pBlob.get()[40]);
			Assert::AreEqual(48ull, query.GetBlobSize());
		}
		TEST_METHOD(TestQueryBoolFlag)
		{
			const PmNsmFrameData frame{
				.present_event = {
					.PresentStartTime = 69420ull,
					.Runtime = Runtime::DXGI,
					.PresentMode = PresentMode::Composed_Flip,
				},
				.power_telemetry = {
					.gpu_power_w = 420.,
					.fan_speed_rpm = { 1.1, 2.2, 3.3, 4.4, 5.5 },
					.gpu_temperature_limited = true,
				},
				.cpu_telemetry = {
					.cpu_utilization = 30.,
				},
			};
			PM_QUERY_ELEMENT queryElements[]{
				{ PM_METRIC_GPU_POWER, PM_STAT_NONE, 1, 0 },
				{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_UTILIZATION, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_QPC, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_GPU_FAN_SPEED, PM_STAT_NONE, 1, 1 },
				{ PM_METRIC_GPU_FAN_SPEED, PM_STAT_NONE, 1, 4 },
				{ PM_METRIC_GPU_TEMPERATURE_LIMITED, PM_STAT_NONE, 1, 0 },
			};
			PM_FRAME_EVENT_QUERY query{ queryElements };
			auto pBlob = std::make_unique<uint8_t[]>(49);
			Assert::AreEqual(49ull, query.GetBlobSize());
			query.GatherToBlob((const uint8_t*)&frame, pBlob.get());
			Assert::AreEqual(420., *(double*)&pBlob.get()[0]);
			Assert::AreEqual((int)PM_PRESENT_MODE_COMPOSED_FLIP, *(int*)&pBlob.get()[8]);
			Assert::AreEqual((int)PM_GRAPHICS_RUNTIME_DXGI, *(int*)&pBlob.get()[12]);
			Assert::AreEqual(30., *(double*)&pBlob.get()[16]);
			Assert::AreEqual(69420ull, *(uint64_t*)&pBlob.get()[24]);
			Assert::AreEqual(2.2, *(double*)&pBlob.get()[32]);
			Assert::AreEqual(5.5, *(double*)&pBlob.get()[40]);
			Assert::AreEqual(true, *(bool*)&pBlob.get()[48]);
		}
		TEST_METHOD(TestQueryMiddleware)
		{
			PM_QUERY_ELEMENT queryElements[]{
				{ PM_METRIC_GPU_POWER, PM_STAT_NONE, 1, 0 },
				{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_CPU_UTILIZATION, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_PRESENT_QPC, PM_STAT_NONE, 0, 0 },
				{ PM_METRIC_GPU_FAN_SPEED, PM_STAT_NONE, 1, 1 },
				{ PM_METRIC_GPU_FAN_SPEED, PM_STAT_NONE, 1, 4 },
				{ PM_METRIC_GPU_TEMPERATURE_LIMITED, PM_STAT_NONE, 1, 0 },
			};
			pmon::mid::MockMiddleware mid{ true };

			uint32_t blobSize = 0;
			auto pQuery = mid.RegisterFrameEventQuery(queryElements, blobSize);
			Assert::AreEqual(uint32_t(49), blobSize);
			auto pBlob = std::make_unique<uint8_t[]>(blobSize);
			uint32_t nFrames = 1;
			mid.ConsumeFrameEvents(pQuery, 111, pBlob.get(), nFrames);

			Assert::AreEqual(420., *(double*)&pBlob.get()[0]);
			Assert::AreEqual((int)PM_PRESENT_MODE_COMPOSED_FLIP, *(int*)&pBlob.get()[8]);
			Assert::AreEqual((int)PM_GRAPHICS_RUNTIME_DXGI, *(int*)&pBlob.get()[12]);
			Assert::AreEqual(30., *(double*)&pBlob.get()[16]);
			Assert::AreEqual(69420ull, *(uint64_t*)&pBlob.get()[24]);
			Assert::AreEqual(2.2, *(double*)&pBlob.get()[32]);
			Assert::AreEqual(5.5, *(double*)&pBlob.get()[40]);
			Assert::AreEqual(true, *(bool*)&pBlob.get()[48]);

			mid.FreeFrameEventQuery(pQuery);
		}
	};
}
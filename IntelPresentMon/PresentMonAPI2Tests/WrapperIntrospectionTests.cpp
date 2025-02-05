#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "BoostProcess.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPI2Loader/Loader.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process;
using namespace std::string_literals;
using namespace std::chrono_literals;
namespace rn = std::ranges;
namespace vi = rn::views;
using namespace pmapi;

namespace EndToEndTests
{
	TEST_CLASS(WrapperServiceTests)
	{
		std::string ctlPipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
		std::string introName = "PM_intro_test_nsm_2"s;
		std::optional<boost::process::child> oChild;
		std::optional<Session> oSession;
		std::shared_ptr<intro::Root> pData;
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
			if (pData) {
				pData.reset();
			}
			if (oSession) {
				oSession.reset();
			}
			if (oChild) {
				oChild->terminate();
				oChild->wait();
				oChild.reset();
			}
		}
		TEST_METHOD_INITIALIZE(Init)
		{
			oChild.emplace("PresentMonService.exe"s,
				//"--timed-stop"s, "4000"s,
				"--control-pipe"s, ctlPipeName.c_str(),
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName);

			std::this_thread::sleep_for(100ms);

			pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
			pmConfigureStandaloneLogging_();
			oSession.emplace(ctlPipeName, introName);

			pData = oSession->GetIntrospectionRoot();
		}

		TEST_METHOD(IntrospectMetaEnumFullSet)
		{
			// Question: why are these not like "PM_ENUM_STATUS"?
			const std::vector expected{
				"PM_STATUS"s,
				"PM_METRIC"s,
				"PM_METRIC_TYPE"s,
				"PM_DEVICE_VENDOR"s,
				"PM_PRESENT_MODE"s,
				"PM_UNIT"s,
				"PM_STAT"s,
				"PM_DATA_TYPE"s,
				"PM_GRAPHICS_RUNTIME"s,
				"PM_FRAME_TYPE"s,
				"PM_DEVICE_TYPE"s,
				"PM_METRIC_AVAILABILITY"s,
				"PM_NULL_ENUM"s,
			};
			for (auto&& [expected, actual] : vi::zip(expected, pData->GetEnums())) {
				Assert::AreEqual(expected, actual.GetSymbol());
			}
		}
		TEST_METHOD(IntrospectEnumKeyBySymbols)
		{
			auto unit = rn::find(pData->GetEnums(), "PM_UNIT"s, &intro::EnumView::GetSymbol);
			auto millisec = rn::find(unit->GetKeys(), "PM_UNIT_MILLISECONDS"s, &intro::EnumKeyView::GetSymbol);
			Assert::AreEqual("ms"s, millisec->GetShortName());
		}
		TEST_METHOD(IntrospectUnitById)
		{
			for (auto u : pData->GetUnits()) {
				auto id = u->GetId();
				int x = 0;
			}
			auto fps = rn::find(pData->GetUnits(), PM_UNIT_FPS, &intro::UnitView::GetId);
			Assert::IsFalse(fps == pData->GetUnits().end());
			Assert::AreEqual("fps"s, fps->Introspect()->GetShortName());
		}
		TEST_METHOD(IntrospectMetricByName)
		{
			auto gpuName = rn::find_if(pData->GetMetrics(),
				[](auto&& e) { return e.GetName() == "GPU Name"s; },
				&intro::MetricView::Introspect);
			Assert::AreEqual((int)PM_METRIC_GPU_NAME, (int)gpuName->GetId());
		}
		//TEST_METHOD(IntrospectMetricStatsWithLookup)
		//{
		//	using namespace std::string_literals;
		//	Assert::AreEqual("avg"s, data->GetMetrics().begin()[1].GetStatInfo().begin()->IntrospectStat()->GetShortName());
		//}
		//TEST_METHOD(IntrospectMetricDataTypeEnum)
		//{
		//	using namespace std::string_literals;
		//	auto metric = data->FindMetric(PM_METRIC_PRESENT_MODE);
		//	auto type = metric.GetDataTypeInfo();
		//	Assert::AreEqual("Present Mode"s, metric.Introspect().GetName());
		//	Assert::AreEqual((int)PM_DATA_TYPE_ENUM, (int)type.GetPolledType());
		//	Assert::AreEqual((int)PM_DATA_TYPE_ENUM, (int)type.GetFrameType());
		//	Assert::AreEqual("PM_PRESENT_MODE"s, type.IntrospectEnum().GetSymbol());
		//}
		//TEST_METHOD(IntrospectMetricDataTypeDivergent)
		//{
		//	using namespace std::string_literals;
		//	auto metric = data->FindMetric(PM_METRIC_GPU_POWER_LIMITED);
		//	auto type = metric.GetDataTypeInfo();
		//	Assert::AreEqual("GPU Power Limited"s, metric.Introspect().GetName());
		//	Assert::AreEqual((int)PM_DATA_TYPE_DOUBLE, (int)type.GetPolledType());
		//	Assert::AreEqual((int)PM_DATA_TYPE_BOOL, (int)type.GetFrameType());
		//}
		//TEST_METHOD(IntrospectMetricDeviceMetricInfo)
		//{
		//	using namespace std::string_literals;
		//	auto metric = data->FindMetric(PM_METRIC_GPU_FAN_SPEED);
		//	auto deviceInfos = metric.GetDeviceMetricInfo();
		//	Assert::AreEqual(2ull, deviceInfos.size());
		//	{
		//		auto deviceInfo = deviceInfos.begin()[0];
		//		Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)deviceInfo.GetAvailability());
		//		Assert::AreEqual(1u, deviceInfo.GetArraySize());
		//	}
		//	{
		//		auto deviceInfo = deviceInfos.begin()[1];
		//		Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)deviceInfo.GetAvailability());
		//		Assert::AreEqual(2u, deviceInfo.GetArraySize());
		//	}
		//}
		//TEST_METHOD(IntrospectDevices)
		//{
		//	using namespace std::string_literals;
		//	auto devices = data->GetDevices();
		//	Assert::AreEqual(3ull, devices.size());
		//	{
		//		auto device = devices.begin()[0];
		//		Assert::AreEqual("Unknown"s, device.IntrospectVendor().GetName());
		//		Assert::AreEqual("Device-independent"s, device.GetName());
		//		Assert::AreEqual("Device Independent"s, device.IntrospectType().GetName());
		//		Assert::AreEqual(0u, device.GetId());
		//	}
		//	{
		//		auto device = devices.begin()[1];
		//		Assert::AreEqual("Intel"s, device.IntrospectVendor().GetName());
		//		Assert::AreEqual("Arc 750"s, device.GetName());
		//		Assert::AreEqual("Graphics Adapter"s, device.IntrospectType().GetName());
		//		Assert::AreEqual(1u, device.GetId());
		//	}
		//	{
		//		auto device = devices.begin()[2];
		//		Assert::AreEqual("NVIDIA"s, device.IntrospectVendor().GetName());
		//		Assert::AreEqual("GeForce RTX 2080 ti"s, device.GetName());
		//		Assert::AreEqual("Graphics Adapter"s, device.IntrospectType().GetName());
		//		Assert::AreEqual(2u, device.GetId());
		//	}
		//}
		//TEST_METHOD(IntrospectLookupDevice)
		//{
		//	using namespace std::string_literals;
		//	{
		//		auto device = data->FindDevice(1);
		//		Assert::AreEqual("Intel"s, device.IntrospectVendor().GetName());
		//		Assert::AreEqual("Arc 750"s, device.GetName());
		//		Assert::AreEqual("Graphics Adapter"s, device.IntrospectType().GetName());
		//		Assert::AreEqual(1u, device.GetId());
		//	}
		//}
		//TEST_METHOD(IntrospectMetricDeviceMetricInfoLookupDevice)
		//{
		//	using namespace std::string_literals;
		//	auto metric = data->FindMetric(PM_METRIC_GPU_FAN_SPEED);
		//	auto deviceInfos = metric.GetDeviceMetricInfo();
		//	Assert::AreEqual(2ull, deviceInfos.size());
		//	{
		//		auto deviceInfo = deviceInfos.begin()[1];
		//		Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)deviceInfo.GetAvailability());
		//		Assert::IsTrue(deviceInfo.IsAvailable());
		//		Assert::AreEqual(2u, deviceInfo.GetArraySize());
		//		Assert::AreEqual("NVIDIA"s, deviceInfo.GetDevice().IntrospectVendor().GetName());
		//	}
		//}
		//TEST_METHOD(IntrospectMetricLookupError)
		//{
		//	using namespace std::string_literals;
		//	Assert::ExpectException<pmapi::LookupException>([this] {
		//		data->FindMetric((PM_METRIC)420);
		//		});
		//}
		//TEST_METHOD(IntrospectMetricType)
		//{
		//	using namespace std::string_literals;

		//	Assert::AreEqual("Dynamic and Frame Event Metric"s, data->FindMetric(PM_METRIC_CPU_UTILIZATION).IntrospectType().GetName());
		//	Assert::AreEqual("Static Metric"s, data->FindMetric(PM_METRIC_APPLICATION).IntrospectType().GetName());
		//}
		//TEST_METHOD(IntrospectStat)
		//{
		//	using namespace std::string_literals;

		//	auto shortName = data->FindEnumKey(PM_ENUM_STAT, PM_STAT_AVG).GetShortName();

		//	Assert::AreEqual("avg"s, shortName);
		//}
		//TEST_METHOD(IntrospectPreferredUnit)
		//{
		//	using namespace std::string_literals;

		//	auto metric = data->FindMetric(PM_METRIC_GPU_MEM_SIZE);
		//	Assert::AreEqual((int)PM_UNIT_BYTES, (int)metric.GetUnit());
		//	Assert::AreEqual((int)PM_UNIT_GIGABYTES, (int)metric.GetPreferredUnitHint());
		//}
		//TEST_METHOD(IntrospectUnitUpConversion)
		//{
		//	using namespace std::string_literals;

		//	Assert::AreEqual(0.000'001, data->FindUnit(PM_UNIT_HERTZ).MakeConversionFactor(PM_UNIT_MEGAHERTZ));
		//}
	};
}
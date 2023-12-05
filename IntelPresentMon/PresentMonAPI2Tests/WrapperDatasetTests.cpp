#include "CppUnitTest.h"
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include "../PresentMonAPI2/source/Internal.h"
#include <cstring>
#include <vector>
#include <optional>
#include "Utilities.h"

#include "../PresentMonAPIWrapper/source/PresentMonAPIWrapper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2Mock
{
	TEST_CLASS(WrapperDatasetTests)
	{
	public:
		TEST_METHOD_INITIALIZE(BeforeEachTestMethod)
		{
			pmSetMiddlewareAsMock_(true, true);
			session.emplace();
			data = session->GetIntrospectionDataset();
		}
		TEST_METHOD_CLEANUP(AfterEachTestMethod)
		{
			data.reset();
			session.reset();
		}
		TEST_METHOD(IntrospectRootRange)
		{
			using namespace std::string_literals;

			const std::vector expected{
				"PM_STATUS"s, "PM_METRIC"s, "PM_METRIC_TYPE"s, "PM_DEVICE_VENDOR"s, "PM_PRESENT_MODE"s, "PM_PSU_TYPE"s,
				"PM_UNIT"s, "PM_STAT"s, "PM_DATA_TYPE"s, "PM_GRAPHICS_RUNTIME"s, "PM_DEVICE_TYPE"s, "PM_METRIC_AVAILABILITY"s
			};
			auto e = expected.begin();
			for (auto ev : data->GetEnums()) {
				Assert::AreEqual(*e, ev.GetSymbol());
				e++;
			}
		}
		TEST_METHOD(IntrospectViewRange)
		{
			using namespace std::string_literals;

			const std::vector expected{
				"PM_STATUS_SUCCESS"s,
				"PM_STATUS_FAILURE"s,
				"PM_STATUS_SESSION_NOT_OPEN"s,
			};
			auto e = expected.begin();
			for (auto kv : data->GetEnums().begin()->GetKeys()) {
				Assert::AreEqual(*e, kv.GetSymbol());
				e++;
			}
		}
		TEST_METHOD(IntrospectMetricToEnumKey)
		{
			using namespace std::string_literals;
			Assert::AreEqual("Displayed FPS"s, data->GetMetrics().begin()->GetMetricKey().GetName());
		}
		TEST_METHOD(IntrospectMetricUnit)
		{
			using namespace std::string_literals;
			Assert::AreEqual("fps"s, data->GetMetrics().begin()->GetUnit().GetShortName());
		}
		TEST_METHOD(IntrospectMetricStats)
		{
			using namespace std::string_literals;
			Assert::AreEqual("Average"s, data->GetMetrics().begin()->GetStatInfo().begin()->GetStat()->GetName());
			Assert::AreEqual(7ull, data->GetMetrics().begin()->GetStatInfo().size());
		}
		TEST_METHOD(IntrospectMetricStatsWithLookup)
		{
			using namespace std::string_literals;
			Assert::AreEqual("avg"s, data->GetMetrics().begin()->GetStatInfo().begin()->GetStat()->GetShortName());
		}
		TEST_METHOD(IntrospectMetricDataType)
		{
			using namespace std::string_literals;
			auto metric = data->FindMetric(PM_METRIC_PRESENT_MODE);
			auto type = metric.GetDataTypeInfo();
			Assert::AreEqual("Present Mode"s, metric.GetMetricKey().GetName());
			Assert::AreEqual("PM_PRESENT_MODE"s, type.GetEnum().GetSymbol());
		}
		TEST_METHOD(IntrospectMetricDeviceMetricInfo)
		{
			using namespace std::string_literals;
			auto metric = data->FindMetric(PM_METRIC_GPU_FAN_SPEED);
			auto deviceInfos = metric.GetDeviceMetricInfo();
			Assert::AreEqual(2ull, deviceInfos.size());
			{
				auto deviceInfo = deviceInfos.begin()[0];
				Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, deviceInfo.GetAvailablity().GetValue());
				Assert::AreEqual(1u, deviceInfo.GetArraySize());
			}
			{
				auto deviceInfo = deviceInfos.begin()[1];
				Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, deviceInfo.GetAvailablity().GetValue());
				Assert::AreEqual(2u, deviceInfo.GetArraySize());
			}
		}
		TEST_METHOD(IntrospectDevices)
		{
			using namespace std::string_literals;
			auto devices = data->GetDevices();
			Assert::AreEqual(3ull, devices.size());
			{
				auto device = devices.begin()[0];
				Assert::AreEqual("Unknown"s, device.GetVendor().GetName());
				Assert::AreEqual("Device-independent"s, device.GetName());
				Assert::AreEqual("Device Independent"s, device.GetType().GetName());
				Assert::AreEqual(0u, device.GetId());
			}
			{
				auto device = devices.begin()[1];
				Assert::AreEqual("Intel"s, device.GetVendor().GetName());
				Assert::AreEqual("Arc 750"s, device.GetName());
				Assert::AreEqual("Graphics Adapter"s, device.GetType().GetName());
				Assert::AreEqual(1u, device.GetId());
			}
			{
				auto device = devices.begin()[2];
				Assert::AreEqual("NVIDIA"s, device.GetVendor().GetName());
				Assert::AreEqual("GeForce RTX 2080 ti"s, device.GetName());
				Assert::AreEqual("Graphics Adapter"s, device.GetType().GetName());
				Assert::AreEqual(2u, device.GetId());
			}
		}
		TEST_METHOD(IntrospectLookupDevice)
		{
			using namespace std::string_literals;
			{
				auto device = data->FindDevice(1);
				Assert::AreEqual("Intel"s, device.GetVendor().GetName());
				Assert::AreEqual("Arc 750"s, device.GetName());
				Assert::AreEqual("Graphics Adapter"s, device.GetType().GetName());
				Assert::AreEqual(1u, device.GetId());
			}
		}
		TEST_METHOD(IntrospectMetricDeviceMetricInfoLookupDevice)
		{
			using namespace std::string_literals;
			auto metric = data->FindMetric(PM_METRIC_GPU_FAN_SPEED);
			auto deviceInfos = metric.GetDeviceMetricInfo();
			Assert::AreEqual(2ull, deviceInfos.size());
			{
				auto deviceInfo = deviceInfos.begin()[1];
				Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, deviceInfo.GetAvailablity().GetValue());
				Assert::IsTrue(deviceInfo.IsAvailable());
				Assert::AreEqual(2u, deviceInfo.GetArraySize());
				Assert::AreEqual("NVIDIA"s, deviceInfo.GetDevice().GetVendor().GetName());
			}
		}
		TEST_METHOD(IntrospectMetricLookupError)
		{
			using namespace std::string_literals;
			Assert::ExpectException<pmapi::LookupException>([this] {
				data->FindMetric((PM_METRIC)420);
				});
		}
		TEST_METHOD(IntrospectDatatypeError)
		{
			using namespace std::string_literals;
			Assert::ExpectException<pmapi::DatatypeException>([this] {
				data->FindMetric(PM_METRIC_FRAME_TIME).GetDataTypeInfo().GetEnum();
			});
		}
		TEST_METHOD(IntrospectMetricType)
		{
			using namespace std::string_literals;

			Assert::AreEqual("Dynamic Metric"s, data->FindMetric(PM_METRIC_CPU_UTILIZATION).GetType().GetName());
			Assert::AreEqual("Static Metric"s, data->FindMetric(PM_METRIC_PROCESS_NAME).GetType().GetName());
		}
	private:
		std::optional<pmapi::Session> session;
		std::optional<pmapi::intro::Dataset> data;
	};
}

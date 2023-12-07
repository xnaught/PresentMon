#include "CppUnitTest.h"
#include <cstring>
#include <vector>
#include <optional>
#include "Utilities.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/IntrospectionTransfer.h"
#include "../Interprocess/source/IntrospectionCloneAllocators.h"
#include "../PresentMonAPIWrapper/source/PresentMonAPIWrapper.h"
#include "../PresentMonAPI2/source/Internal.h"
#include "BoostProcess.h"
#include "../PresentMonMiddleware/source/MockCommon.h"
#include "../PresentMonMiddleware/source/MockMiddleware.h"
#include "../CommonUtilities/source/Memory.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2Mock
{
	using namespace pmon;
	TEST_CLASS(InterprocessTests)
	{
	public:
		TEST_METHOD(ApiProbeCloneSize)
		{
			auto pComm = ipc::MakeServiceComms("svc_comms_shm_1");
			ipc::intro::RegisterMockIntrospectionDevices(*pComm);
			auto& root = pComm->GetIntrospectionRoot();
			ipc::intro::ProbeAllocator<void> alloc;
			auto pClone = root.ApiClone(alloc);
			Assert::AreEqual(17004ull, alloc.GetTotalSize());
			Assert::IsNull(pClone);
			free((void*)pClone);
		}
		TEST_METHOD(Padding)
		{
			Assert::AreEqual(4ull, util::GetPadding<void*>(4ull));
			Assert::AreEqual(3ull, util::GetPadding<PM_ENUM>(41ull));
			Assert::AreEqual(1ull, util::GetPadding<uint32_t>(3ull));
		}
		TEST_METHOD(ApiBlockClone)
		{
			auto pComm = ipc::MakeServiceComms("svc_comms_shm_2");
			ipc::intro::RegisterMockIntrospectionDevices(*pComm);
			auto& root = pComm->GetIntrospectionRoot();
			ipc::intro::ProbeAllocator<void> probeAlloc;
			auto pNullClone = root.ApiClone(probeAlloc);
			Assert::AreEqual(17004ull, probeAlloc.GetTotalSize());
			Assert::IsNull(pNullClone);

			ipc::intro::BlockAllocator<void> blockAlloc{ probeAlloc.GetTotalSize() };
			auto pRoot = root.ApiClone(blockAlloc);

			Assert::IsNotNull(pRoot);
			Assert::AreEqual(12ull, pRoot->pEnums->size);
			Assert::AreEqual(14ull, pRoot->pMetrics->size);
			Assert::AreEqual(3ull, pRoot->pDevices->size);

			// checking 7th enum (unit)
			{
				auto pEnum = static_cast<const PM_INTROSPECTION_ENUM*>(pRoot->pEnums->pData[6]);
				Assert::IsNotNull(pEnum);
				Assert::AreEqual((int)PM_ENUM_UNIT, (int)pEnum->id);
				Assert::AreEqual("PM_UNIT", pEnum->pSymbol->pData);
				Assert::AreEqual("List of all units of measure used for metrics", pEnum->pDescription->pData);
				Assert::AreEqual(13ull, pEnum->pKeys->size);
				// 1st key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[0]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_UNIT_DIMENSIONLESS", pKey->pSymbol->pData);
					Assert::AreEqual("Dimensionless", pKey->pName->pData);
					Assert::AreEqual("", pKey->pShortName->pData);
					Assert::AreEqual((int)PM_ENUM_UNIT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_UNIT_DIMENSIONLESS, pKey->value);
				}
				// 5th key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[4]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_UNIT_PERCENT", pKey->pSymbol->pData);
					Assert::AreEqual("Percent", pKey->pName->pData);
					Assert::AreEqual("%", pKey->pShortName->pData);
					Assert::AreEqual((int)PM_ENUM_UNIT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_UNIT_PERCENT, pKey->value);
				}
			}

			// check device
			{
				auto pDevice = static_cast<const PM_INTROSPECTION_DEVICE*>(pRoot->pDevices->pData[0]);
				Assert::IsNotNull(pDevice);
				Assert::AreEqual(0, (int)pDevice->id);
				Assert::AreEqual((int)PM_DEVICE_TYPE_INDEPENDENT, (int)pDevice->type);
				Assert::AreEqual((int)PM_DEVICE_VENDOR_UNKNOWN, (int)pDevice->vendor);
				Assert::AreEqual("Device-independent", pDevice->pName->pData);
			}

			// check metric 1st
			{
				auto pMetric = static_cast<const PM_INTROSPECTION_METRIC*>(pRoot->pMetrics->pData[0]);
				Assert::IsNotNull(pMetric);
				Assert::AreEqual((int)PM_METRIC_DISPLAYED_FPS, (int)pMetric->id);
				Assert::AreEqual((int)PM_UNIT_FPS, (int)pMetric->unit);
				Assert::AreEqual((int)PM_DATA_TYPE_DOUBLE, (int)pMetric->pTypeInfo->type);
				Assert::AreEqual(7ull, pMetric->pStatInfo->size);
				// check 1st stat
				{
					auto pStatInfo = static_cast<const PM_INTROSPECTION_STAT_INFO*>(pMetric->pStatInfo->pData[0]);
					Assert::AreEqual((int)PM_STAT_AVG, (int)pStatInfo->stat);
				}
				// check device info
				Assert::AreEqual(1ull, pMetric->pDeviceMetricInfo->size);
				{
					auto pInfo = static_cast<const PM_INTROSPECTION_DEVICE_METRIC_INFO*>(pMetric->pDeviceMetricInfo->pData[0]);
					Assert::AreEqual(0u, pInfo->deviceId);
					Assert::AreEqual(1u, pInfo->arraySize);
					Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)pInfo->availability);
				}
			}

			// check metric gpu array 2 device (fan)
			{
				auto pMetric = static_cast<const PM_INTROSPECTION_METRIC*>(pRoot->pMetrics->pData[6]);
				Assert::IsNotNull(pMetric);
				Assert::AreEqual((int)PM_METRIC_GPU_FAN_SPEED, (int)pMetric->id);
				Assert::AreEqual((int)PM_UNIT_RPM, (int)pMetric->unit);
				Assert::AreEqual((int)PM_DATA_TYPE_DOUBLE, (int)pMetric->pTypeInfo->type);
				Assert::AreEqual(7ull, pMetric->pStatInfo->size);
				// check 7th stat
				{
					auto pStatInfo = static_cast<const PM_INTROSPECTION_STAT_INFO*>(pMetric->pStatInfo->pData[6]);
					Assert::AreEqual((int)PM_STAT_RAW, (int)pStatInfo->stat);
				}
				// check device infos
				Assert::AreEqual(2ull, pMetric->pDeviceMetricInfo->size);
				{
					auto pInfo = static_cast<const PM_INTROSPECTION_DEVICE_METRIC_INFO*>(pMetric->pDeviceMetricInfo->pData[0]);
					Assert::AreEqual(1u, pInfo->deviceId);
					Assert::AreEqual(1u, pInfo->arraySize);
					Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)pInfo->availability);
				}
				{
					auto pInfo = static_cast<const PM_INTROSPECTION_DEVICE_METRIC_INFO*>(pMetric->pDeviceMetricInfo->pData[1]);
					Assert::AreEqual(2u, pInfo->deviceId);
					Assert::AreEqual(2u, pInfo->arraySize);
					Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)pInfo->availability);
				}
			}
			free((void*)pRoot);
		}
		TEST_METHOD(SeparateProcessesApiBlockClone)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto introNsm = "intro_shm_test_1"s;

			bp::child process("InterprocessMock.exe"s,
				"--basic-intro"s,
				"--intro-nsm"s, introNsm,
				bp::std_out > out, bp::std_in < in);

			std::string output;
			out >> output;

			Assert::AreEqual("ready"s, output);

			auto pComm = ipc::MakeMiddlewareComms(introNsm);
			auto pRoot = pComm->GetIntrospectionRoot();

			Assert::IsNotNull(pRoot);
			Assert::AreEqual(12ull, pRoot->pEnums->size);
			Assert::AreEqual(14ull, pRoot->pMetrics->size);
			Assert::AreEqual(3ull, pRoot->pDevices->size);

			// checking 7th enum (unit)
			{
				auto pEnum = static_cast<const PM_INTROSPECTION_ENUM*>(pRoot->pEnums->pData[6]);
				Assert::IsNotNull(pEnum);
				Assert::AreEqual((int)PM_ENUM_UNIT, (int)pEnum->id);
				Assert::AreEqual("PM_UNIT", pEnum->pSymbol->pData);
				Assert::AreEqual("List of all units of measure used for metrics", pEnum->pDescription->pData);
				Assert::AreEqual(13ull, pEnum->pKeys->size);
				// 1st key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[0]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_UNIT_DIMENSIONLESS", pKey->pSymbol->pData);
					Assert::AreEqual("Dimensionless", pKey->pName->pData);
					Assert::AreEqual("", pKey->pShortName->pData);
					Assert::AreEqual((int)PM_ENUM_UNIT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_UNIT_DIMENSIONLESS, pKey->value);
				}
				// 5th key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[4]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_UNIT_PERCENT", pKey->pSymbol->pData);
					Assert::AreEqual("Percent", pKey->pName->pData);
					Assert::AreEqual("%", pKey->pShortName->pData);
					Assert::AreEqual((int)PM_ENUM_UNIT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_UNIT_PERCENT, pKey->value);
				}
			}

			// check device
			{
				auto pDevice = static_cast<const PM_INTROSPECTION_DEVICE*>(pRoot->pDevices->pData[0]);
				Assert::IsNotNull(pDevice);
				Assert::AreEqual(0, (int)pDevice->id);
				Assert::AreEqual((int)PM_DEVICE_TYPE_INDEPENDENT, (int)pDevice->type);
				Assert::AreEqual((int)PM_DEVICE_VENDOR_UNKNOWN, (int)pDevice->vendor);
				Assert::AreEqual("Device-independent", pDevice->pName->pData);
			}

			// check metric 1st
			{
				auto pMetric = static_cast<const PM_INTROSPECTION_METRIC*>(pRoot->pMetrics->pData[0]);
				Assert::IsNotNull(pMetric);
				Assert::AreEqual((int)PM_METRIC_DISPLAYED_FPS, (int)pMetric->id);
				Assert::AreEqual((int)PM_UNIT_FPS, (int)pMetric->unit);
				Assert::AreEqual((int)PM_DATA_TYPE_DOUBLE, (int)pMetric->pTypeInfo->type);
				Assert::AreEqual(7ull, pMetric->pStatInfo->size);
				// check 1st stat
				{
					auto pStatInfo = static_cast<const PM_INTROSPECTION_STAT_INFO*>(pMetric->pStatInfo->pData[0]);
					Assert::AreEqual((int)PM_STAT_AVG, (int)pStatInfo->stat);
				}
				// check device info
				Assert::AreEqual(1ull, pMetric->pDeviceMetricInfo->size);
				{
					auto pInfo = static_cast<const PM_INTROSPECTION_DEVICE_METRIC_INFO*>(pMetric->pDeviceMetricInfo->pData[0]);
					Assert::AreEqual(0u, pInfo->deviceId);
					Assert::AreEqual(1u, pInfo->arraySize);
					Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)pInfo->availability);
				}
			}

			// check metric gpu array 2 device (fan)
			{
				auto pMetric = static_cast<const PM_INTROSPECTION_METRIC*>(pRoot->pMetrics->pData[6]);
				Assert::IsNotNull(pMetric);
				Assert::AreEqual((int)PM_METRIC_GPU_FAN_SPEED, (int)pMetric->id);
				Assert::AreEqual((int)PM_UNIT_RPM, (int)pMetric->unit);
				Assert::AreEqual((int)PM_DATA_TYPE_DOUBLE, (int)pMetric->pTypeInfo->type);
				Assert::AreEqual(7ull, pMetric->pStatInfo->size);
				// check 7th stat
				{
					auto pStatInfo = static_cast<const PM_INTROSPECTION_STAT_INFO*>(pMetric->pStatInfo->pData[6]);
					Assert::AreEqual((int)PM_STAT_RAW, (int)pStatInfo->stat);
				}
				// check device infos
				Assert::AreEqual(2ull, pMetric->pDeviceMetricInfo->size);
				{
					auto pInfo = static_cast<const PM_INTROSPECTION_DEVICE_METRIC_INFO*>(pMetric->pDeviceMetricInfo->pData[0]);
					Assert::AreEqual(1u, pInfo->deviceId);
					Assert::AreEqual(1u, pInfo->arraySize);
					Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)pInfo->availability);
				}
				{
					auto pInfo = static_cast<const PM_INTROSPECTION_DEVICE_METRIC_INFO*>(pMetric->pDeviceMetricInfo->pData[1]);
					Assert::AreEqual(2u, pInfo->deviceId);
					Assert::AreEqual(2u, pInfo->arraySize);
					Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)pInfo->availability);
				}
			}
			free((void*)pRoot);

			// ack to companion process so it can exit
			in << "ack" << std::endl;

			process.wait();

			Assert::AreEqual(0, process.exit_code());
		}
		TEST_METHOD(SeparateProcessesWrapperSession)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			bp::child process("InterprocessMock.exe"s,
				"--basic-intro"s,
				"--intro-nsm"s, mid::MockMiddleware::mockIntrospectionNsmName,
				bp::std_out > out, bp::std_in < in);

			std::string output;
			out >> output;

			Assert::AreEqual("ready"s, output);

			pmSetMiddlewareAsMock_(true, true, false);

			const auto heapBefore = pmCreateHeapCheckpoint_();

			{
				pmapi::Session session;
				auto data = session.GetIntrospectionDataset();
			}

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsFalse(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));

			in << "ack" << std::endl;

			process.wait();

			Assert::AreEqual(0, process.exit_code());
		}
		TEST_METHOD(SeparateProcessesWrapperIntrospectionOmnibus)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			bp::child process("InterprocessMock.exe"s,
				"--basic-intro"s,
				"--intro-nsm"s, mid::MockMiddleware::mockIntrospectionNsmName,
				bp::std_out > out, bp::std_in < in);

			std::string output;
			out >> output;

			Assert::AreEqual("ready"s, output);

			pmSetMiddlewareAsMock_(true, false, false);

			{
				pmapi::Session session;
				auto datax = session.GetIntrospectionDataset();
				auto data = &datax;

				{
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
				{
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
				{
					auto metric = data->FindMetric(PM_METRIC_PRESENT_MODE);
					auto type = metric.GetDataTypeInfo();
					Assert::AreEqual("Present Mode"s, metric.GetMetricKey().GetName());
					Assert::AreEqual("PM_PRESENT_MODE"s, type.GetEnum().GetSymbol());
				}
				{
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
				{
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
				{
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
				{
					Assert::AreEqual("Dynamic Metric"s, data->FindMetric(PM_METRIC_CPU_UTILIZATION).GetType().GetName());
					Assert::AreEqual("Static Metric"s, data->FindMetric(PM_METRIC_PROCESS_NAME).GetType().GetName());
				}
			}

			in << "ack" << std::endl;

			process.wait();

			Assert::AreEqual(0, process.exit_code());
		}
	};
}

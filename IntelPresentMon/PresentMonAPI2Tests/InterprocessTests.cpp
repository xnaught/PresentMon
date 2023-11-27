#include "CppUnitTest.h"
#include <cstring>
#include <vector>
#include <optional>
#include "Utilities.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/IntrospectionTransfer.h"
#include "../Interprocess/source/IntrospectionCloneAllocators.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2
{
	using namespace pmon;
	TEST_CLASS(InterprocessTests)
	{
	public:
		TEST_METHOD(ApiProbeCloneSize)
		{
			auto pComm = ipc::MakeServiceComms();
			auto& root = pComm->GetIntrospectionRoot();
			ipc::intro::ProbeAllocator<void> alloc;
			auto pClone = root.ApiClone(alloc);
			Assert::AreEqual(14812ull, alloc.GetTotalSize());
			Assert::IsNull(pClone.get());
		}
		TEST_METHOD(Padding)
		{
			Assert::AreEqual(4ull, ipc::intro::GetPadding<void*>(4ull));
			Assert::AreEqual(3ull, ipc::intro::GetPadding<PM_ENUM>(41ull));
			Assert::AreEqual(1ull, ipc::intro::GetPadding<uint32_t>(3ull));
		}
		TEST_METHOD(ApiBlockClone)
		{
			auto pComm = ipc::MakeServiceComms();
			auto& root = pComm->GetIntrospectionRoot();
			ipc::intro::ProbeAllocator<void> probeAlloc;
			auto pNullClone = root.ApiClone(probeAlloc);
			Assert::AreEqual(14812ull, probeAlloc.GetTotalSize());
			Assert::IsNull(pNullClone.get());

			ipc::intro::BlockAllocator<void> blockAlloc{ probeAlloc.GetTotalSize() };
			auto pRoot = root.ApiClone(blockAlloc);

			Assert::IsNotNull(pRoot.get());
			Assert::AreEqual(12ull, pRoot->pEnums->size);
			Assert::AreEqual(8ull, pRoot->pMetrics->size);
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
		}
	};
}

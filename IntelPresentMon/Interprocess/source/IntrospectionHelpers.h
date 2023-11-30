#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "SharedMemoryTypes.h"
#include "../../ControlLib/PresentMonPowerTelemetry.h"
#include "../../ControlLib/CpuTelemetryInfo.h"

// TODO: forward declare the segment manager (or type erase)

namespace pmon::ipc::intro
{
	size_t GetDataTypeSize(PM_DATA_TYPE v);
	void PopulateEnums(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateMetrics(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateGpuDevice(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root, uint32_t deviceId,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const GpuTelemetryBitset& gpuCaps);
	void PopulateCpu(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const CpuTelemetryBitset& cpuCaps);
}
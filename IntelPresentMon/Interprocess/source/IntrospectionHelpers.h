#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "SharedMemoryTypes.h"

// TODO: forward declare the segment manager (or type erase)

namespace pmon::ipc::intro
{
	size_t GetDataTypeSize(PM_DATA_TYPE v);
	void PopulateEnums(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateDevices(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateMetrics(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
}
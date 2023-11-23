#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"


namespace pmon::ipc::intro
{
	size_t GetDataTypeSize(PM_DATA_TYPE v);
	void PopulateEnums(struct IntrospectionRoot& root);
	void PopulateDevices(struct IntrospectionRoot& root);
	void PopulateMetrics(struct IntrospectionRoot& root);
}
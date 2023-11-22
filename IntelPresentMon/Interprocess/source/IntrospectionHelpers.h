#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"


namespace pmon::ipc::intro
{
	size_t GetDataTypeSize(PM_DATA_TYPE v);
	void PopulateEnums(class IntrospectionRoot& root);
	void PopulateDevices(class IntrospectionRoot& root);
	void PopulateMetrics(class IntrospectionRoot& root);
}
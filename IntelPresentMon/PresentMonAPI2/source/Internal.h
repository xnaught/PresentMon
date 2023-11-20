#pragma once
#include "PresentMonAPI.h"
#include <vector>

struct PM_DYNAMIC_QUERY
{
	std::vector<PM_QUERY_ELEMENT> elements;
	size_t GetBlobSize() const
	{
		return elements.back().dataOffset + elements.back().dataSize;
	}
};

PRESENTMON_API_EXPORT void pmSetMiddlewareAsMock_(bool mocked, bool useCrtHeapDebug = false);
PRESENTMON_API_EXPORT _CrtMemState pmCreateHeapCheckpoint_();
PRESENTMON_API_EXPORT PM_STATUS pmMiddlewareSpeak_(char* buffer);
PRESENTMON_API_EXPORT PM_STATUS pmMiddlewareAdvanceTime_(uint32_t milliseconds);

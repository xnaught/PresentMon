#pragma once
#include "PresentMonAPI.h"
#include <vector>

struct PM_DYNAMIC_QUERY
{
	std::vector<PM_QUERY_ELEMENT> elements;
	uint64_t blobSize = 0;
};
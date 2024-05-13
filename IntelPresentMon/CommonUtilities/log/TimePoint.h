#pragma once
#include <chrono>

namespace pmon::util::log
{
	struct TimePoint
	{
		std::chrono::high_resolution_clock::time_point value = std::chrono::high_resolution_clock::now();
	};
}
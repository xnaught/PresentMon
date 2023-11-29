#pragma once
#include <vector>
#include <bitset>
#include <map>
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../ControlLib/CpuTelemetryInfo.h"
#include "../../ControlLib/PresentMonPowerTelemetry.h"

struct CompiledStats
{
	bool calcAvg;
	bool calcPercentile99;
	bool calcPercentile95;
	bool calcPercentile90;
	bool calcMax;
	bool calcMin;
	bool calcRaw;
};

struct PM_DYNAMIC_QUERY
{
	std::vector<PM_QUERY_ELEMENT> elements;
	size_t GetBlobSize() const
	{
		return elements.back().dataOffset + elements.back().dataSize;
	}
	// Data used to track what should be accumulated
	bool accumFpsData;
	std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)> accumGpuBits;
	std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)> accumCpuBits;
	// Data used to calculate the requested metrics
	std::map<PM_METRIC, CompiledStats> compiledMetrics;
	uint32_t processId;
	double windowSizeMs;
	double metricOffsetMs;
};


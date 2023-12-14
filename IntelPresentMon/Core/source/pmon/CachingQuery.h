#pragma once
#include <PresentMonAPI/PresentMonAPI.h>
#include <PresentMonAPI2/source/PresentMonAPI.h>
#include <span>
#include <memory>
#include <vector>
#include <optional>

namespace pmapi
{
	class Session;
	class DynamicQuery;
	namespace intro
	{
		class Root;
	}
}

namespace p2c::pmon
{
	namespace met
	{
		class DynamicPollingMetric;
	}

	class CachingQuery
	{
	public:
		CachingQuery(pmapi::Session& session, const pmapi::intro::Root& introRoot, uint32_t pid,
			std::span<const met::DynamicPollingMetric*> metricsRequestedPtrs, double winSizeMs, double metricOffsetMs);
		const uint8_t* Poll(double timestamp_);
	private:
		uint32_t pid;
		std::shared_ptr<pmapi::DynamicQuery> pQuery;
		std::vector<std::unique_ptr<met::DynamicPollingMetric>> metricPtrs;
		std::unique_ptr<uint8_t[]> pBlob;
		std::optional<double> timestamp;
	};
}
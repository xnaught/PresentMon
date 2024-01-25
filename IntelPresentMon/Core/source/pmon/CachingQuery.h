#pragma once
#include <PresentMonAPI/PresentMonAPI.h>
#include <PresentMonAPI2/source/PresentMonAPI.h>
#include <PresentMonAPIWrapper/source/BlobContainer.h>
#include <PresentMonAPIWrapper/source/DynamicQuery.h>
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
		class Metric;
		class DynamicPollingMetric;
	}

	class CachingQuery
	{
	public:
		CachingQuery(const pmapi::ProcessTracker& procTracker, double winSizeMs, double metricOffsetMs);
		CachingQuery(const CachingQuery&) = delete;
		CachingQuery& operator=(const CachingQuery&) = delete;
		void AddDynamicMetric(std::unique_ptr<met::DynamicPollingMetric> pMetric);
		void Finalize(pmapi::Session& session);
		const uint8_t* Poll(double timestamp_);
		void Reset();
		size_t GetMetricCount() const;
	private:
		const pmapi::ProcessTracker& procTracker;
		double winSizeMs;
		double metricOffsetMs;
		pmapi::DynamicQuery query;
		std::vector<std::unique_ptr<met::DynamicPollingMetric>> metricPtrs;
		pmapi::BlobContainer blobs;
		std::optional<double> timestamp;
	};
}
#pragma once
#include "CachingQuery.h"
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include "metric/DynamicPollingMetric.h"
#include <ranges>


namespace p2c::pmon
{
	CachingQuery::CachingQuery(const pmapi::ProcessTracker& procTrackerIn, double winSizeMs, double metricOffsetMs)
		:
		procTracker{ procTrackerIn },
		winSizeMs{ winSizeMs },
		metricOffsetMs{ metricOffsetMs }
	{}

	void CachingQuery::AddDynamicMetric(std::unique_ptr<met::DynamicPollingMetric> pMetric)
	{
		metricPtrs.push_back(std::move(pMetric));
	}

	void CachingQuery::Finalize(pmapi::Session& session)
	{
		if (metricPtrs.empty()) {
			return;
		}
		std::vector<PM_QUERY_ELEMENT> queryElements;
		for (auto& pMet : metricPtrs) {
			queryElements.push_back(pMet->MakeQueryElement());
		}
		query = session.RegisterDyanamicQuery(queryElements, winSizeMs, metricOffsetMs);
		for (auto&& [e, m] : std::views::zip(queryElements, metricPtrs)) {
			m->Finalize(uint32_t(e.dataOffset));
		}
		blobs = query.MakeBlobContainer(1u);
	}

	const uint8_t* CachingQuery::Poll(double timestamp_)
	{
		if (!query) {
			return nullptr;
		}
		if (!timestamp || *timestamp != timestamp_) {
			query.Poll(procTracker, blobs);
			timestamp = timestamp_;
		}
		return blobs.GetFirst();
	}
	void CachingQuery::Reset()
	{
		query.Reset();
		metricPtrs.clear();
		blobs.Reset();
		timestamp.reset();
	}
	size_t CachingQuery::GetMetricCount() const
	{
		return metricPtrs.size();
	}
}
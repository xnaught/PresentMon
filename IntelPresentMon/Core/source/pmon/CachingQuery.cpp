#pragma once
#include "CachingQuery.h"
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include "metric/DynamicPollingMetric.h"
#include <ranges>


namespace p2c::pmon
{
	CachingQuery::CachingQuery(pmapi::Session& session, const pmapi::intro::Root& introRoot, uint32_t pid,
		std::span<const met::DynamicPollingMetric*> metricsRequestedPtrs, double winSizeMs, double metricOffsetMs)
		:
		pid{ pid }
	{
		std::vector<PM_QUERY_ELEMENT> queryElements;
		for (auto pReq : metricsRequestedPtrs) {
			queryElements.push_back(pReq->MakeQueryElement());
		}
		pQuery = session.RegisterDyanamicQuery(queryElements, winSizeMs, metricOffsetMs);
		for (auto&& [e, m] : std::views::zip(queryElements, metricsRequestedPtrs)) {
			const auto dataTypeId = introRoot.FindMetric(e.metric).GetDataTypeInfo().GetBasePtr()->polledType;
			std::unique_ptr<met::DynamicPollingMetric> pRealized;
			switch (dataTypeId) {
			case PM_DATA_TYPE_DOUBLE:
				pRealized = std::make_unique<met::TypedDynamicPollingMetric<double>>(*m, this, e.dataOffset);
				break;
			case PM_DATA_TYPE_STRING:
				pRealized = std::make_unique<met::TypedDynamicPollingMetric<char*>>(*m, this, e.dataOffset);
				break;
			}
			metricPtrs.push_back(std::move(pRealized));
		}
		pBlob = std::make_unique<uint8_t[]>(pQuery->GetBlobSize());
	}

	const uint8_t* CachingQuery::Poll(double timestamp_)
	{
		if (!timestamp || *timestamp != timestamp_) {
			uint32_t numSwap = 1;
			pQuery->Poll(pid, pBlob.get(), numSwap);
			timestamp = timestamp_;
		}
		return pBlob.get();
	}
}
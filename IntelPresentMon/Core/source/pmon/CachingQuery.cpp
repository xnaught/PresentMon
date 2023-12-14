#pragma once
#include "CachingQuery.h"
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include "metric/DynamicPollingMetric.h"
#include <ranges>


namespace p2c::pmon
{
	CachingQuery::CachingQuery(uint32_t pid, double winSizeMs, double metricOffsetMs)
		:
		pid{ pid },
		winSizeMs{ winSizeMs },
		metricOffsetMs{ metricOffsetMs }
	{}

	met::Metric* CachingQuery::AddDynamicMetric(const pmapi::intro::Root& introRoot, const met::DynamicPollingMetric& requestedMetric, uint32_t activeGpuId)
	{
		namespace rn = std::ranges;
		const auto metricId = requestedMetric.MakeQueryElement().metric;
		const auto metricIntro = introRoot.FindMetric(metricId);
		const auto dataTypeId = metricIntro.GetDataTypeInfo().GetBasePtr()->polledType;
		const bool isGpuMetric = rn::any_of(metricIntro.GetDeviceMetricInfo(), [](auto&& info) {
			return info.GetDevice().GetBasePtr()->type == PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
			});
		const auto deviceId = isGpuMetric ? activeGpuId : 0u;
		std::unique_ptr<met::DynamicPollingMetric> pRealized;
		switch (dataTypeId) {
		case PM_DATA_TYPE_DOUBLE:
			pRealized = std::make_unique<met::TypedDynamicPollingMetric<double>>(requestedMetric, this, deviceId);
			break;
		case PM_DATA_TYPE_STRING:
			pRealized = std::make_unique<met::TypedDynamicPollingMetric<const char*>>(requestedMetric, this, deviceId);
			break;
		}
		metricPtrs.push_back(std::move(pRealized));
		return metricPtrs.back().get();
	}

	void CachingQuery::Finalize(pmapi::Session& session)
	{
		std::vector<PM_QUERY_ELEMENT> queryElements;
		for (auto& pMet : metricPtrs) {
			queryElements.push_back(pMet->MakeQueryElement());
		}
		pQuery = session.RegisterDyanamicQuery(queryElements, winSizeMs, metricOffsetMs);
		for (auto&& [e, m] : std::views::zip(queryElements, metricPtrs)) {
			m->Finalize(uint32_t(e.dataOffset));
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
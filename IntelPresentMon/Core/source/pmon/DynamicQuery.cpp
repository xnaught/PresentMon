#pragma once
#include "DynamicQuery.h"
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include <ranges>


namespace p2c::pmon
{
	DynamicQuery::DynamicQuery(pmapi::Session& session, uint32_t pid, double winSizeMs, double metricOffsetMs, std::span<const kern::QualifiedMetric> qmets)
		:
		pid{ pid }
	{
		for (auto& qmet : qmets) {
			elements.push_back(PM_QUERY_ELEMENT{
				.metric = (PM_METRIC)qmet.metricId,
				.stat = (PM_STAT)qmet.statId,
				.deviceId = qmet.deviceId,
				.arrayIndex = qmet.arrayIndex,
			});
		}
		pQuery = session.RegisterDyanamicQuery(elements, winSizeMs, metricOffsetMs);
		blobs = pQuery->MakeBlobContainer(1u);
	}

	std::vector<PM_QUERY_ELEMENT> DynamicQuery::ExtractElements()
	{
		return std::move(elements);
	}

	void DynamicQuery::Poll()
	{
		if (pQuery) {
			pQuery->Poll(pid, blobs);
		}
	}

	const uint8_t* DynamicQuery::GetBlobData() const
	{
		if (blobs.GetNumBlobsPopulated() == 0) {
			return nullptr;
		}
		return blobs.GetFirst();
	}
}
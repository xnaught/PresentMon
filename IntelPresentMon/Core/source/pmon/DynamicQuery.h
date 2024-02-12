#pragma once
#include <PresentMonAPI/PresentMonAPI.h>
#include <PresentMonAPI2/source/PresentMonAPI.h>
#include <PresentMonAPIWrapper/source/BlobContainer.h>
#include <span>
#include <memory>
#include <vector>
#include <optional>
#include "../kernel/OverlaySpec.h"

namespace pmapi
{
	class Session;
	class DynamicQuery;
}

namespace p2c::pmon
{
	class DynamicQuery
	{
	public:
		DynamicQuery(pmapi::Session& session, uint32_t pid, double winSizeMs, double metricOffsetMs, std::span<const kern::QualifiedMetric> qmet);
		void Poll();
		const uint8_t* GetBlobData() const;
		std::vector<PM_QUERY_ELEMENT> ExtractElements();
	private:
		uint32_t pid;
		std::shared_ptr<pmapi::DynamicQuery> pQuery;
		std::vector<PM_QUERY_ELEMENT> elements;
		pmapi::BlobContainer blobs;
	};
}
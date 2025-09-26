#pragma once
#include <set>
#include <optional>
#include <cereal/types/set.hpp>
#include <cereal/types/optional.hpp>


namespace pmon::test::service
{
	struct Status
	{
		std::set<uint32_t> nsmStreamedPids;
		uint32_t activeAdapterId;
		uint32_t telemetryPeriodMs;
		std::optional<uint32_t> etwFlushPeriodMs;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(nsmStreamedPids, activeAdapterId, telemetryPeriodMs, etwFlushPeriodMs);
		}
	};
}
#include "../../PresentMonUtils/PresentMonNamedPipe.h"
#include "FrameEventQuery.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include "../../CommonUtilities/source/Memory.h"
#include <algorithm>
#include <cstddef>

using namespace pmon;

PmNsmFrameData fff;

#define METRIC_OFFSET_SIZE_LOOKUP_LIST \
	X_(PM_METRIC_PRESENT_QPC, present_event.PresentStartTime) \
	X_(PM_METRIC_RUNTIME, present_event.Runtime) \
	X_(PM_METRIC_PRESENT_MODE, present_event.PresentMode) \
	X_(PM_METRIC_GPU_POWER, power_telemetry.gpu_power_w) \
	X_(PM_METRIC_CPU_UTILIZATION, cpu_telemetry.cpu_utilization)

constexpr uint16_t GetNsmMemberSize_(PM_METRIC metric)
{
	constexpr PmNsmFrameData fd{};
	switch (metric) {
#define X_(metric, nsm) case metric: return sizeof(fd.nsm);
		METRIC_OFFSET_SIZE_LOOKUP_LIST
#undef X_
	default: return 0;
	}
}

constexpr uint32_t GetNsmMemberOffset_(PM_METRIC metric)
{
	switch (metric) {
#define X_(metric, nsm) case metric: return (uint32_t)offsetof(PmNsmFrameData, nsm);
		METRIC_OFFSET_SIZE_LOOKUP_LIST
#undef X_
	default: return 0;
	}
}

PM_FRAME_EVENT_QUERY::PM_FRAME_EVENT_QUERY(std::span<PM_QUERY_ELEMENT> queryElements)
{
	for (auto& q : queryElements) {
		copyCommands_.push_back(MapQueryElementToCopyCommand_(q, blobSize_));
		const auto& cmd = copyCommands_.back();
		q.dataSize = cmd.size;
		q.dataOffset = blobSize_ + cmd.padding;
		blobSize_ += cmd.size + cmd.padding;
	}
}

void PM_FRAME_EVENT_QUERY::GatherToBlob(const uint8_t* sourceFrameData, uint8_t* destBlob) const
{
	for (auto& cmd : copyCommands_) {
		std::copy_n(sourceFrameData + cmd.offset, cmd.size, destBlob + cmd.padding);
		destBlob += cmd.size + cmd.padding;
	}
}

PM_FRAME_EVENT_QUERY::CopyCommand_ PM_FRAME_EVENT_QUERY::MapQueryElementToCopyCommand_(const PM_QUERY_ELEMENT& q, size_t pos)
{
	// TODO: figure out what to do when metric is an array
	const auto size = GetNsmMemberSize_(q.metric);
	return CopyCommand_{
		.offset = GetNsmMemberOffset_(q.metric),
		// this padding will cause issues with something like char[260]
		// TODO: calculate padding based on static type via x-macro machinery
		// lookup from metric
		.padding = (uint8_t)util::GetPadding(pos, size),
		.size = size,
	};
}

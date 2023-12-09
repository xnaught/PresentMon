#include "../../PresentMonUtils/PresentMonNamedPipe.h"
#include "FrameEventQuery.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include "../../CommonUtilities/source/Memory.h"
#include <algorithm>
#include <cstddef>

using namespace pmon;

// TODO: some what of validating that all frame event metrics are covered here
#define METRIC_OFFSET_SIZE_LOOKUP_LIST \
	X_(PM_METRIC_PRESENT_QPC, present_event.PresentStartTime) \
	X_(PM_METRIC_PRESENT_RUNTIME, present_event.Runtime) \
	X_(PM_METRIC_PRESENT_MODE, present_event.PresentMode) \
	X_(PM_METRIC_GPU_POWER, power_telemetry.gpu_power_w) \
	X_(PM_METRIC_CPU_UTILIZATION, cpu_telemetry.cpu_utilization) \
	X_(PM_METRIC_GPU_FAN_SPEED, power_telemetry.fan_speed_rpm[0]) \
	X_(PM_METRIC_GPU_TEMPERATURE_LIMITED, power_telemetry.gpu_temperature_limited)

constexpr uint16_t GetNsmMemberSize_(PM_METRIC metric)
{
	constexpr PmNsmFrameData fd{};
	switch (metric) {
#define X_(metric, nsm) case metric: return sizeof(fd.nsm);
		METRIC_OFFSET_SIZE_LOOKUP_LIST
#undef X_
			// TODO: do something with this default
	default: return 0;
	}
}

constexpr uint32_t GetNsmMemberOffset_(PM_METRIC metric)
{
	switch (metric) {
#define X_(metric, nsm) case metric: return (uint32_t)offsetof(PmNsmFrameData, nsm);
		METRIC_OFFSET_SIZE_LOOKUP_LIST
#undef X_
			// TODO: do something with this default
	default: return 0;
	}
}

PM_FRAME_EVENT_QUERY::PM_FRAME_EVENT_QUERY(std::span<PM_QUERY_ELEMENT> queryElements)
{
	// TODO: validation
	//	only allow array index zero if not array type in nsm
	//	fail if array index out of bounds
	//  fail if any metrics aren't event-compatible
	//  fail if any stats other than NONE are specified
	
	// we need to keep track of how many non-universal devices are specified
	// current release: only 1 gpu device maybe be polled at a time
	for (auto& q : queryElements) {
		// validate that maximum 1 device (gpu) id is specified throughout the query
		if (q.deviceId != 0) {
			if (!referencedDevice_) {
				referencedDevice_ = q.deviceId;
			}
			else if (*referencedDevice_ != q.deviceId) {
				throw std::runtime_error{ "Cannot specify 2 different non-universal devices in the same query" };
			}
		}
		copyCommands_.push_back(MapQueryElementToCopyCommand_(q, blobSize_));
		const auto& cmd = copyCommands_.back();
		q.dataSize = cmd.size;
		q.dataOffset = blobSize_ + cmd.padding;
		blobSize_ += cmd.size + cmd.padding;
	}
	// make sure blobs are a multiple of 16 so that blobs in array always start 16-aligned
	blobSize_ += util::GetPadding(blobSize_, 16);
}

void PM_FRAME_EVENT_QUERY::GatherToBlob(const uint8_t* sourceFrameData, uint8_t* destBlob) const
{
	for (auto& cmd : copyCommands_) {
		std::copy_n(sourceFrameData + cmd.offset, cmd.size, destBlob + cmd.padding);
		destBlob += cmd.size + cmd.padding;
	}
}

size_t PM_FRAME_EVENT_QUERY::GetBlobSize() const
{
	return blobSize_;
}

PM_FRAME_EVENT_QUERY::CopyCommand_ PM_FRAME_EVENT_QUERY::MapQueryElementToCopyCommand_(const PM_QUERY_ELEMENT& q, size_t pos)
{
	// TODO: figure out what to do when metric is an array
	const auto size = GetNsmMemberSize_(q.metric);
	return CopyCommand_{
		.offset = GetNsmMemberOffset_(q.metric) + size * q.arrayIndex,
		// this padding will cause issues with something like char[260]
		// TODO: calculate padding based on static type via x-macro machinery
		// lookup from metric (not good for forward compat., maybe use intro?)
		.padding = (uint8_t)util::GetPadding(pos, size),
		.size = size,
	};
}

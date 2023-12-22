#define NOMINMAX
#include "../../PresentMonUtils/PresentMonNamedPipe.h"
#include "FrameEventQuery.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include "../../CommonUtilities/source/Memory.h"
#include "../../CommonUtilities/source/Meta.h"
#include <algorithm>
#include <cstddef>

using namespace pmon;
using Context = PM_FRAME_QUERY::Context;

namespace pmon::mid
{
	class GatherCommand_
	{
	public:
		virtual ~GatherCommand_() = default;
		virtual void Gather(const Context& ctx, uint8_t* pDestBlob) const = 0;
		virtual uint32_t GetBeginOffset() const = 0;
		virtual uint32_t GetEndOffset() const = 0;
		virtual uint32_t GetOutputOffset() const = 0;
		uint32_t GetDataSize() const { return GetEndOffset() - GetOutputOffset(); }
		uint32_t GetTotalSize() const { return GetEndOffset() - GetBeginOffset(); }
	};
}

namespace
{
	template<auto pMember>
	constexpr auto GetSubstructurePointer()
	{
		using SubstructureType = util::MemberPointerInfo<decltype(pMember)>::StructType;
		if constexpr (std::same_as<SubstructureType, PmNsmPresentEvent>) {
			return &PmNsmFrameData::present_event;
		}
		else if constexpr (std::same_as<SubstructureType, PresentMonPowerTelemetryInfo>) {
			return &PmNsmFrameData::power_telemetry;
		}
		else if constexpr (std::same_as<SubstructureType, CpuTelemetryInfo>) {
			return &PmNsmFrameData::cpu_telemetry;
		}
	}

	template<auto pMember>
	class CopyGatherCommand_ : public mid::GatherCommand_
	{
		using Type = util::MemberPointerInfo<decltype(pMember)>::MemberType;
	public:
		CopyGatherCommand_(size_t nextAvailableByteOffset, uint16_t index = 0)
			:
			inputIndex_{ index }
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(Type));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const Context& ctx, uint8_t* pDestBlob) const override
		{
			constexpr auto pSubstruct = GetSubstructurePointer<pMember>();
			if constexpr (std::is_array_v<Type>) {
				const auto val = (ctx.pSourceFrameData->*pSubstruct.*pMember)[inputIndex_];
				reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
			}
			else {
				const auto val = ctx.pSourceFrameData->*pSubstruct.*pMember;
				reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(Type);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
		uint16_t inputIndex_;
	};
	template<uint64_t PmNsmPresentEvent::* pMember>
	class QpcDurationGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		QpcDurationGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const Context& ctx, uint8_t* pDestBlob) const override
		{
			const auto qpcDuration = ctx.pSourceFrameData->present_event.*pMember;
			if (qpcDuration != 0) {
				const auto val = ctx.performanceCounterPeriodMs * double(qpcDuration);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	template<uint64_t PmNsmPresentEvent::* pStart, uint64_t PmNsmPresentEvent::* pEnd, bool doZeroCheck, bool doDroppedCheck, bool allowNegative, bool clampZero>
	class QpcDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		QpcDifferenceGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const Context& ctx, uint8_t* pDestBlob) const override
		{
			static_assert(!allowNegative || !clampZero);
			if constexpr (doDroppedCheck) {
				if (ctx.dropped) {
					return;
				}
			}
			uint64_t start = ctx.pSourceFrameData->present_event.*pStart;
			if constexpr (doZeroCheck) {
				if (start == 0ull) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
					return;
				}
			}
			if constexpr (allowNegative || clampZero) {
				auto qpcDurationDouble = double(ctx.pSourceFrameData->present_event.*pEnd) - double(start);
				if constexpr (clampZero) {
					qpcDurationDouble = std::max(0., qpcDurationDouble);
				}
				const auto val = ctx.performanceCounterPeriodMs * qpcDurationDouble;
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
			else {
				const auto qpcDuration = ctx.pSourceFrameData->present_event.*pEnd - start;
				const auto val = ctx.performanceCounterPeriodMs * double(qpcDuration);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class DroppedGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		DroppedGatherCommand_(size_t nextAvailableByteOffset) : outputOffset_{ (uint32_t)nextAvailableByteOffset } {}
		void Gather(const Context& ctx, uint8_t* pDestBlob) const override
		{
			reinterpret_cast<bool&>(pDestBlob[outputOffset_]) = ctx.dropped;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(bool);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
	};
	template<uint64_t PmNsmPresentEvent::* pEnd>
	class StartDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		StartDifferenceGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const Context& ctx, uint8_t* pDestBlob) const override
		{
			const auto qpcDuration = ctx.pSourceFrameData->present_event.*pEnd - ctx.qpcStart;
			const auto val = ctx.performanceCounterPeriodMs * double(qpcDuration);
			reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
}

PM_FRAME_QUERY::PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements)
{
	// TODO: validation
	//	only allow array index zero if not array type in nsm
	//	fail if array index out of bounds
	//  fail if any metrics aren't event-compatible
	//  fail if any stats other than NONE are specified
	//  fail if intro size doesn't match nsm size
	
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
		gatherCommands_.push_back(MapQueryElementToGatherCommand_(q, blobSize_));
		const auto& cmd = gatherCommands_.back();
		q.dataSize = cmd->GetDataSize();
		q.dataOffset = cmd->GetOutputOffset();
		blobSize_ += cmd->GetTotalSize();
	}
	// make sure blobs are a multiple of 16 so that blobs in array always start 16-aligned
	blobSize_ += util::GetPadding(blobSize_, 16);
}

PM_FRAME_QUERY::~PM_FRAME_QUERY() = default;

void PM_FRAME_QUERY::GatherToBlob(const Context& ctx, uint8_t* pDestBlob) const
{
	for (auto& cmd : gatherCommands_) {
		cmd->Gather(ctx, pDestBlob);
	}
}

size_t PM_FRAME_QUERY::GetBlobSize() const
{
	return blobSize_;
}

std::optional<uint32_t> PM_FRAME_QUERY::GetReferencedDevice() const
{
	return referencedDevice_;
}

std::unique_ptr<mid::GatherCommand_> PM_FRAME_QUERY::MapQueryElementToGatherCommand_(const PM_QUERY_ELEMENT& q, size_t pos)
{
	using Pre = PmNsmPresentEvent;
	using Gpu = PresentMonPowerTelemetryInfo;
	using Cpu = CpuTelemetryInfo;

	switch (q.metric) {
	// temporary static metric lookup via nsm
	// only implementing the ones used by appcef right now... others available in the future
	// TODO: implement fill for all static OR drop support for filling static
	case PM_METRIC_GPU_MEM_SIZE:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_total_size_b>>(pos);
	case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_max_bandwidth_bps>>(pos);


	case PM_METRIC_SWAP_CHAIN:
		return std::make_unique<CopyGatherCommand_<&Pre::SwapChainAddress>>(pos);
	case PM_METRIC_GPU_BUSY_TIME:
		return std::make_unique<QpcDurationGatherCommand_<&Pre::GPUDuration>>(pos);
	case PM_METRIC_DROPPED_FRAMES:
		return std::make_unique<DroppedGatherCommand_>(pos);
	case PM_METRIC_PRESENT_MODE:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentMode>>(pos);
	case PM_METRIC_PRESENT_RUNTIME:
		return std::make_unique<CopyGatherCommand_<&Pre::Runtime>>(pos);
	case PM_METRIC_PRESENT_QPC:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentStartTime>>(pos);
	case PM_METRIC_ALLOWS_TEARING:
		return std::make_unique<CopyGatherCommand_<&Pre::SupportsTearing>>(pos);
	case PM_METRIC_SYNC_INTERVAL:
		return std::make_unique<CopyGatherCommand_<&Pre::SyncInterval>>(pos);

	case PM_METRIC_GPU_POWER:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_power_w>>(pos);
	case PM_METRIC_GPU_VOLTAGE:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_voltage_v>>(pos);
	case PM_METRIC_GPU_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_frequency_mhz>>(pos);
	case PM_METRIC_GPU_TEMPERATURE:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_temperature_c>>(pos);
	case PM_METRIC_GPU_FAN_SPEED:
		return std::make_unique<CopyGatherCommand_<&Gpu::fan_speed_rpm>>(pos, q.arrayIndex);
	case PM_METRIC_GPU_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_utilization>>(pos);
	case PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_render_compute_utilization>>(pos);
	case PM_METRIC_GPU_MEDIA_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_media_utilization>>(pos);
	case PM_METRIC_VRAM_POWER:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_power_w>>(pos);
	case PM_METRIC_VRAM_VOLTAGE:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_voltage_v>>(pos);
	case PM_METRIC_VRAM_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_frequency_mhz>>(pos);
	case PM_METRIC_VRAM_EFFECTIVE_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_effective_frequency_gbps>>(pos);
	case PM_METRIC_VRAM_TEMPERATURE:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_temperature_c>>(pos);
	case PM_METRIC_GPU_MEM_USED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_used_b>>(pos);
	case PM_METRIC_GPU_MEM_WRITE_BANDWIDTH:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_write_bandwidth_bps>>(pos);
	case PM_METRIC_GPU_MEM_READ_BANDWIDTH:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_read_bandwidth_bps>>(pos);
	case PM_METRIC_GPU_POWER_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_power_limited>>(pos);
	case PM_METRIC_GPU_TEMPERATURE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_temperature_limited>>(pos);
	case PM_METRIC_GPU_CURRENT_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_current_limited>>(pos);
	case PM_METRIC_GPU_VOLTAGE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_voltage_limited>>(pos);
	case PM_METRIC_GPU_UTILIZATION_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_utilization_limited>>(pos);
	case PM_METRIC_VRAM_POWER_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_power_limited>>(pos);
	case PM_METRIC_VRAM_TEMPERATURE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_temperature_limited>>(pos);
	case PM_METRIC_VRAM_CURRENT_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_current_limited>>(pos);
	case PM_METRIC_VRAM_VOLTAGE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_voltage_limited>>(pos);
	case PM_METRIC_VRAM_UTILIZATION_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_utilization_limited>>(pos);

	case PM_METRIC_CPU_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_utilization>>(pos);
	case PM_METRIC_CPU_POWER:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_power_w>>(pos);
	case PM_METRIC_CPU_TEMPERATURE:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_temperature>>(pos);
	case PM_METRIC_CPU_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_frequency>>(pos);

	case PM_METRIC_PRESENT_FLAGS:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentFlags>>(pos);
	case PM_METRIC_TIME:
		return std::make_unique<StartDifferenceGatherCommand_<&Pre::PresentStartTime>>(pos);
	case PM_METRIC_TIME_IN_PRESENT_API:
		return std::make_unique<QpcDifferenceGatherCommand_<&Pre::PresentStartTime, &Pre::ScreenTime, 0, 0, 0, 1>>(pos);
	case PM_METRIC_TIME_BETWEEN_PRESENTS:
		return std::make_unique<QpcDifferenceGatherCommand_<&Pre::last_present_qpc, &Pre::PresentStartTime, 0, 0, 0, 0>>(pos);
	case PM_METRIC_TIME_UNTIL_RENDER_COMPLETE:
		return std::make_unique<QpcDifferenceGatherCommand_<&Pre::PresentStartTime, &Pre::ReadyTime, 1, 0, 1, 0>>(pos);
	case PM_METRIC_TIME_UNTIL_DISPLAYED:
		return std::make_unique<QpcDifferenceGatherCommand_<&Pre::PresentStartTime, &Pre::ScreenTime, 0, 1, 0, 0>>(pos);
	case PM_METRIC_TIME_BETWEEN_DISPLAY_CHANGE:
		return std::make_unique<QpcDifferenceGatherCommand_<&Pre::last_displayed_qpc, &Pre::ScreenTime, 1, 1, 0, 0>>(pos);
	case PM_METRIC_TIME_UNTIL_RENDER_START:
		return std::make_unique<QpcDifferenceGatherCommand_<&Pre::PresentStartTime, &Pre::GPUStartTime, 1, 0, 1, 0>>(pos);
	case PM_METRIC_TIME_SINCE_INPUT:
		return std::make_unique<QpcDifferenceGatherCommand_<&Pre::InputTime, &Pre::PresentStartTime, 1, 0, 0, 0>>(pos);
	case PM_METRIC_GPU_VIDEO_BUSY_TIME:
		return std::make_unique<QpcDurationGatherCommand_<&Pre::GPUVideoDuration>>(pos);
	default: return {};
	}
}

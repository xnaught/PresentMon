#include "../../PresentMonUtils/PresentMonNamedPipe.h"
#include "FrameEventQuery.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include "../../CommonUtilities/source/Memory.h"
#include "../../CommonUtilities/source/Meta.h"
#include <algorithm>
#include <cstddef>

using namespace pmon;

PmNsmFrameData ff;

namespace pmon::mid
{
	class GatherCommand_
	{
	public:
		struct Context
		{
			double performanceCounterPeriodMs;
			uint64_t qpcStart;
			bool dropped;

		};
		virtual ~GatherCommand_() = default;
		virtual void Gather(const PmNsmFrameData* pSourceFrameData, uint8_t* pDestBlob, const Context& ctx) const = 0;
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
			// TODO: checking that introspection type matches nsm type
			if constexpr (std::is_array_v<Type>) {
				using ElementType = util::ContainerElementType<Type>;
				outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, sizeof(ElementType));
			}
			else {
				outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, sizeof(Type));
			}
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const PmNsmFrameData* pSourceFrameData, uint8_t* pDestBlob, const Context&) const override
		{
			constexpr auto pSubstruct = GetSubstructurePointer<pMember>();
			if constexpr (std::is_array_v<Type>) {
				const auto val = (pSourceFrameData->*pSubstruct.*pMember)[inputIndex_];
				reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
			}
			else {
				const auto val = pSourceFrameData->*pSubstruct.*pMember;
				reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			if constexpr (std::is_array_v<Type>) {
				return outputOffset_ + sizeof(util::ContainerElementType<Type>);
			}
			else {
				return outputOffset_ + sizeof(Type);
			}
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
		QpcDurationGatherCommand_(size_t nextAvailableByteOffset) 		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, sizeof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const PmNsmFrameData* pSourceFrameData, uint8_t* pDestBlob, const Context& ctx) const override
		{
			const auto qpcDuration = pSourceFrameData->present_event.*pMember;
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
			return outputOffset_ + sizeof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	//class QpcDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	//{

	//};
	//class DroppedGatherCommand_ : public pmon::mid::GatherCommand_
	//{

	//};
	//class DroppedQpcDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	//{

	//};
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

void PM_FRAME_QUERY::GatherToBlob(const PmNsmFrameData* pSourceFrameData, uint8_t* pDestBlob, uint64_t qpcStart, double performanceCounterPeriodMs) const
{
	const mid::GatherCommand_::Context ctx{
		.performanceCounterPeriodMs = performanceCounterPeriodMs,
		.qpcStart = qpcStart,
		.dropped = pSourceFrameData->present_event.FinalState != PresentResult::Presented,
	};
	for (auto& cmd : gatherCommands_) {
		cmd->Gather(pSourceFrameData, pDestBlob, ctx);
	}
}

size_t PM_FRAME_QUERY::GetBlobSize() const
{
	return blobSize_;
}

std::unique_ptr<mid::GatherCommand_> PM_FRAME_QUERY::MapQueryElementToGatherCommand_(const PM_QUERY_ELEMENT& q, size_t pos)
{
	using Pre = PmNsmPresentEvent;
	using Gpu = PresentMonPowerTelemetryInfo;
	using Cpu = CpuTelemetryInfo;

	switch (q.metric) {
	case PM_METRIC_PRESENT_RUNTIME:
		return std::make_unique<CopyGatherCommand_<&Pre::Runtime>>(pos);
	case PM_METRIC_GPU_POWER:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_power_w>>(pos);
	case PM_METRIC_GPU_TEMPERATURE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_temperature_limited>>(pos);
	case PM_METRIC_PRESENT_MODE:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentMode>>(pos);
	case PM_METRIC_PRESENT_QPC:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentStartTime>>(pos);
	case PM_METRIC_GPU_FAN_SPEED:
		return std::make_unique<CopyGatherCommand_<&Gpu::fan_speed_rpm>>(pos, q.arrayIndex);
	case PM_METRIC_CPU_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_utilization>>(pos);
	case PM_METRIC_GPU_BUSY_TIME:
		return std::make_unique<QpcDurationGatherCommand_<&Pre::GPUDuration>>(pos);
	default: return {};
	}
}

// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#define NOMINMAX
#include "../PresentMonUtils/StreamFormat.h"
#include "FrameEventQuery.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../CommonUtilities/Memory.h"
#include "../CommonUtilities/Meta.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Exception.h"
#include <algorithm>
#include <cstddef>
#include <limits>

using namespace pmon;
using namespace pmon::util;
using Context = PM_FRAME_QUERY::Context;

namespace pmon::mid
{
	class GatherCommand_
	{
	public:
		virtual ~GatherCommand_() = default;
		virtual void Gather(Context& ctx, uint8_t* pDestBlob) const = 0;
		virtual uint32_t GetBeginOffset() const = 0;
		virtual uint32_t GetEndOffset() const = 0;
		virtual uint32_t GetOutputOffset() const = 0;
		uint32_t GetDataSize() const { return GetEndOffset() - GetOutputOffset(); }
		uint32_t GetTotalSize() const { return GetEndOffset() - GetBeginOffset(); }
	};
}

namespace
{
	double TimestampDeltaToMilliSeconds(uint64_t timestampDelta, double performanceCounterPeriodMs)
	{
		return performanceCounterPeriodMs * double(timestampDelta);
	}

	double TimestampDeltaToUnsignedMilliSeconds(uint64_t timestampFrom, uint64_t timestampTo, double performanceCounterPeriodMs)
	{
		return timestampFrom == 0 || timestampTo <= timestampFrom ? 0.0 : 
			TimestampDeltaToMilliSeconds(timestampTo - timestampFrom, performanceCounterPeriodMs);
	}

	double TimestampDeltaToMilliSeconds(uint64_t timestampFrom, uint64_t timestampTo, double performanceCounterPeriodMs)
	{
		return timestampFrom == 0 || timestampTo == 0 || timestampFrom == timestampTo ? 0.0 :
			timestampTo > timestampFrom ? TimestampDeltaToMilliSeconds(timestampTo - timestampFrom, performanceCounterPeriodMs)
			: -TimestampDeltaToMilliSeconds(timestampFrom - timestampTo, performanceCounterPeriodMs);
	}

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
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			constexpr auto pSubstruct = GetSubstructurePointer<pMember>();
			if constexpr (std::is_array_v<Type>) {
				if constexpr (std::is_same_v<std::remove_extent_t<Type>, char>) {
					const auto val = (ctx.pSourceFrameData->*pSubstruct.*pMember)[inputIndex_];
					// TODO: only getting first character of application name. Hmmm.
					strcpy_s(reinterpret_cast<char*>(&pDestBlob[outputOffset_]), 260, &val);
				}
				else {
					const auto val = (ctx.pSourceFrameData->*pSubstruct.*pMember)[inputIndex_];
					reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
				}
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
	class CopyGatherFrameTypeCommand_ : public mid::GatherCommand_
	{
	public:
		CopyGatherFrameTypeCommand_(size_t nextAvailableByteOffset, uint16_t index = 0)
			:
			inputIndex_{ index }
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(FrameType));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const Context& ctx, uint8_t* pDestBlob) const override
		{
            auto val = ctx.pSourceFrameData->present_event.Displayed_FrameType[ctx.sourceFrameDisplayIndex];
			// Currently not reporting out not set or repeated frames.
			if (val == FrameType::NotSet || val == FrameType::Repeated) {
				val = FrameType::Application;
			}
            reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(FrameType);
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
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			const auto qpcDuration = ctx.pSourceFrameData->present_event.*pMember;
			if (qpcDuration != 0) {
				if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
					const auto val = ctx.performanceCounterPeriodMs * double(qpcDuration);
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
				}
				else {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
				}
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
	
	class GpuTimeGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		GpuTimeGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
				const auto gpuDuration = TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.GPUStartTime,
					ctx.pSourceFrameData->present_event.ReadyTime, ctx.performanceCounterPeriodMs);
				const auto gpuBusy = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.GPUDuration,
					ctx.performanceCounterPeriodMs);
				const auto gpuWait = std::max(0., gpuDuration - gpuBusy);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = gpuBusy + gpuWait;
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
	class ClickToPhotonGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		ClickToPhotonGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			uint64_t start = ctx.pSourceFrameData->present_event.InputTime;
			if (start == 0ull) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
				const auto val = TimestampDeltaToUnsignedMilliSeconds(start,
					ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex], ctx.performanceCounterPeriodMs);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
			else
			{
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
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
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
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
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
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
	class CpuFrameQpcGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		CpuFrameQpcGatherCommand_(size_t nextAvailableByteOffset) : outputOffset_{ (uint32_t)nextAvailableByteOffset } {}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			reinterpret_cast<uint64_t&>(pDestBlob[outputOffset_]) = ctx.cpuStart;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(uint64_t);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
	};
	template<uint64_t PmNsmPresentEvent::* pEnd, bool doDroppedCheck>
	class CpuFrameQpcDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		CpuFrameQpcDifferenceGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if constexpr (doDroppedCheck) {
				if (ctx.dropped) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
			}
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
				const auto val = TimestampDeltaToUnsignedMilliSeconds(ctx.cpuStart,
					ctx.pSourceFrameData->present_event.*pEnd, ctx.performanceCounterPeriodMs);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
			else{
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
	class DisplayLatencyGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		DisplayLatencyGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(const Context& ctx, uint8_t* pDestBlob) const override
		{
            if (ctx.dropped) {
                reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
                    std::numeric_limits<double>::quiet_NaN();
                return;
            }
			const auto val = TimestampDeltaToUnsignedMilliSeconds(ctx.cpuStart,
				ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex], ctx.performanceCounterPeriodMs);
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
	class DisplayDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		DisplayDifferenceGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			auto ScreenTime = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
			auto NextScreenTime = ctx.sourceFrameDisplayIndex == ctx.pSourceFrameData->present_event.DisplayedCount - 1
                ? ctx.nextDisplayedQpc
                : ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex + 1];
			const auto val = TimestampDeltaToUnsignedMilliSeconds(ScreenTime, NextScreenTime, ctx.performanceCounterPeriodMs);
			if (val == 0.) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
			else {
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
	class AnimationErrorGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		AnimationErrorGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t) util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			if (ctx.previousDisplayedCpuStartQpc == 0) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex && ctx.previousDisplayedCpuStartQpc != 0) {
				auto ScreenTime = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
				auto PrevScreenTime = ctx.previousDisplayedQpc; // Always use application display time for animation error
				const auto val = TimestampDeltaToMilliSeconds(ScreenTime - PrevScreenTime,
					ctx.cpuStart - ctx.previousDisplayedCpuStartQpc, ctx.performanceCounterPeriodMs);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
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
	class CpuFrameQpcFrameTimeCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		CpuFrameQpcFrameTimeCommand_(size_t nextAvailableByteOffset) : outputOffset_{ (uint32_t)nextAvailableByteOffset } {}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
				const auto cpuBusy = TimestampDeltaToUnsignedMilliSeconds(ctx.cpuStart, ctx.pSourceFrameData->present_event.PresentStartTime,
					ctx.performanceCounterPeriodMs);
				const auto cpuWait = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.TimeInPresent,
					ctx.performanceCounterPeriodMs);

				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = cpuBusy + cpuWait;
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(uint64_t);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
	};
	class GpuWaitGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		GpuWaitGatherCommand_(size_t nextAvailableByteOffset) : outputOffset_{ (uint32_t)nextAvailableByteOffset } {}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
				const auto gpuDuration = TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.GPUStartTime,
					ctx.pSourceFrameData->present_event.ReadyTime, ctx.performanceCounterPeriodMs);
				const auto gpuBusy = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.GPUDuration,
					ctx.performanceCounterPeriodMs);
				const auto val = std::max(0., gpuDuration - gpuBusy);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(uint64_t);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
	};
	template<uint64_t PmNsmPresentEvent::* pStart, uint64_t PmNsmPresentEvent::* pEnd, bool doDroppedCheck, bool isMouseClick>
	class InputLatencyGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		InputLatencyGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if constexpr (doDroppedCheck) {
				if (ctx.dropped) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
			}

			double updatedInputTime = 0.;
			double val = 0.;
			if (isMouseClick) {
				updatedInputTime = ctx.lastReceivedNotDisplayedClickQpc == 0 ? 0. :
					TimestampDeltaToUnsignedMilliSeconds(ctx.lastReceivedNotDisplayedClickQpc,
						ctx.pSourceFrameData->present_event.*pEnd, ctx.performanceCounterPeriodMs);
				val = ctx.pSourceFrameData->present_event.*pStart == 0 ? updatedInputTime :
					TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.*pStart,
						ctx.pSourceFrameData->present_event.*pEnd,
						ctx.performanceCounterPeriodMs);
				ctx.lastReceivedNotDisplayedClickQpc = 0;
			}
			else {
				updatedInputTime = ctx.lastReceivedNotDisplayedAllInputTime == 0 ? 0. :
					TimestampDeltaToUnsignedMilliSeconds(ctx.lastReceivedNotDisplayedAllInputTime,
						ctx.pSourceFrameData->present_event.*pEnd, ctx.performanceCounterPeriodMs);
				val = ctx.pSourceFrameData->present_event.*pStart == 0 ? updatedInputTime :
					TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.*pStart,
						ctx.pSourceFrameData->present_event.*pEnd,
						ctx.performanceCounterPeriodMs);
				ctx.lastReceivedNotDisplayedAllInputTime = 0;
			}

			if (val == 0.) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
			else {
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
				pmlog_error("Cannot specify 2 different non-universal devices in the same query")
					.pmwatch(*referencedDevice_).pmwatch(q.deviceId).diag();
				throw Except<Exception>("2 different non-universal devices in same query");
			}
		}
		if (auto pElement = MapQueryElementToGatherCommand_(q, blobSize_)) {
			gatherCommands_.push_back(std::move(pElement));
			const auto& cmd = gatherCommands_.back();
			q.dataSize = cmd->GetDataSize();
			q.dataOffset = cmd->GetOutputOffset();
			blobSize_ += cmd->GetTotalSize();
		}
	}
	// make sure blobs are a multiple of 16 so that blobs in array always start 16-aligned
	blobSize_ += util::GetPadding(blobSize_, 16);
}

PM_FRAME_QUERY::~PM_FRAME_QUERY() = default;

void PM_FRAME_QUERY::GatherToBlob(Context& ctx, uint8_t* pDestBlob) const
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
	case PM_METRIC_APPLICATION:
		return std::make_unique<CopyGatherCommand_<&Pre::application>>(pos);
	case PM_METRIC_GPU_MEM_SIZE:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_total_size_b>>(pos);
	case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_max_bandwidth_bps>>(pos);

	case PM_METRIC_SWAP_CHAIN_ADDRESS:
		return std::make_unique<CopyGatherCommand_<&Pre::SwapChainAddress>>(pos);
	case PM_METRIC_GPU_BUSY:
		return std::make_unique<QpcDurationGatherCommand_<&Pre::GPUDuration>>(pos);
	case PM_METRIC_DROPPED_FRAMES:
		return std::make_unique<DroppedGatherCommand_>(pos);
	case PM_METRIC_PRESENT_MODE:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentMode>>(pos);
	case PM_METRIC_PRESENT_RUNTIME:
		return std::make_unique<CopyGatherCommand_<&Pre::Runtime>>(pos);
	case PM_METRIC_CPU_START_QPC:
		return std::make_unique<CpuFrameQpcGatherCommand_>(pos);
	case PM_METRIC_ALLOWS_TEARING:
		return std::make_unique<CopyGatherCommand_<&Pre::SupportsTearing>>(pos);
	case PM_METRIC_FRAME_TYPE:
		return std::make_unique<CopyGatherFrameTypeCommand_>(pos);
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
	case PM_METRIC_GPU_MEM_POWER:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_power_w>>(pos);
	case PM_METRIC_GPU_MEM_VOLTAGE:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_voltage_v>>(pos);
	case PM_METRIC_GPU_MEM_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_frequency_mhz>>(pos);
	case PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_effective_frequency_gbps>>(pos);
	case PM_METRIC_GPU_MEM_TEMPERATURE:
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
	case PM_METRIC_GPU_MEM_POWER_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_power_limited>>(pos);
	case PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_temperature_limited>>(pos);
	case PM_METRIC_GPU_MEM_CURRENT_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_current_limited>>(pos);
	case PM_METRIC_GPU_MEM_VOLTAGE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_voltage_limited>>(pos);
	case PM_METRIC_GPU_MEM_UTILIZATION_LIMITED:
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
	case PM_METRIC_CPU_START_TIME:
		return std::make_unique<StartDifferenceGatherCommand_<&Pre::PresentStartTime>>(pos);
	case PM_METRIC_CPU_FRAME_TIME:
		return std::make_unique<CpuFrameQpcFrameTimeCommand_>(pos);
	case PM_METRIC_CPU_BUSY:
		return std::make_unique<CpuFrameQpcDifferenceGatherCommand_<&Pre::PresentStartTime, 0>>(pos);
	case PM_METRIC_CPU_WAIT:
		return std::make_unique<QpcDurationGatherCommand_<&Pre::TimeInPresent>>(pos);
	case PM_METRIC_GPU_TIME:
		return std::make_unique<GpuTimeGatherCommand_>(pos);
	case PM_METRIC_GPU_WAIT:
		return std::make_unique<GpuWaitGatherCommand_>(pos);
	case PM_METRIC_DISPLAYED_TIME:
		return std::make_unique<DisplayDifferenceGatherCommand_>(pos);
	case PM_METRIC_ANIMATION_ERROR:
		return std::make_unique<AnimationErrorGatherCommand_>(pos);
	case PM_METRIC_GPU_LATENCY:
		return std::make_unique<CpuFrameQpcDifferenceGatherCommand_<&Pre::GPUStartTime, 0>>(pos);
	case PM_METRIC_DISPLAY_LATENCY:
		return std::make_unique<DisplayLatencyGatherCommand_>(pos);
	case PM_METRIC_CLICK_TO_PHOTON_LATENCY:
		return std::make_unique<InputLatencyGatherCommand_<&Pre::MouseClickTime, &Pre::ScreenTime, 1, 1>>(pos);
	case PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY:
		return std::make_unique<InputLatencyGatherCommand_<&Pre::InputTime, &Pre::ScreenTime, 1, 0>>(pos);

	default:
		pmlog_error("unknown metric id").pmwatch((int)q.metric).diag();
		return {};
	}
}

void PM_FRAME_QUERY::Context::UpdateSourceData(const PmNsmFrameData* pSourceFrameData_in,
											   const PmNsmFrameData* pFrameDataOfNextDisplayed,
										       const PmNsmFrameData* pFrameDataOfLastPresented,
										       const PmNsmFrameData* pFrameDataOfLastDisplayed,
										       const PmNsmFrameData* pPreviousFrameDataOfLastDisplayed)
{
	pSourceFrameData = pSourceFrameData_in;
    @@@ do manual merge from GameTechDev/main
	sourceFrameDisplayIndex = 0;
	dropped = pSourceFrameData->present_event.FinalState != PresentResult::Presented && pSourceFrameData->present_event.DisplayedCount == 0;
	cpuStart = pFrameDataOfLastPresented->present_event.PresentStartTime + pFrameDataOfLastPresented->present_event.TimeInPresent;
	nextDisplayedQpc = pFrameDataOfNextDisplayed->present_event.Displayed_ScreenTime[0];
	if (pFrameDataOfLastDisplayed && pFrameDataOfLastDisplayed->present_event.DisplayedCount > 0) {
		previousDisplayedQpc = pFrameDataOfLastDisplayed->present_event.Displayed_ScreenTime[pFrameDataOfLastDisplayed->present_event.DisplayedCount - 1];
	}
	else {
		// TODO: log issue or invalidate related columns or drop frame (or some combination)
		pmlog_info("null pFrameDataOfLastDisplayed");
		previousDisplayedQpc = 0;
	}
	if (pPreviousFrameDataOfLastDisplayed) {
		previousDisplayedCpuStartQpc = pPreviousFrameDataOfLastDisplayed->present_event.PresentStartTime + pPreviousFrameDataOfLastDisplayed->present_event.TimeInPresent;
	}
	else {
		previousDisplayedCpuStartQpc = 0;
	}
	appIndex = 0;
	for (size_t i = 0; i < pSourceFrameData->present_event.DisplayedCount; ++i) {
		if (pSourceFrameData->present_event.Displayed_FrameType[i] == FrameType::NotSet ||
			pSourceFrameData->present_event.Displayed_FrameType[i] == FrameType::Application) {
			appIndex = i;
			break;
		}
	}
}

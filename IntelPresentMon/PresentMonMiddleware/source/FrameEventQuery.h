#pragma once
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include <vector>
#include <span>
#include <memory>

namespace pmapi::intro
{
	class Root;
}

namespace pmon::mid
{
	class GatherCommand_;
}

struct PM_FRAME_QUERY
{
public:
	// types
	struct Context
	{
		// functions
		Context(uint64_t qpcStart, long long perfCounterFrequency) : qpcStart{ qpcStart },
			performanceCounterPeriodMs{ 1000.0 / perfCounterFrequency } {}
		void UpdateSourceData(const PmNsmFrameData* pSourceFrameData_in,
			const PmNsmFrameData* pNextDisplayedFrameData,
			const PmNsmFrameData* pPreviousFrameData)
		{
			pSourceFrameData = pSourceFrameData_in;
			dropped = pSourceFrameData->present_event.FinalState != PresentResult::Presented;
			if (pPreviousFrameData) {
				cpuFrameQpc = pPreviousFrameData->present_event.PresentStartTime + pPreviousFrameData->present_event.TimeInPresent;
			}
			else {
				// TODO: log issue or invalidate related columns or drop frame (or some combination)
				cpuFrameQpc = 0;
			}
			if (pNextDisplayedFrameData) {
				nextDisplayedQpc = pNextDisplayedFrameData->present_event.ScreenTime;
			}
			else {
				// TODO: log issue or invalidate related columns or drop frame (or some combination)
				nextDisplayedQpc = 0;
			}
		}
		// data
		const PmNsmFrameData* pSourceFrameData = nullptr;
		const double performanceCounterPeriodMs;
		const uint64_t qpcStart;
		bool dropped;
		uint64_t cpuFrameQpc = 0;
		uint64_t nextDisplayedQpc = 0;
	};
	// functions
	PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements);
	~PM_FRAME_QUERY();
	void GatherToBlob(const Context& ctx, uint8_t* pDestBlob) const;
	size_t GetBlobSize() const;
	std::optional<uint32_t> GetReferencedDevice() const;
private:
	// functions
	std::unique_ptr<pmon::mid::GatherCommand_> MapQueryElementToGatherCommand_(const PM_QUERY_ELEMENT& q, size_t pos);
	// data
	std::vector<std::unique_ptr<pmon::mid::GatherCommand_>> gatherCommands_;
	size_t blobSize_ = 0;
	std::optional<uint32_t> referencedDevice_;
};
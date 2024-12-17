#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonUtils/StreamFormat.h"
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
		Context(uint64_t qpcStart, long long perfCounterFrequency, uint64_t appSimStartTime) : qpcStart{ qpcStart },
			performanceCounterPeriodMs{ perfCounterFrequency != 0.f ? 1000.0 / perfCounterFrequency : 0.f },
			firstAppSimStartTime { appSimStartTime} {}
		void UpdateSourceData(const PmNsmFrameData* pSourceFrameData_in,
			const PmNsmFrameData* pFrameDataOfNextDisplayed,
			const PmNsmFrameData* pFrameDataofLastPresented,
			const PmNsmFrameData* pFrameDataOfLastDisplayed,
			const PmNsmFrameData* pPreviousFrameDataOfLastDisplayed);
		// data
		const PmNsmFrameData* pSourceFrameData = nullptr;
		uint32_t sourceFrameDisplayIndex = 0;
		const double performanceCounterPeriodMs{};
		const uint64_t qpcStart{};
		bool dropped{};
		// Start qpc of the previous frame, displayed or not
		uint64_t cpuStart = 0;
		// The simulation start of the last displayed frame
		uint64_t previousDisplayedSimStartQpc = 0;
		// Start cpustart qpc of the previously displayed frame
		uint64_t lastDisplayedCpuStart = 0;
		// Screen time qpc of the previously displayed frame.
		uint64_t previousDisplayedQpc = 0;
		// Screen time qpc of the first display in the next displayed PmNsmFrameData
		uint64_t nextDisplayedQpc = 0;
		// Display index to attribute cpu work, gpu work, animation error and
		// input latency
		size_t appIndex = 0;
		// Click time qpc of non displayed frame
		uint64_t lastReceivedNotDisplayedClickQpc = 0;
		// All other input time qpc of non displayed frame
		uint64_t lastReceivedNotDisplayedAllInputTime = 0;
		// The first app sim start time
		uint64_t firstAppSimStartTime = 0;
	};
	// functions
	PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements);
	~PM_FRAME_QUERY();
	void GatherToBlob(Context& ctx, uint8_t* pDestBlob) const;
	size_t GetBlobSize() const;
	std::optional<uint32_t> GetReferencedDevice() const;

	PM_FRAME_QUERY(const PM_FRAME_QUERY&) = delete;
	PM_FRAME_QUERY& operator=(const PM_FRAME_QUERY&) = delete;
	PM_FRAME_QUERY(PM_FRAME_QUERY&&) = delete;
	PM_FRAME_QUERY& operator=(PM_FRAME_QUERY&&) = delete;

private:
	// functions
	std::unique_ptr<pmon::mid::GatherCommand_> MapQueryElementToGatherCommand_(const PM_QUERY_ELEMENT& q, size_t pos);
	// data
	std::vector<std::unique_ptr<pmon::mid::GatherCommand_>> gatherCommands_;
	size_t blobSize_ = 0;
	std::optional<uint32_t> referencedDevice_;
};
#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
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
		void UpdateSourceData(const PmNsmFrameData* pSourceFrameData_in)
		{
			pSourceFrameData = pSourceFrameData_in;
			dropped = pSourceFrameData->present_event.FinalState != PresentResult::Presented;
		}
		// data
		const PmNsmFrameData* pSourceFrameData = nullptr;
		const double performanceCounterPeriodMs;
		const uint64_t qpcStart;
		bool dropped;
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
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
	PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements);
	~PM_FRAME_QUERY();
	void GatherToBlob(const PmNsmFrameData* pSourceFrameData, uint8_t* pDestBlob) const;
	size_t GetBlobSize() const;
private:
	// functions
	std::unique_ptr<pmon::mid::GatherCommand_> MapQueryElementToGatherCommand_(const PM_QUERY_ELEMENT& q, size_t pos);
	// data
	std::vector<std::unique_ptr<pmon::mid::GatherCommand_>> gatherCommands_;
	size_t blobSize_ = 0;
	std::optional<uint32_t> referencedDevice_;
};
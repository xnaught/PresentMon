#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include <vector>
#include <span>

namespace pmapi::intro
{
	class Dataset;
}

struct PM_FRAME_EVENT_QUERY
{
public:
	PM_FRAME_EVENT_QUERY(std::span<PM_QUERY_ELEMENT> queryElements);
	void GatherToBlob(const uint8_t* sourceFrameData, uint8_t* destBlob) const;
	size_t GetBlobSize() const;
private:
	// types
	struct CopyCommand_
	{
		uint32_t offset;
		uint16_t padding;
		uint16_t size;
	};
	// functions
	CopyCommand_ MapQueryElementToCopyCommand_(const PM_QUERY_ELEMENT& q, size_t pos);
	// data
	std::vector<CopyCommand_> copyCommands_;
	size_t blobSize_ = 0;
};
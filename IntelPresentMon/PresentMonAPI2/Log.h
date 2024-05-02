#pragma once
#include <Memory>
#include "../CommonUtilities/log/IChannel.h"

namespace pmon::util::log
{
	void InjectCopyChannel(std::shared_ptr<IChannel> pCopyTargetChannel) noexcept;
}
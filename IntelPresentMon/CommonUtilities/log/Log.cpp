#include "Log.h"
#include "Channel.h"
#include "MsvcDebugDriver.h"
#include "SimpleFileDriver.h"
#include "TextFormatter.h"
#include <memory>
#include <shared_mutex>

namespace pmon::util::log
{
	std::shared_mutex channelMutex_;
	std::shared_ptr<IChannel> pChannel_;

	void InjectDefaultChannel(std::shared_ptr<IChannel> pChannel)
	{
		std::unique_lock lk{ channelMutex_ };
		pChannel_ = std::move(pChannel);
	}

	IChannel* GetDefaultChannel()
	{
		std::shared_lock lk{ channelMutex_ };
		// check if we have a channel
		if (!pChannel_) {
			// if no channel, switch from a shared to unique lock
			lk.unlock();
			std::unique_lock lk{ channelMutex_ };
			// make sure channel was not set in interim of switching lock
			if (!pChannel_) {
				// make the formatter
				const auto pFormatter = std::make_shared<TextFormatter>();
				// construct and configure default logging channel
				pChannel_ = std::make_shared<Channel>();
				pChannel_->AttachDriver(std::make_shared<MsvcDebugDriver>(pFormatter));
				pChannel_->AttachDriver(std::make_shared<SimpleFileDriver>("log.txt", pFormatter));
			}
		}
		return pChannel_.get();
	}
}
#include "Log.h"
#include "Channel.h"
#include "MsvcDebugDriver.h"
#include "BasicFileDriver.h"
#include "TextFormatter.h"
#include "SimpleFileStrategy.h"
#include <memory>
#include <shared_mutex>

namespace pmon::util::log
{
	IChannel* GetDefaultChannel() noexcept
	{
		try {
			// this call is a workaround to make sure CRT global objects required by
			// std::stacktrace are destroyed after channel
			if (auto dummyTrace = std::stacktrace::current(); !dummyTrace.empty()) {
				auto dummyDescription = dummyTrace[0].description();
			}
			// using static-local to sequence destruction of channel before destruction of
			// CRT lazy initialized objects requried by std::stacktrace
			static struct ChannelManager {
				Channel channel;
				ChannelManager() {
					// make the formatter
					const auto pFormatter = std::make_shared<TextFormatter>();
					const auto pFileStrategy = std::make_shared<SimpleFileStrategy>("log.txt");
					// construct and configure default logging channel
					channel.AttachDriver(std::make_shared<MsvcDebugDriver>(pFormatter));
					channel.AttachDriver(std::make_shared<BasicFileDriver>(pFormatter, pFileStrategy));
				}
			} channelManager;

			return &channelManager.channel;
		}
		catch (...) {
			return nullptr;
		}
	}
}
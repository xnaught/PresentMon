#include "Log.h"
#include "Channel.h"
#include "MsvcDebugDriver.h"
#include "BasicFileDriver.h"
#include "TextFormatter.h"
#include "SimpleFileStrategy.h"
#include <memory>
#include <shared_mutex>
#include <stacktrace>
#include "PanicLogger.h"
#include <future>

namespace pmon::util::log
{
	namespace
	{
		Channel* GetDefaultChannelImpl_() noexcept
		{
			try {
				// @SINGLETON
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
				pmlog_panic_(L"Exception thrown while getting default log channel");
				return nullptr;
			}
		}
	}
	IChannel* GetDefaultChannel() noexcept
	{
		return GetDefaultChannelImpl_();
	}
	void BootDefaultChannelEager() noexcept
	{
		std::thread{ [] {
			GlobalPolicy::GetLogLevel();
			GetDefaultChannel();
		} }.detach();
	}
	DefaultChannelManager::DefaultChannelManager()
	{
		BootDefaultChannelEager();
	}
	DefaultChannelManager::~DefaultChannelManager()
	{
		GlobalPolicy::SetResolveTraceInClientThread(true);
		GetDefaultChannelImpl_()->FlushEntryPointExit();
	}
}
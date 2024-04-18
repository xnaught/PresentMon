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
#include "IdentificationTable.h"
#include "LinePolicy.h"
#include "LineTable.h"

namespace pmon::util::log
{
	namespace
	{
		Channel* GetDefaultChannelImpl_() noexcept
		{
			try {
				// make sure ID table is booted
				IdentificationTable::LookupThread(0);
				// @SINGLETON
				static struct ChannelManager {
					Channel channel;
					ChannelManager() {
						// make the formatter
						const auto pFormatter = std::make_shared<TextFormatter>();
						const auto pFileStrategy = std::make_shared<SimpleFileStrategy>("log.txt");
						// make and add the line-tracking policy
						channel.AttachPolicy(std::make_shared<LinePolicy>());
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
			LineTable::TryLookup(L"", 0);
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
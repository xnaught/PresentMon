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
#include "ErrorCodeResolvePolicy.h"
#include "ErrorCodeResolver.h"
#include "../win/HrErrorCodeProvider.h"

namespace pmon::util::log
{
	IChannel* GetDefaultChannelWithFactory(std::function<std::shared_ptr<IChannel>()> factory) noexcept
	{
		try {
			// make sure singleton dependencies are booted
			GlobalPolicy::GetLogLevel();
			LineTable::TryLookup(L"", 0);
			IdentificationTable::LookupThread(0);
			// @SINGLETON
			static struct ChannelManager {
				std::shared_ptr<IChannel> pChannel;
				ChannelManager(std::function<std::shared_ptr<IChannel>()> factory)
					:
					pChannel{ factory() }
				{}
			} channelManager{ std::move(factory) };
			return channelManager.pChannel.get();
		}
		catch (...) {
			pmlog_panic_(L"Exception thrown while getting default log channel");
			return nullptr;
		}
	}
	void BootDefaultChannelEager() noexcept
	{
		std::thread{ [] {
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
		GetDefaultChannel()->FlushEntryPointExit();
	}
}
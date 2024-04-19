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
	namespace
	{
		Channel* GetDefaultChannelImpl_() noexcept
		{
			try {
				// make sure singleton dependencies are booted
				GlobalPolicy::GetLogLevel();
				LineTable::TryLookup(L"", 0);
				IdentificationTable::LookupThread(0);
				// @SINGLETON
				static struct ChannelManager {
					Channel channel;
					ChannelManager() {
						// error resolver
						auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
						pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
						// error resolving policy
						auto pErrPolicy = std::make_shared<ErrorCodeResolvePolicy>();
						pErrPolicy->SetResolver(std::move(pErrorResolver));
						channel.AttachPolicy(std::move(pErrPolicy));
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
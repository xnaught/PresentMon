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
		// calling stacktrace::description will cause some lazy-initialized objects to be
		// created in the CRT
		void PreloadStdStackTrace()
		{
			if (auto dummyTrace = std::stacktrace::current(); !dummyTrace.empty()) {
				auto dummyDescription = dummyTrace[0].description();
			}
		}
		std::once_flag preloadTraceOnce_;
	}
	void BootDefaultChannelEager() noexcept
	{
		std::thread{ [] {GetDefaultChannel(); } }.detach();
	}
	IChannel* GetDefaultChannel() noexcept
	{
		try {
			// this call is a workaround to make sure CRT global objects required by
			// std::stacktrace are destroyed after channel
			std::call_once(preloadTraceOnce_, PreloadStdStackTrace);
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
			pmlog_panic_(L"Exception thrown while getting default log channel");
			return nullptr;
		}
	}
}
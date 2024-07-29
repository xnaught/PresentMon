#include "LogSetup.h"
#include <chrono>
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/Channel.h"
#include "../CommonUtilities/log/MsvcDebugDriver.h"
#include "../CommonUtilities/log/BasicFileDriver.h"
#include "../CommonUtilities/log/StdioDriver.h"
#include "../CommonUtilities/log/TextFormatter.h"
#include "../CommonUtilities/log/SimpleFileStrategy.h"
#include "../CommonUtilities/log/LinePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolvePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolver.h"
#include "../CommonUtilities/win/HrErrorCodeProvider.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/Exception.h"
#include "CliOptions.h"

namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel()
		{
			GlobalPolicy::Get().SetSubsystem(Subsystem::Server);
			// channel (use custom deleter to ensure deletion in this module's heap)
			auto pChannel = std::shared_ptr<IChannel>{ new Channel{}, [](Channel* p) { delete p; } };
			// error resolver
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
			// error resolving policy
			auto pErrPolicy = std::make_shared<ErrorCodeResolvePolicy>();
			pErrPolicy->SetResolver(std::move(pErrorResolver));
			pChannel->AttachComponent(std::move(pErrPolicy));
			// make and add the line-tracking policy
			pChannel->AttachComponent(std::make_shared<LinePolicy>());
			// make the formatter
			const auto pFormatter = std::make_shared<TextFormatter>();
			// attach drivers
			// TODO: test if cwd is writeable, if not write to some other location (temp?)
			pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter), "drv:dbg");
			pChannel->AttachComponent(std::make_shared<StdioDriver>(pFormatter), "drv:std");

			return pChannel;
		}
	}

	// this is injected into to util::log namespace and hooks into that system
	std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeChannel);
	}
}

namespace logsetup
{
	using namespace ::pmon::util;
	using namespace ::pmon::util::log;

	void ConfigureLogging() noexcept
	{
		try {
			// shortcut for command line
			const auto& opt = clio::Options::Get();
			// get the channel
			auto pChannel = GetDefaultChannel();
			// configure logging based on command line
			if (opt.logLevel) {
				GlobalPolicy::Get().SetLogLevel(*opt.logLevel);
			}
			if (opt.disableStdioLog) {
				pChannel->AttachComponent({}, "drv:std");
			}
			if (opt.disableDebuggerLog) {
				pChannel->AttachComponent({}, "drv:dbg");
			}
			if (opt.logDir) {
				const std::chrono::zoned_time now{ std::chrono::current_zone(), std::chrono::system_clock::now() };
				auto fullPath = std::format("{0}\\pmsvc-log-{1:%y}{1:%m}{1:%d}-{1:%H}{1:%M}{1:%OS}.csv", *opt.logDir, now);
				const auto pFileStrategy = std::make_shared<SimpleFileStrategy>(fullPath);
				pChannel->AttachComponent(std::make_shared<BasicFileDriver>(
					std::make_shared<TextFormatter>(), pFileStrategy), "drv:file");
			}
		}
		catch (...) {}
	}

	LogChannelManager::LogChannelManager() noexcept
	{
		InstallSehTranslator();
		BootDefaultChannelEager();
	}
	LogChannelManager::~LogChannelManager()
	{
		FlushEntryPoint();
	}
}
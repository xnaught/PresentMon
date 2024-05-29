#include <CommonUtilities/log/Log.h>
#include <CommonUtilities/log/Channel.h>
#include <CommonUtilities/log/MsvcDebugDriver.h>
#include <CommonUtilities/log/BasicFileDriver.h>
#include <CommonUtilities/log/MarshallDriver.h>
#include <CommonUtilities/log/NamedPipeMarshallSender.h>
#include <CommonUtilities/log/TextFormatter.h>
#include <CommonUtilities/log/SimpleFileStrategy.h>
#include <CommonUtilities/log/LinePolicy.h>
#include <CommonUtilities/log/ErrorCodeResolvePolicy.h>
#include <CommonUtilities/log/ErrorCodeResolver.h>
#include <CommonUtilities/win/WinAPI.h>
#include <CommonUtilities/win/HrErrorCodeProvider.h>
#include <CommonUtilities/str/String.h>
#include <CommonUtilities/log/PanicLogger.h>
#include <PresentMonAPIWrapperCommon/PmErrorCodeProvider.h>
#include <PresentMonAPI2/Internal.h>
#include <Core/source/infra/util/FolderResolver.h>
#include "../cli/CliOptions.h"
#include "LogSetup.h"
#include <memory>
#include <format>
#include <chrono>
#include <filesystem>

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel()
		{
			GlobalPolicy::Get().SetSubsystem(Subsystem::IntelPresentmon);
			// channel (use custom deleter to ensure deletion in this module's heap)
			auto pChannel = std::shared_ptr<IChannel>{ new Channel{}, [](Channel* p) { delete p; } };
			// error resolver
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
			pErrorResolver->AddProvider(std::make_unique<pmapi::PmErrorCodeProvider>());
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
			const auto pFileStrategy = std::make_shared<SimpleFileStrategy>(std::format("pm-early-log-{}.txt", GetCurrentProcessId()));
			pChannel->AttachComponent(std::make_shared<BasicFileDriver>(pFormatter, pFileStrategy), "drv:file");

			return pChannel;
		}
	}

	// this is injected into to util::log namespace and hooks into that system
	std::shared_ptr<IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeChannel);
	}
}

namespace p2c
{
	using namespace pmon::util;
	using namespace pmon::util::log;

	void ConfigureLogging() noexcept
	{
		try {
			auto pChan = GetDefaultChannel();
			// shortcut for command line
			const auto& opt = cli::Options::Get();
			const auto render = opt.cefType && *opt.cefType == "renderer";
			// connect dll channel and id table to exe, get access to global settings in dll
			LoggingSingletons getters;
			if (render) {
				getters = pmLinkLogging_(pChan, []() -> IdentificationTable& {
					return IdentificationTable::Get_(); });
			}

			// configure logging based on command line
			{
				auto logfileName = bool(opt.cefType) ? std::format(L"log-{}-{}.txt", 
					str::ToWide(*opt.cefType), GetCurrentProcessId()) : L"log.txt"s;
				std::wstring path;
				if (opt.logFolder) {
					path = std::format(L"{}\\{}", str::ToWide(*opt.logFolder), logfileName);
				}
				else {
					path = infra::util::FolderResolver::Get().Resolve(
						infra::util::FolderResolver::Folder::App,
						std::format(L"logs\\{}", logfileName));
				}
				pChan->AttachComponent(std::make_shared<BasicFileDriver>(std::make_shared<TextFormatter>(),
					std::make_shared<SimpleFileStrategy>(path)), "drv:file");
			}
			if (opt.logLevel) {
				GlobalPolicy::Get().SetLogLevel(*opt.logLevel);
				if (render) {
					getters.getGlobalPolicy().SetLogLevel(*opt.logLevel);
				}
			}
			if (opt.logTraceLevel) {
				GlobalPolicy::Get().SetTraceLevel(*opt.logTraceLevel);
				if (render) {
					getters.getGlobalPolicy().SetTraceLevel(*opt.logTraceLevel);
				}
			}
			if (opt.traceExceptions) {
				GlobalPolicy::Get().SetExceptionTrace(*opt.traceExceptions);
				GlobalPolicy::Get().SetSehTracing(*opt.traceExceptions);
				if (render) {
					getters.getGlobalPolicy().SetExceptionTrace(*opt.traceExceptions);
					getters.getGlobalPolicy().SetSehTracing(*opt.traceExceptions);
				}
			}
			if (opt.logDenyList) {
				pmquell(LineTable::IngestList(str::ToWide(*opt.logDenyList), true));
				if (render) {
					pmquell(getters.getLineTable().IngestList_(str::ToWide(*opt.logDenyList), true));
				}
			}
			else if (opt.logAllowList) {
				pmquell(LineTable::IngestList(str::ToWide(*opt.logAllowList), false));
				if (render) {
					pmquell(getters.getLineTable().IngestList_(str::ToWide(*opt.logAllowList), false));
				}
			}
			// setup server to ipc logging connection for child processes (renderer)
			if (opt.logPipeName) {
				try {
					auto pSender = std::make_shared<log::NamedPipeMarshallSender>(str::ToWide(*opt.logPipeName), 1);
					log::IdentificationTable::RegisterSink(pSender);
					auto pDriver = std::make_shared<log::MarshallDriver>(pSender);
					pChan->AttachComponent(std::move(pDriver));
					// block here waiting for the browser process to connect
					if (!pSender->WaitForConnection(1500ms)) {
						pmlog_warn(L"browser process did not connect to ipc logging pipe before timeout (it might have connected later)");
					}
					else {
						// remove independent output drivers (only copy to main process via IPC)
						pChan->AttachComponent({}, "drv:dbg");
						pChan->AttachComponent({}, "drv:file");
					}
				}
				catch (...) {
					pmlog_panic_(ReportExceptionWide());
				}
			}
		}
		catch (...) {
			pmlog_panic_(ReportExceptionWide());
		}
	}
	LogChannelManager::LogChannelManager() noexcept
	{
		InstallSehTranslator();
		BootDefaultChannelEager();
	}
	LogChannelManager::~LogChannelManager()
	{
		pmFlushEntryPoint_();
		FlushEntryPoint();
	}
}
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/Channel.h"
#include "../CommonUtilities/log/MsvcDebugDriver.h"
#include "../CommonUtilities/log/BasicFileDriver.h"
#include "../CommonUtilities/log/TextFormatter.h"
#include "../CommonUtilities/log/SimpleFileStrategy.h"
#include "../CommonUtilities/log/LinePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolvePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolver.h"
#include "../CommonUtilities/win/HrErrorCodeProvider.h"
#include "../PresentMonAPIWrapperCommon/PmErrorCodeProvider.h"
#include "../CommonUtilities/log/CopyDriver.h"
#include "../CommonUtilities/log/DiagnosticDriver.h"
#include "LogSetup.h"
#include <memory>


namespace pmon::util::log
{
	namespace
	{
		// cache reference to diagnostics system here for C-Api interaction and attachment to new channels
		std::shared_ptr<DiagnosticDriver> pDiagnostics_;
		std::mutex diagnosticsMtx_;

		// creates an independent logging channel or a copy channel, resets log level to default
		std::shared_ptr<IChannel> MakeChannel_(std::shared_ptr<IChannel> pCopyTargetChannel = {}) noexcept
		{
			try {
				// channel
				auto pChannel = std::make_shared<Channel>();
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
				// configure drivers
				if (pCopyTargetChannel) {
					pChannel->AttachComponent(std::make_shared<CopyDriver>(std::move(pCopyTargetChannel)));
				}
				else {
					// make the formatter and file strategy
					const auto pFormatter = std::make_shared<TextFormatter>();
					const auto pFileStrategy = std::make_shared<SimpleFileStrategy>("log-pmapi-dll.txt");
					pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter), "drv:dbg");
					pChannel->AttachComponent(std::make_shared<BasicFileDriver>(pFormatter, pFileStrategy), "drv:file");
				}
				// connect diagnostic driver if present to new channel
				{
					std::lock_guard lk{ diagnosticsMtx_ };
					if (pDiagnostics_) {
						pChannel->AttachComponent(pDiagnostics_, "drv:diag");
					}
				}
				return pChannel;
			}
			catch (...) {
				return {};
			}
		}
		// creates a channel for debug diagnostic purposes
		std::shared_ptr<IChannel> MakeDiagnosticChannel_(std::shared_ptr<DiagnosticDriver> pDiag)
		{
			// channel
			auto pChannel = std::make_shared<Channel>();
			// make and add the line-tracking policy
			pChannel->AttachComponent(std::make_shared<LinePolicy>());
			// configure drivers
			pChannel->AttachComponent(pDiag, "drv:diag");
			return pChannel;
		}
		// creates a null channel and configures logging to be disabled as much as possible
		std::shared_ptr<IChannel> MakeNullChannel_()
		{
			// disable logging globally
			GlobalPolicy::Get().SetLogLevel(Level::None);
			return {};
		}
	}

	std::shared_ptr<IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeNullChannel_);
	}

	void InjectCopyChannel(std::shared_ptr<IChannel> pCopyTargetChannel) noexcept
	{
		// reset logging level when channel is explicitly requested
		GlobalPolicy::Get().SetLogLevelDefault();
		GlobalPolicy::Get().SetTraceLevelDefault();
		InjectDefaultChannel(MakeChannel_(std::move(pCopyTargetChannel)));
	}

	void InjectStandaloneChannel() noexcept
	{
		// reset logging level when channel is explicitly requested
		GlobalPolicy::Get().SetLogLevelDefault();
		GlobalPolicy::Get().SetTraceLevelDefault();
		InjectDefaultChannel(MakeChannel_());
	}

	void SetupDiagnosticLayer(const PM_DIAGNOSTIC_CONFIGURATION* pConfig)
	{
		// create / replace diagnostic driver
		{
			std::lock_guard lk{ diagnosticsMtx_ };
			pDiagnostics_ = std::make_shared<log::DiagnosticDriver>(pConfig);
		}
		// attach to existing channel if present
		if (auto pChan = GetDefaultChannel()) {
			pChan->AttachComponent(pDiagnostics_, "drv:diag");
		}
		else { // otherwise make a minimal channel for diagnostic handling
			InjectDefaultChannel(MakeDiagnosticChannel_(pDiagnostics_));
		}
		GlobalPolicy::Get().SetLogLevel((Level)pConfig->filterLevel);
		if (!pConfig->enableTrace) {
			GlobalPolicy::Get().SetTraceLevel(Level::None);
		}
	}

	std::shared_ptr<class DiagnosticDriver> GetDiagnostics()
	{
		std::lock_guard lk{ diagnosticsMtx_ };
		return pDiagnostics_;
	}
}
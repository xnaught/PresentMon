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
		// creates an independent logging channel or a copy channel, resets log level to default
		std::shared_ptr<IChannel> MakeChannel_(std::shared_ptr<IChannel> pCopyTargetChannel = {})
		{
			GlobalPolicy::Get().SetLogLevelDefault();
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
			return pChannel;
		}
		// creates a channel for debug diagnostic purposes
		std::shared_ptr<IChannel> MakeDiagnosticChannel_(std::shared_ptr<DiagnosticDriver> pDiag)
		{
			GlobalPolicy::Get().SetLogLevelDefault();
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
			pChannel->AttachComponent(pDiag, "drv:diag");
			return pChannel;
		}
		// creates a null channel and configures logging to be disabled as much as possible
		std::shared_ptr<IChannel> MakeNullChannel_()
		{
			GlobalPolicy::Get().SetLogLevel(Level::None);
			return {};
		}
		// cache reference to diagnostics system here for C-Api interaction
		std::shared_ptr<DiagnosticDriver> pDiagnostics_;
	}

	std::shared_ptr<IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeNullChannel_);
	}

	void InjectCopyChannel(std::shared_ptr<IChannel> pCopyTargetChannel) noexcept
	{
		InjectDefaultChannel(MakeChannel_(std::move(pCopyTargetChannel)));
	}

	void InjectStandaloneChannel() noexcept
	{
		InjectDefaultChannel(MakeChannel_());
	}

	void SetupDiagnosticLayer(const PM_DIAGNOSTIC_CONFIGURATION* pConfig)
	{
		if (!pDiagnostics_) {
			pDiagnostics_ = std::make_shared<log::DiagnosticDriver>(pConfig);
		}
		if (auto pChan = GetDefaultChannel()) {
			pChan->AttachComponent(pDiagnostics_, "drv:diag");
		}
		else {
			InjectDefaultChannel(MakeDiagnosticChannel_(pDiagnostics_));
		}
	}

	std::shared_ptr<class DiagnosticDriver> GetDiagnostics()
	{
		return pDiagnostics_;
	}
}
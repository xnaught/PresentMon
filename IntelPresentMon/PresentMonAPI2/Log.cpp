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
			// construct and configure default logging channel
			if (pCopyTargetChannel) {
				pChannel->AttachComponent(std::make_shared<CopyDriver>(std::move(pCopyTargetChannel)));
			}
			else {
				// make the formatter and file strategy
				const auto pFormatter = std::make_shared<TextFormatter>();
				const auto pFileStrategy = std::make_shared<SimpleFileStrategy>("log-pmapi-dll.txt");
				pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter));
				pChannel->AttachComponent(std::make_shared<BasicFileDriver>(pFormatter, pFileStrategy));
			}
			return pChannel;
		}
		// creates a null channel and configures logging to be disabled as much as possible
		std::shared_ptr<IChannel> MakeNullChannel_()
		{
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
		InjectDefaultChannel(MakeChannel_(std::move(pCopyTargetChannel)));
	}

	void InjectStandaloneChannel() noexcept
	{
		InjectDefaultChannel(MakeChannel_());
	}
}
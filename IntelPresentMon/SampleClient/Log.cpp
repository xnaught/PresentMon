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
#include <memory>


namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel()
		{
			// channel
			auto pChannel = std::make_shared<Channel>();
			// error resolver
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
			pErrorResolver->AddProvider(std::make_unique<pmapi::PmErrorCodeProvider>());
			// error resolving policy
			auto pErrPolicy = std::make_shared<ErrorCodeResolvePolicy>();
			pErrPolicy->SetResolver(std::move(pErrorResolver));
			pChannel->AttachPolicy(std::move(pErrPolicy));
			// make the formatter
			const auto pFormatter = std::make_shared<TextFormatter>();
			const auto pFileStrategy = std::make_shared<SimpleFileStrategy>("log.txt");
			// make and add the line-tracking policy
			pChannel->AttachPolicy(std::make_shared<LinePolicy>());
			// construct and configure default logging channel
			pChannel->AttachDriver(std::make_shared<MsvcDebugDriver>(pFormatter));
			pChannel->AttachDriver(std::make_shared<BasicFileDriver>(pFormatter, pFileStrategy));
			return pChannel;
		}
	}

	IChannel* GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeChannel);
	}
}
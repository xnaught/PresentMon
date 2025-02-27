#include "../IntelPresentMon/CommonUtilities/log/Log.h"
#include "../IntelPresentMon/CommonUtilities/log/Channel.h"
#include "../IntelPresentMon/CommonUtilities/log/MsvcDebugDriver.h"
#include "../IntelPresentMon/CommonUtilities/log/BasicFileDriver.h"
#include "../IntelPresentMon/CommonUtilities/log/StdioDriver.h"
#include "../IntelPresentMon/CommonUtilities/log/TextFormatter.h"
#include "../IntelPresentMon/CommonUtilities/log/SimpleFileStrategy.h"
#include "../IntelPresentMon/CommonUtilities/log/LinePolicy.h"
#include "../IntelPresentMon/CommonUtilities/log/ErrorCodeResolvePolicy.h"
#include "../IntelPresentMon/CommonUtilities/log/ErrorCodeResolver.h"
#include "../IntelPresentMon/CommonUtilities/log/ChannelFlusher.h"
#include "../IntelPresentMon/CommonUtilities/log/NamedPipeMarshallSender.h"
#include "../IntelPresentMon/CommonUtilities/log/MarshallDriver.h"
#include "../IntelPresentMon/CommonUtilities/win/HrErrorCodeProvider.h"
#include "../IntelPresentMon/CommonUtilities/str/String.h"
#include "../IntelPresentMon/CommonUtilities/Exception.h"
#include "../IntelPresentMon/ControlLib/IgclErrorCodeProvider.h"
#include <chrono>

namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel()
		{
			// make the channel
			auto pChannel = std::make_shared<Channel>();
			// error resolver
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
			// error resolving policy
			auto pErrPolicy = std::make_shared<ErrorCodeResolvePolicy>();
			pErrPolicy->SetResolver(std::move(pErrorResolver));
			pChannel->AttachComponent(std::move(pErrPolicy));
			// make and add the line-tracking policy
			pChannel->AttachComponent(std::make_shared<LinePolicy>());
			// attach drivers
			// only debugger logging for now in opm because file logging would need options support in the cli
			// and stdio is not compatible with the non-standard console output that opm does
			pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(std::make_shared<TextFormatter>()), "drv:dbg");
			// flusher
			pChannel->AttachComponent(std::make_shared<ChannelFlusher>(pChannel), "obj:fsh");

			return pChannel;
		}
	}

	// this is injected into to util::log namespace and hooks into that system
	std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeChannel);
	}
}
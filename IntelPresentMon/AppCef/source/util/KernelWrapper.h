#pragma once
#include <memory>
#include "HotkeyListener.h"
#include "SignalManager.h"
#include "AsyncEndpointManager.h"
#include <Interprocess/source/act/SymmetricActionClient.h>
#include <Interprocess/source/act/SymmetricActionServer.h>
#include "cact/CefExecutionContext.h"
#include "kact/KernelExecutionContext.h"
#include "kact/OpenSession.h"

namespace p2c::kern
{
    class Kernel;
    class KernelHandler;
}

namespace p2c::client::util
{
    using namespace ::pmon::ipc;

    using KernelServer = act::SymmetricActionServer<kact::KernelExecutionContext>;

    class CefClient : public act::SymmetricActionClient<cact::CefExecutionContext>
    {
    public:
        CefClient(std::string pipeName, cact::CefExecutionContext context = {})
            :
            SymmetricActionClient{ std::move(pipeName), std::move(context) }
        {
            auto res = DispatchSync(kact::OpenSession::Params{ GetCurrentProcessId() });
            EstablishSession_(res.kernelPid);
        }
    };

	struct KernelWrapper
	{
        KernelWrapper();
        ~KernelWrapper();
        util::SignalManager signals;
        util::AsyncEndpointManager asyncEndpoints;
        std::unique_ptr<kern::Kernel> pKernel;
        std::unique_ptr<kern::KernelHandler> pKernelHandler;
        std::unique_ptr<KernelServer> pServer;
        std::unique_ptr<CefClient> pClient;
        std::unique_ptr<Hotkeys> pHotkeys;
	};
}

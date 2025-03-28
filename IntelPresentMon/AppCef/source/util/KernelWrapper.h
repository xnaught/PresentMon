#pragma once
#include <memory>
#include "HotkeyListener.h"
#include "SignalManager.h"
#include "AsyncEndpointManager.h"
#include <Interprocess/source/act/SymmetricActionClient.h>
#include <Interprocess/source/act/SymmetricActionServer.h>
#include "cact/CefExecutionContext.h"
#include "kact/KernelExecutionContext.h"

namespace p2c::kern
{
    class Kernel;
    class KernelHandler;
}

namespace p2c::client::util
{
    using namespace ::pmon::ipc;
	struct KernelWrapper
	{
        KernelWrapper();
        ~KernelWrapper();
        util::SignalManager signals;
        util::AsyncEndpointManager asyncEndpoints;
        std::unique_ptr<kern::Kernel> pKernel;
        std::unique_ptr<kern::KernelHandler> pKernelHandler;
        std::unique_ptr<act::SymmetricActionServer<kact::KernelExecutionContext>> pServer;
        std::unique_ptr<act::SymmetricActionClient<cact::CefExecutionContext>> pClient;
        std::unique_ptr<Hotkeys> pHotkeys;
	};
}

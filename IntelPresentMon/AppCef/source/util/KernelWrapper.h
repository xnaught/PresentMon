#pragma once
#include <memory>
#include "HotkeyListener.h"
#include "SignalManager.h"
#include "AsyncEndpointManager.h"
#include "ActionClientServer.h"
#include "IpcInvocationManager.h"


namespace p2c::client::util
{
	struct KernelWrapper
	{
        std::unique_ptr<Hotkeys> pHotkeys;
        util::SignalManager signals;
        util::AsyncEndpointManager asyncEndpoints;
        std::unique_ptr<CefClient> pClient;
        std::unique_ptr<IpcInvocationManager> pInvocationManager;
	};
}

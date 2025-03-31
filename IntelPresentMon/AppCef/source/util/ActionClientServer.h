#pragma once
#include <Interprocess/source/act/SymmetricActionClient.h>
#include <Interprocess/source/act/SymmetricActionServer.h>
#include "cact/CefExecutionContext.h"
#include "kact/KernelExecutionContext.h"
#include "kact/OpenSession.h"

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
}
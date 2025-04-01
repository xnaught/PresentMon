#include "ActionClientServer.h"
#include "kact/OpenSession.h"

namespace p2c::client::util
{
    CefClient::CefClient(std::string pipeName, cact::CefExecutionContext context)
        :
        SymmetricActionClient{ std::move(pipeName), std::move(context) }
    {
        auto res = DispatchSync(kact::OpenSession::Params{ GetCurrentProcessId() });
        EstablishSession_(res.kernelPid);
    }
}
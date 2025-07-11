#pragma once
#include "../../../Interprocess/source/act/SymmetricActionClient.h"
#include <cstdint>
#include <string>
#include <optional>
#include <memory>

namespace p2c::iact
{
    using namespace ::pmon;
    using namespace util;

    // define minimial context for client side connection
    struct InjectionLibExecutionContext;
    struct InjectionLibSessionContext
    {
        // common session context items
        std::unique_ptr<ipc::act::SymmetricActionConnector<InjectionLibExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;
    };
    struct InjectionLibExecutionContext
    {
        // types
        using SessionContextType = InjectionLibSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;
    };

    using ClientBase = ipc::act::SymmetricActionClient<InjectionLibExecutionContext>;
    class ActionClient : public ClientBase
    {
    public:
        ActionClient(const std::string& pipeName);
    };
}
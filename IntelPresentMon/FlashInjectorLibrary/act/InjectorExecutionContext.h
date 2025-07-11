#pragma once
#include "../../Interprocess/source/act/SymmetricActionConnector.h"
#include "../../Interprocess/source/act/SymmetricActionServer.h"
#include <memory>
#include <optional>
#include <cstdint>
#include "../Extension/OverlayConfig.h"

namespace inj::act
{
    using namespace ::pmon;
    struct InjectorExecutionContext;

    struct InjectorSessionContext
    {
        // common session context items
        std::unique_ptr<ipc::act::SymmetricActionConnector<InjectorExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;
    };

    struct InjectorExecutionContext
    {
        // types
        using SessionContextType = InjectorSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;
    };

    using ActionServer = ipc::act::SymmetricActionServer<InjectorExecutionContext>;
}
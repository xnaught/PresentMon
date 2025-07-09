#pragma once
#include "../../Interprocess/source/act/SymmetricActionConnector.h"
#include <memory>
#include <optional>
#include <cstdint>

namespace inj::iact
{
    using namespace ::pmon;
    struct InjectorExecutionContext;

    struct InjectorSessionContext
    {
        enum class Type
        {
            Library,
            Kernel
        };
        // common session context items
        std::unique_ptr<ipc::act::SymmetricActionConnector<InjectorExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;
        Type clientType;
    };

    struct InjectorExecutionContext
    {
        // types
        using SessionContextType = InjectorSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;
    };
}
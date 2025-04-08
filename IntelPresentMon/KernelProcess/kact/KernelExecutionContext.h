#pragma once
#include "../../Interprocess/source/act/SymmetricActionConnector.h"
#include "../../CommonUtilities/pipe/ManualAsyncEvent.h"
#include <memory>
#include <set>
#include <unordered_map>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include "../../Core/source/kernel/Kernel.h"

namespace kproc::kact
{
    using namespace ::pmon;
    struct KernelExecutionContext;

    struct KernelSessionContext
    {
        std::unique_ptr<ipc::act::SymmetricActionConnector<KernelExecutionContext>> pConn;
        uint32_t remotePid = 0;
        std::optional<uint32_t> lastTokenSeen;
        uint32_t nextCommandToken = 0;
        std::chrono::high_resolution_clock::time_point lastReceived;
        // std::chrono::high_resolution_clock::time_point lastSent;
        // uint32_t sendCount = 0;
        uint32_t receiveCount = 0;
        uint32_t errorCount = 0;
    };

    struct KernelExecutionContext
    {
        // types
        using SessionContextType = KernelSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;

        p2c::kern::Kernel** ppKernel = nullptr;
    };
}
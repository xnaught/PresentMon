#pragma once
#include "../CommonUtilities/pipe/Pipe.h"
#include <memory>
#include <set>
#include <unordered_map>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include "PresentMon.h"
#include "Service.h"


namespace pmon::svc
{
    struct ActionSession
    {
        std::shared_ptr<util::pipe::DuplexPipe> pPipe;
        uint32_t clientPid = 0;
        std::set<uint32_t> trackedPids;
        std::optional<uint32_t> requestedAdapterId;
        std::optional<uint32_t> requestedTelemetryPeriodMs;
        std::optional<uint32_t> lastTokenSeen;
        // uint32_t nextCommandToken = 0;
        std::chrono::high_resolution_clock::time_point lastReceived;
        // std::chrono::high_resolution_clock::time_point lastSent;
        // uint32_t sendCount = 0;
        uint32_t receiveCount = 0;
        uint32_t errorCount = 0;
        std::string clientBuildId;
    };

    struct ServiceExecutionContext
    {
        using SessionContextType = ActionSession;
        Service* pSvc;
        PresentMon* pPmon;
        // maps session uid => session (uid is same as session pipe id)
        std::unordered_map<uint32_t, SessionContextType> sessions;
    };
}
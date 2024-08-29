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
        std::optional<uint32_t> requestedEtwFlushPeriodMs;
        std::optional<bool> requestedEtwFlushEnabled;
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
        // types
        using SessionContextType = ActionSession;

        // data
        Service* pSvc;
        PresentMon* pPmon;
        uint32_t responseWriteTimeoutMs = 0;
        // maps session uid => session (uid is same as session pipe id)
        std::unordered_map<uint32_t, SessionContextType> sessions;

        // functions
        void DisposeSession(uint32_t sessionId)
        {
            pmlog_dbg(std::format("Disposing session id:{}", sessionId));
            if (auto i = sessions.find(sessionId); i != sessions.end()) {
                auto& session = i->second;
                if (session.clientPid) {
                    for (auto& tracked : session.trackedPids) {
                        pPmon->StopStreaming(session.clientPid, tracked);
                    }
                }
                sessions.erase(i);
            }
            else {
                pmlog_warn("Session to be removed not found");
            }
        }
    };
}
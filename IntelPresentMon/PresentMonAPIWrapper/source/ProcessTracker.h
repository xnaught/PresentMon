#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include <format>
#include <string>
#include <memory>

namespace pmapi
{
    class ProcessTracker
    {
        friend class Session;
    public:
        uint32_t GetPid() const
        {
            return pid_;
        }
        ~ProcessTracker()
        {
            pmStopTrackingProcess(pid_);
        }
    private:
        ProcessTracker(uint32_t pid) : pid_{ pid }
        {
            if (auto sta = pmStartTrackingProcess(pid_); sta != PM_STATUS_SUCCESS) {
                throw Exception{ std::format("start process tracking call failed with error id={}", (int)sta) };
            }
        }
        uint32_t pid_;
    };
}

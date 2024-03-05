#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../PresentMonAPIWrapperCommon/Exception.h"
#include "../Interprocess/source/IntrospectionDataTypeMapping.h"
#include <format>
#include <string>
#include <memory>
#include <cassert>

namespace pmapi
{
    class ProcessTracker
    {
        friend class Session;
    public:
        ProcessTracker() = default;
        ~ProcessTracker() { Reset(); }
        ProcessTracker(ProcessTracker&& other) noexcept
        {
            *this = std::move(other);
        }
        ProcessTracker& operator=(ProcessTracker&& rhs) noexcept
        {
            pid_ = rhs.pid_;
            hSession_ = rhs.hSession_;
            rhs.Clear_();;
            return *this;
        }
        uint32_t GetPid() const
        {
            assert(!Empty());
            return pid_;
        }
        void Reset() noexcept
        {
            if (!Empty()) {
                // TODO: report error here noexcept
                pmStopTrackingProcess(hSession_, pid_);
            }
            Clear_();
        }
        bool Empty() const
        {
            return hSession_ == nullptr;
        }
        operator bool() const { return !Empty(); }
    private:
        // functions
        ProcessTracker(PM_SESSION_HANDLE hSession, uint32_t pid)
            :
            pid_{ pid },
            hSession_{ hSession }
        {
            if (auto sta = pmStartTrackingProcess(hSession_, pid_); sta != PM_STATUS_SUCCESS) {
                throw ApiErrorException{ sta, "start process tracking call failed" };
            }
        }
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept
        {
            pid_ = 0;
            hSession_ = nullptr;
        }
        // data
        uint32_t pid_ = 0u;
        PM_SESSION_HANDLE hSession_ = nullptr;
    };
}

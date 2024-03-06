#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmapi
{
    class ProcessTracker
    {
        friend class Session;
    public:
        ProcessTracker() = default;
        ~ProcessTracker();
        ProcessTracker(ProcessTracker&& other) noexcept;
        ProcessTracker& operator=(ProcessTracker&& rhs) noexcept;
        uint32_t GetPid() const;
        void Reset() noexcept;
        bool Empty() const;
        operator bool() const;
    private:
        // functions
        ProcessTracker(PM_SESSION_HANDLE hSession, uint32_t pid);
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept;
        // data
        uint32_t pid_ = 0u;
        PM_SESSION_HANDLE hSession_ = nullptr;
    };
}

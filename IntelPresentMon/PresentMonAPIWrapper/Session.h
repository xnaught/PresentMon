#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "ProcessTracker.h"
#include "FrameQuery.h"
#include "DynamicQuery.h"
#include <string>
#include <memory>
#include <span>

namespace pmapi
{
    class Session
    {
    public:
        Session();
        Session(std::string controlPipe, std::string introspectionNsm);
        Session(Session&& rhs) noexcept;
        Session& operator=(Session&& rhs) noexcept;
        ~Session();
        void Reset() noexcept;
        bool Empty() const;
        operator bool() const;
        std::shared_ptr<intro::Root> GetIntrospectionRoot() const;
        ProcessTracker TrackProcess(uint32_t pid);
        DynamicQuery RegisterDyanamicQuery(std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs);
        FrameQuery RegisterFrameQuery(std::span<PM_QUERY_ELEMENT> elements);
        void SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t milliseconds);
        // it is recommended to use the Session member functions instead of using this handle directly
        PM_SESSION_HANDLE GetHandle() const;
    private:
        // functions
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept;
        // data
        PM_SESSION_HANDLE handle_ = nullptr;
        mutable std::shared_ptr<intro::Root> pIntrospectionRootCache_;
    };
}

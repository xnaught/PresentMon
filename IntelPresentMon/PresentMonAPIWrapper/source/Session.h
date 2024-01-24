#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include <format>
#include <string>
#include <memory>
#include "ProcessTracker.h"
#include "FrameQuery.h"
#include "DynamicQuery.h"

namespace pmapi
{
    class SessionException : public Exception { using Exception::Exception; };

    class Session
    {
    public:
        Session();
        Session(std::string controlPipe, std::string introspectionNsm);
        Session(Session&& rhs) noexcept
            :
            handle_{ rhs.handle_ }
        {
            rhs.handle_ = nullptr;
        }
        Session& operator=(Session&& rhs) noexcept
        {
            handle_ = rhs.handle_;
            rhs.handle_ = nullptr;
            return *this;
        }
        ~Session()
        {
            if (handle_) {
                // TODO: report error noexcept
                pmCloseSession(handle_);
            }
        }
        bool Empty() const
        {
            return handle_ == nullptr;
        }
        operator bool() const { return !Empty(); }
        std::shared_ptr<intro::Root> GetIntrospectionRoot() const
        {
            // TODO: consider whether this should be an assertion
            if (!handle_) {
                throw SessionException{ "introspection call failed due to empty session object" };
            }
            if (pIntrospectionRootCache_) {
                return pIntrospectionRootCache_;
            }
            const PM_INTROSPECTION_ROOT* pRoot{};
            if (auto sta = pmGetIntrospectionRoot(handle_, &pRoot); sta != PM_STATUS_SUCCESS) {
                throw SessionException{ std::format("introspection call failed with error id={}", (int)sta) };
            }
            return pIntrospectionRootCache_ = std::make_shared<intro::Root>(pRoot, [](const PM_INTROSPECTION_ROOT* ptr) { pmFreeIntrospectionRoot(ptr); });
        }
        ProcessTracker TrackProcess(uint32_t pid)
        {
            return { handle_, pid };
        }
        DynamicQuery RegisterDyanamicQuery(std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs)
        {
            return { handle_, elements, winSizeMs, metricOffsetMs };
        }
        FrameQuery RegisterFrameQuery(std::span<PM_QUERY_ELEMENT> elements)
        {
            return { handle_, elements };
        }
        void SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t milliseconds)
        {
            if (auto sta = pmSetTelemetryPollingPeriod(handle_, deviceId, milliseconds); sta != PM_STATUS_SUCCESS) {
                throw SessionException{ std::format("set telemetry period call failed with error id={}", (int)sta) };
            }
        }
        // it is recommended to use the Session member functions instead of using this handle directly
        PM_SESSION_HANDLE GetHandle() const
        {
            return handle_;
        }
    private:
        PM_SESSION_HANDLE handle_ = nullptr;
        mutable std::shared_ptr<intro::Root> pIntrospectionRootCache_;
    };
}

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
                pmCloseSession(handle_);
            }
        }
        std::shared_ptr<intro::Root> GetIntrospectionRoot() const
        {
            // throw an exception on error or non-token
            if (!handle_) {
                throw SessionException{ "introspection call failed due to empty session object" };
            }
            const PM_INTROSPECTION_ROOT* pRoot{};
            if (auto sta = pmGetIntrospectionRoot(handle_, &pRoot); sta != PM_STATUS_SUCCESS) {
                throw SessionException{ std::format("introspection call failed with error id={}", (int)sta) };
            }
            return std::make_shared<intro::Root>(pRoot, [](const PM_INTROSPECTION_ROOT* ptr) { pmFreeIntrospectionRoot(ptr); });
        }
        std::shared_ptr<ProcessTracker> TrackProcess(uint32_t pid)
        {
            return std::shared_ptr<ProcessTracker>{ new ProcessTracker{ handle_, pid } };
        }
        std::shared_ptr<DynamicQuery> RegisterDyanamicQuery(std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs)
        {
            return std::shared_ptr<DynamicQuery>{ new DynamicQuery{ handle_, elements, winSizeMs, metricOffsetMs } };
        }
        std::shared_ptr<FrameQuery> RegisterFrameQuery(std::span<PM_QUERY_ELEMENT> elements)
        {
            return std::shared_ptr<FrameQuery>{ new FrameQuery{ handle_, elements } };
        }
        void SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t milliseconds)
        {
            if (auto sta = pmSetTelemetryPollingPeriod(handle_, deviceId, milliseconds); sta != PM_STATUS_SUCCESS) {
                throw SessionException{ std::format("set telemetry period call failed with error id={}", (int)sta) };
            }
        }
    private:
        PM_SESSION_HANDLE handle_ = nullptr;
    };
}

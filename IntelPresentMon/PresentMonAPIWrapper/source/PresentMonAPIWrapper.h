#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include <format>
#include <string>
#include <memory>

namespace pmapi
{
    class SessionException : public Exception { using Exception::Exception; };

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
            pmStartTrackingProcess(pid_);
        }
        uint32_t pid_;
    };

	class Session
	{
    public:
        // types
        static constexpr struct {} emptyCtorToken{};
        // functions
        Session();
        Session(decltype(emptyCtorToken)) : token_{ false } {}
        Session(std::string controlPipe, std::string introspectionNsm);
        Session(Session&& rhs) noexcept
            :
            token_{ rhs.token_ }
        {
            rhs.token_ = false;
        }
        Session& operator=(Session&& rhs) noexcept
        {
            token_ = rhs.token_;
            rhs.token_ = false;
            return *this;
        }
        ~Session()
        {
            if (token_) {
                pmCloseSession();
            }
        }
        intro::Dataset GetIntrospectionDataset() const
        {
            // throw an exception on error or non-token
            if (!token_) {
                throw SessionException{ "introspection call failed due to empty session object" };
            }
            const PM_INTROSPECTION_ROOT* pRoot{};
            if (auto sta = pmGetIntrospectionRoot(&pRoot); sta != PM_STATUS_SUCCESS) {
                throw SessionException{ std::format("introspection call failed with error id={}", (int)sta) };
            }
            return { pRoot, [](const PM_INTROSPECTION_ROOT* ptr) { pmFreeIntrospectionRoot(ptr); } };
        }
        std::shared_ptr<ProcessTracker> TrackProcess(uint32_t pid)
        {
            return std::shared_ptr<ProcessTracker>{ new ProcessTracker{ pid } };
        }
    private:
        bool token_ = true;
	};
}
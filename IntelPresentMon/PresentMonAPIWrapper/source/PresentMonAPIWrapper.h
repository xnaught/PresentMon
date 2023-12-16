#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include <format>
#include <string>
#include <memory>

namespace pmapi
{
    class SessionException : public Exception { using Exception::Exception; };

    class FrameQuery
    {
        friend class Session;
    public:
        ~FrameQuery()
        {
            pmFreeFrameQuery(hQuery_);
        }
        size_t GetBlobSize() const
        {
            return blobSize_;
        }
        void Consume(uint32_t pid, uint8_t* pBlobs, uint32_t& numBlobsInOut)
        {
            pmConsumeFrames(hQuery_, pid, pBlobs, &numBlobsInOut);
        }
    private:
        FrameQuery(std::span<PM_QUERY_ELEMENT> elements)
        {
            pmRegisterFrameQuery(&hQuery_, elements.data(), elements.size(), &blobSize_);
        }
        PM_FRAME_QUERY_HANDLE hQuery_ = nullptr;
        uint32_t blobSize_ = 0ull;
    };

    class DynamicQuery
    {
        friend class Session;
    public:
        ~DynamicQuery()
        {
            pmFreeDynamicQuery(hQuery_);
        }
        size_t GetBlobSize() const
        {
            return blobSize_;
        }
        void Poll(uint32_t pid, uint8_t* pBlob, uint32_t& numSwapChains)
        {
            pmPollDynamicQuery(hQuery_, pid, pBlob, &numSwapChains);
        }
    private:
        DynamicQuery(std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs)
        {
            pmRegisterDynamicQuery(&hQuery_, elements.data(), elements.size(), winSizeMs, metricOffsetMs);
            if (elements.size() > 0) {
                blobSize_ = elements.back().dataOffset + elements.back().dataSize;
            }
        }
        PM_DYNAMIC_QUERY_HANDLE hQuery_ = nullptr;
        size_t blobSize_ = 0ull;
    };

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
        Session();
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
        std::shared_ptr<intro::Root> GetIntrospectionRoot() const
        {
            // throw an exception on error or non-token
            if (!token_) {
                throw SessionException{ "introspection call failed due to empty session object" };
            }
            const PM_INTROSPECTION_ROOT* pRoot{};
            if (auto sta = pmGetIntrospectionRoot(&pRoot); sta != PM_STATUS_SUCCESS) {
                throw SessionException{ std::format("introspection call failed with error id={}", (int)sta) };
            }
            return std::make_shared<intro::Root>(pRoot, [](const PM_INTROSPECTION_ROOT* ptr) { pmFreeIntrospectionRoot(ptr); });
        }
        std::shared_ptr<ProcessTracker> TrackProcess(uint32_t pid)
        {
            return std::shared_ptr<ProcessTracker>{ new ProcessTracker{ pid } };
        }
        std::shared_ptr<DynamicQuery> RegisterDyanamicQuery(std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs)
        {
            return std::shared_ptr<DynamicQuery>{ new DynamicQuery{ elements, winSizeMs, metricOffsetMs } };
        }
        std::shared_ptr<FrameQuery> RegisterFrameQuery(std::span<PM_QUERY_ELEMENT> elements)
        {
            return std::shared_ptr<FrameQuery>{ new FrameQuery{ elements } };
        }
    private:
        bool token_ = true;
	};
}
#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include <format>
#include <string>
#include <cassert>
#include "BlobContainer.h"
#include "ProcessTracker.h"

namespace pmapi
{
    class FrameQuery
    {
        friend class Session;
    public:
        FrameQuery() = default;
        ~FrameQuery() { Reset(); }
        FrameQuery(FrameQuery&& other) noexcept
        {
            *this = std::move(other);
        }
        FrameQuery& operator=(FrameQuery&& rhs) noexcept
        {
            hQuery_ = rhs.hQuery_;
            blobSize_ = rhs.blobSize_;
            rhs.Clear_();;
            return *this;
        }
        size_t GetBlobSize() const
        {
            return blobSize_;
        }
        void Consume(const ProcessTracker& tracker, uint8_t* pBlobs, uint32_t& numBlobsInOut)
        {
            if (auto sta = pmConsumeFrames(hQuery_, tracker.GetPid(), pBlobs, &numBlobsInOut); sta != PM_STATUS_SUCCESS) {
                throw Exception{ std::format("consume frame call failed with error id={}", (int)sta) };
            }
        }
        void Consume(const ProcessTracker& tracker, BlobContainer& blobs)
        {
            assert(!Empty());
            assert(blobs.CheckHandle(hQuery_));
            Consume(tracker, blobs.GetFirst(), blobs.AcquireNumBlobsInRef_());
        }
        BlobContainer MakeBlobContainer(uint32_t nBlobs) const
        {
            assert(!Empty());
            return { hQuery_, blobSize_, nBlobs };
        }
        void Reset() noexcept
        {
            if (!Empty()) {
                // TODO: check and report error here (nothrow)
                pmFreeFrameQuery(hQuery_);
            }
            Clear_();
        }
        bool Empty() const
        {
            return hQuery_ == nullptr;
        }
        operator bool() const { return !Empty(); }
    private:
        // functions
        FrameQuery(PM_SESSION_HANDLE hSession, std::span<PM_QUERY_ELEMENT> elements)
        {
            if (auto sta = pmRegisterFrameQuery(hSession, &hQuery_, elements.data(), elements.size(), &blobSize_);
                sta != PM_STATUS_SUCCESS) {
                throw Exception{ std::format("register frame query call failed with error id={}", (int)sta) };
            }
        }
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept
        {
            hQuery_ = nullptr;
            blobSize_ = 0ull;
        }
        // data
        PM_FRAME_QUERY_HANDLE hQuery_ = nullptr;
        uint32_t blobSize_ = 0ull;
    };
}

#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "BlobContainer.h"
#include "ProcessTracker.h"
#include <span>

namespace pmapi
{
    class FrameQuery
    {
        friend class Session;
    public:
        FrameQuery() = default;
        ~FrameQuery();
        FrameQuery(FrameQuery&& other) noexcept;
        FrameQuery& operator=(FrameQuery&& rhs) noexcept;
        size_t GetBlobSize() const;
        void Consume(const ProcessTracker& tracker, uint8_t* pBlobs, uint32_t& numBlobsInOut);
        void Consume(const ProcessTracker& tracker, BlobContainer& blobs);
        BlobContainer MakeBlobContainer(uint32_t nBlobs) const;
        void Reset() noexcept;
        bool Empty() const;
        operator bool() const;
    private:
        // functions
        FrameQuery(PM_SESSION_HANDLE hSession, std::span<PM_QUERY_ELEMENT> elements);
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept;
        // data
        PM_FRAME_QUERY_HANDLE hQuery_ = nullptr;
        uint32_t blobSize_ = 0ull;
    };
}

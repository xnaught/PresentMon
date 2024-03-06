#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "BlobContainer.h"
#include "ProcessTracker.h"
#include <span>

namespace pmapi
{
    class DynamicQuery
    {
        friend class Session;
    public:
        DynamicQuery() = default;
        ~DynamicQuery();
        DynamicQuery(DynamicQuery&& other) noexcept;
        DynamicQuery& operator=(DynamicQuery&& rhs) noexcept;
        size_t GetBlobSize() const;
        void Poll(const ProcessTracker& tracker, uint8_t* pBlob, uint32_t& numSwapChains) const;
        void Poll(const ProcessTracker& tracker, BlobContainer& blobs) const;
        BlobContainer MakeBlobContainer(uint32_t nBlobs) const;
        void Reset();
        bool Empty() const;
        operator bool() const;
    private:
        // function
        DynamicQuery(PM_SESSION_HANDLE hSession, std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs);
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept;
        // data
        PM_DYNAMIC_QUERY_HANDLE hQuery_ = nullptr;
        size_t blobSize_ = 0ull;
    };
}

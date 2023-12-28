#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include <format>
#include <string>
#include <memory>
#include "BlobContainer.h"

namespace pmapi
{
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
            if (auto sta = pmPollDynamicQuery(hQuery_, pid, pBlob, &numSwapChains); sta != PM_STATUS_SUCCESS) {
                throw Exception{ std::format("dynamic poll call failed with error id={}", (int)sta) };
            }
        }
        void Poll(uint32_t pid, BlobContainer& blobs)
        {
            assert(blobs.CheckHandle(hQuery_));
            Poll(pid, blobs.GetFirst(), blobs.AcquireNumBlobsInRef_());
        }
        BlobContainer MakeBlobContainer(uint32_t nBlobs) const
        {
            return { hQuery_, blobSize_, nBlobs };
        }
    private:
        DynamicQuery(std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs)
        {
            if (auto sta = pmRegisterDynamicQuery(&hQuery_, elements.data(),
                elements.size(), winSizeMs, metricOffsetMs); sta != PM_STATUS_SUCCESS) {
                throw Exception{ std::format("dynamic query register call failed with error id={}", (int)sta) };
            }
            if (elements.size() > 0) {
                blobSize_ = elements.back().dataOffset + elements.back().dataSize;
            }
        }
        PM_DYNAMIC_QUERY_HANDLE hQuery_ = nullptr;
        size_t blobSize_ = 0ull;
    };
}

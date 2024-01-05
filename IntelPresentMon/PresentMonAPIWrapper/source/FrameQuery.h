#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include <format>
#include <string>
#include <cassert>
#include "BlobContainer.h"

namespace pmapi
{
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
            if (auto sta = pmConsumeFrames(hQuery_, pid, pBlobs, &numBlobsInOut); sta != PM_STATUS_SUCCESS) {
                throw Exception{ std::format("consume frame call failed with error id={}", (int)sta) };
            }
        }
        void Consume(uint32_t pid, BlobContainer& blobs)
        {
            assert(blobs.CheckHandle(hQuery_));
            Consume(pid, blobs.GetFirst(), blobs.AcquireNumBlobsInRef_());
        }
        BlobContainer MakeBlobContainer(uint32_t nBlobs) const
        {
            return { hQuery_, blobSize_, nBlobs };
        }
    private:
        FrameQuery(PM_SESSION_HANDLE hSession, std::span<PM_QUERY_ELEMENT> elements)
            :
            hSession_{ hSession }
        {
            if (auto sta = pmRegisterFrameQuery(hSession_, &hQuery_, elements.data(), elements.size(), &blobSize_);
                sta != PM_STATUS_SUCCESS) {
                throw Exception{ std::format("register frame query call failed with error id={}", (int)sta) };
            }
        }
        PM_FRAME_QUERY_HANDLE hQuery_ = nullptr;
        PM_SESSION_HANDLE hSession_ = nullptr;
        uint32_t blobSize_ = 0ull;
    };
}

#pragma once
#include <memory>
#include <vector>
#include <cstdint>

namespace pmapi
{
    class BlobContainer
    {
        friend class FrameQuery;
        friend class DynamicQuery;
    public:
        BlobContainer(const void* handle, size_t blobSize, uint32_t nBlobs)
            :
            handle_{ handle },
            blobSize_{ blobSize },
            nBlobs_{ nBlobs }
        {
            pBlobArrayBytes_ = std::make_unique<uint8_t[]>(GetTotalSize());
            for (uint32_t i = 0; i < nBlobs; i++) {
                blobPointers_.push_back((*this)[i]);
            }
        }
        size_t GetTotalSize() const { return blobSize_ * nBlobs_; }
        size_t GetBlobSize() const { return blobSize_; }
        uint32_t GetBlobCount() const { return nBlobs_; }
        uint32_t GetNumBlobsPopulated() const { return nBlobsFilledInOut_; }
        uint8_t* GetFirst() { return pBlobArrayBytes_.get(); }
        uint8_t* operator[](size_t index)
        {
            assert(index < nBlobs_);
            return pBlobArrayBytes_.get() + blobSize_ * index;
        }
        const uint8_t* operator[](size_t index) const
        {
            return const_cast<BlobContainer&>(*this)[index];
        }
        std::vector<uint8_t*>::const_iterator begin() const { return blobPointers_.begin(); }
        std::vector<uint8_t*>::const_iterator end() const { return blobPointers_.begin() + GetNumBlobsPopulated(); }
        bool AllBlobsPopulated() const { return nBlobsFilledInOut_ == nBlobs_; }
        template<typename T>
        bool CheckHandle(T handle) const
        {
            return static_cast<const void*>(handle) == handle_;
        }
    private:
        uint32_t& AcquireNumBlobsInRef_()
        {
            nBlobsFilledInOut_ = nBlobs_;
            return nBlobsFilledInOut_;
        }
        const void* handle_;
        size_t blobSize_;
        uint32_t nBlobs_;
        uint32_t nBlobsFilledInOut_ = 0ull;
        std::unique_ptr<uint8_t[]> pBlobArrayBytes_;
        std::vector<uint8_t*> blobPointers_;
    };
}
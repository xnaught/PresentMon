#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <cassert>

namespace pmapi
{
    class BlobContainer
    {
        friend class FrameQuery;
        friend class DynamicQuery;
    public:
        BlobContainer() = default;
        BlobContainer(const void* handle, size_t blobSize, uint32_t nBlobs)
            :
            handle_{ handle },
            blobSize_{ blobSize },
            nBlobs_{ nBlobs }
        {
            pBlobArrayBytes_ = std::make_unique<uint8_t[]>(GetTotalSize());
            PopulateBlobPointers_();
        }
        BlobContainer(BlobContainer&& other) noexcept
        {
            *this = std::move(other);
        }
        BlobContainer(const BlobContainer& other)
        {
            *this = other;
        }
        BlobContainer& operator=(BlobContainer&& rhs) noexcept
        {
            handle_ = rhs.handle_;
            blobSize_ = rhs.blobSize_;
            nBlobs_ = rhs.nBlobs_;
            nBlobsFilledInOut_ = rhs.nBlobsFilledInOut_;
            pBlobArrayBytes_ = std::move(rhs.pBlobArrayBytes_);
            blobPointers_ = std::move(rhs.blobPointers_);
            rhs.Reset();
            return *this;
        }
        BlobContainer& operator=(const BlobContainer& rhs)
        {
            handle_ = rhs.handle_;
            blobSize_ = rhs.blobSize_;
            nBlobs_ = rhs.nBlobs_;
            nBlobsFilledInOut_ = rhs.nBlobsFilledInOut_;
            pBlobArrayBytes_ = std::make_unique<uint8_t[]>(rhs.GetTotalSize());
            PopulateBlobPointers_();
            std::copy(rhs.pBlobArrayBytes_.get(),
                rhs.pBlobArrayBytes_.get() + rhs.GetTotalSize(),
                pBlobArrayBytes_.get());
            return *this;
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
        void Reset()
        {
            handle_ = nullptr;
            blobSize_ = 0;
            nBlobs_ = 0;
            nBlobsFilledInOut_ = 0;
            pBlobArrayBytes_.reset();
            blobPointers_.clear();
        }
    private:
        // functions
        void PopulateBlobPointers_()
        {
            for (uint32_t i = 0; i < nBlobs_; i++) {
                blobPointers_.push_back((*this)[i]);
            }
        }
        uint32_t& AcquireNumBlobsInRef_()
        {
            nBlobsFilledInOut_ = nBlobs_;
            return nBlobsFilledInOut_;
        }
        // data
        const void* handle_ = nullptr;
        size_t blobSize_ = 0;
        uint32_t nBlobs_ = 0;
        uint32_t nBlobsFilledInOut_ = 0;
        std::unique_ptr<uint8_t[]> pBlobArrayBytes_;
        std::vector<uint8_t*> blobPointers_;
    };
}
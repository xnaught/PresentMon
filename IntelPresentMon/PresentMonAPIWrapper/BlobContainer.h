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
        BlobContainer() = default;
        BlobContainer(const void* handle, size_t blobSize, uint32_t nBlobs);
        BlobContainer(BlobContainer&& other) noexcept;
        BlobContainer(const BlobContainer& other);
        BlobContainer& operator=(BlobContainer&& rhs) noexcept;
        BlobContainer& operator=(const BlobContainer& rhs);
        // size of all blobs in bytes
        size_t GetTotalSize() const;
        // size of a single blob in bytes
        size_t GetBlobSize() const;
        uint32_t GetBlobCount() const;
        uint32_t GetNumBlobsPopulated() const;
        // get a pointer to the first byte of the first blob
        uint8_t* GetFirst();
        const uint8_t* GetFirst() const;
        uint8_t* operator[](size_t index);
        const uint8_t* operator[](size_t index) const;
        // beginning of populated range of blobs
        std::vector<uint8_t*>::const_iterator begin() const;
        // end of populated range of blobs
        std::vector<uint8_t*>::const_iterator end() const;
        bool AllBlobsPopulated() const;
        template<typename T>
        bool CheckHandle(T handle) const
        {
            return static_cast<const void*>(handle) == handle_;
        }
        void Reset();
        bool Empty() const;
        operator bool() const;
    private:
        // functions
        void PopulateBlobPointers_();
        uint32_t& AcquireNumBlobsInRef_();
        // data
        const void* handle_ = nullptr;
        size_t blobSize_ = 0;
        uint32_t nBlobs_ = 0;
        uint32_t nBlobsFilledInOut_ = 0;
        std::unique_ptr<uint8_t[]> pBlobArrayBytes_;
        std::vector<uint8_t*> blobPointers_;
    };
}
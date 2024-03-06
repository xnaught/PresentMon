#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "../CommonUtilities//str/String.h"
#include "../PresentMonAPIWrapperCommon/EnumMap.h"
#include "Session.h"
#include "BlobContainer.h"
#include <vector>
#include <ranges>
#include <array>
#include <cassert>

namespace pmapi
{
	struct FixedQueryContainer_
	{
		const BlobContainer& PeekBlobContainer() const;
		BlobContainer ExtractBlobContainer();
		void InjectBlobContainer(BlobContainer blobs);
		void SwapBlobContainers(BlobContainer& blobs);
		const uint8_t* PeekActiveBlob() const;
		void SetActiveBlobIndex(uint32_t blobIndex);
		size_t GetActiveBlobIndex() const;
	protected:
		friend class FixedQueryElement;
		// functions
		template<typename...S>
		FixedQueryContainer_(Session& session, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			pSession_{ &session },
			pIntrospection_{ session.GetIntrospectionRoot() },
			slotDeviceIds_{ uint32_t(slotDeviceIds)... },
			nBlobs_{ nBlobs }
		{}
		void FinalizationPreprocess_();
		uint32_t MapDeviceId_(uint32_t deviceSlot) const;
		void FinalizationPostprocess_(bool isPolled);
		// data
		//   temporary construction storage
		std::vector<uint32_t> slotDeviceIds_;
		std::vector<PM_QUERY_ELEMENT> rawElements_;
		std::vector<class FixedQueryElement*> smartElements_;
		Session* pSession_ = nullptr;
		std::shared_ptr<pmapi::intro::Root> pIntrospection_;
		uint32_t nBlobs_;
		//   retained storage
		BlobContainer blobs_;
		size_t activeBlobIndex_ = 0;
	};
	
	struct FixedDynamicQueryContainer : public FixedQueryContainer_
	{
		template<typename...S>
		FixedDynamicQueryContainer(Session& session, double winSizeMs, double metricOffsetMs, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			FixedQueryContainer_{ session, nBlobs, slotDeviceIds... },
			winSizeMs_{ winSizeMs },
			metricOffsetMs_{ metricOffsetMs }
		{}
		BlobContainer MakeBlobContainer(uint32_t nBlobs) const;
		void Poll(ProcessTracker& tracker);
	private:
		friend class FinalizingElement;
		// functions
		void Finalize_();
		// data
		//   temporary construction storage
		double winSizeMs_;
		double metricOffsetMs_;
		//   retained storage
		DynamicQuery query_;
	};

	struct FixedFrameQueryContainer : public FixedQueryContainer_
	{
		template<typename...S>
		FixedFrameQueryContainer(Session& session, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			FixedQueryContainer_{ session, nBlobs, slotDeviceIds... }
		{}
		BlobContainer MakeBlobContainer(uint32_t nBlobs) const;
		void Consume(ProcessTracker& tracker);
		size_t ForEachConsume(ProcessTracker& tracker, std::function<void()> frameHandler);
	private:
		friend class FinalizingElement;
		// functions
		void Finalize_();
		// data
		FrameQuery query_;
	};

	namespace
	{
		// this static functor converts static types when bridged with runtime PM_DATA_TYPE info
		// enables the |F|ixed |Q|uery elements to choose correct conversion based on both runtime PM_DATA_TYPE
		// and compile-time type of the type to convert to
		template<PM_DATA_TYPE dt, PM_ENUM staticEnumId, typename DestType>
		struct FQReadBridger
		{
			using SourceType = typename pmon::ipc::intro::DataTypeToStaticType<dt, staticEnumId>::type;
			static void Invoke(PM_ENUM enumId, DestType& dest, const uint8_t* pBlobBytes)
			{
				// void types are not handleable and generally should not occur
				if constexpr (dt == PM_DATA_TYPE_VOID) {
					assert(false && "trying to convert void type");
				}
				// strings (char array) can convert to std::basic_string types
				else if constexpr (dt == PM_DATA_TYPE_STRING) {
					if constexpr (std::same_as<DestType, std::string>) {
						dest = reinterpret_cast<const char*>(pBlobBytes);
					}
					else if constexpr (std::same_as<DestType, std::wstring>) {
						dest = pmon::util::str::ToWide(reinterpret_cast<const char*>(pBlobBytes));
					}
					else {
						assert(false && "failure to convert enum type");
					}
				}
				// enums can convert to numeric types or string types
				else if constexpr (dt == PM_DATA_TYPE_ENUM) {
					if constexpr (std::same_as<DestType, std::string>) {
						try {
							const auto keyId = *reinterpret_cast<const int*>(pBlobBytes);
							dest = EnumMap::GetKeyMap(enumId)->at(keyId).narrowName;
						}
						catch (...) { dest = "Invalid"; }
					}
					else if constexpr (std::same_as<DestType, std::wstring>) {
						try {
							const auto keyId = *reinterpret_cast<const int*>(pBlobBytes);
							dest = EnumMap::GetKeyMap(enumId)->at(keyId).wideName;
						}
						catch (...) { dest = L"Invalid"; }
					}
					else if constexpr (
						std::is_integral_v<DestType> ||
						std::is_floating_point_v<DestType> ||
						std::same_as<DestType, SourceType>) {
						dest = DestType(*reinterpret_cast<const int*>(pBlobBytes));
					}
					else {
						assert(false && "failure to convert enum type");
					}
				}
				// what's left is actual numeric types
				else {
					// don't output if the dest is string type
					if constexpr (!std::same_as<DestType, std::string> && !std::same_as<DestType, std::wstring>) {
						dest = static_cast<DestType>(reinterpret_cast<const SourceType&>(*pBlobBytes));
					}
					else {
						assert(false && "failure to convert numeric type");
					}
				}
			}
			static void Default(DestType& dest, const uint8_t* pBlobBytes) {
				assert(false && "failure to convert unknown type");
			}
		};

		// adapter to convert template taking 4 arguments to template taking 2
		// (we need this because you cannot define templates within a function
		// and the static info is only available inside the templated function)
		template<typename T>
		struct FQReadBridgerAdapter {
			template<PM_DATA_TYPE dt, PM_ENUM enumId>
			using Bridger = FQReadBridger<dt, enumId, T>;
		};
	}

	class FixedQueryElement
	{
		friend FixedQueryContainer_;
	public:
		FixedQueryElement(FixedQueryContainer_* pContainer, PM_METRIC metric,
			PM_STAT stat, uint32_t deviceSlot = 0, uint32_t index = 0);
		template<typename T>
		void Load(T& dest) const
		{
			assert(IsAvailable());
			pmon::ipc::intro::BridgeDataTypeWithEnum<typename FQReadBridgerAdapter<T>::Bridger>(
				dataType_, enumId_, dest, pContainer_->PeekActiveBlob() + dataOffset_);
		}
		template<typename T>
		T As() const
		{
			T val;
			Load(val);
			return val;
		}
		template<typename T>
		operator T() const
		{
			return As<T>();
		}
		bool IsAvailable() const;
	private:
		const FixedQueryContainer_* pContainer_ = nullptr;
		uint64_t dataOffset_ = 0ull;
		PM_DATA_TYPE dataType_ = PM_DATA_TYPE_VOID;
		PM_ENUM enumId_ = PM_ENUM_NULL_ENUM;
	};

	class FinalizingElement
	{
	public:
		template<class E>
		FinalizingElement(E* pContainer)
		{
			pContainer->Finalize_();
		}
	};
}

#define PM_BEGIN_FIXED_DYNAMIC_QUERY(type) struct type : FixedDynamicQueryContainer { using FixedDynamicQueryContainer::FixedDynamicQueryContainer;
#define PM_BEGIN_FIXED_FRAME_QUERY(type) struct type : FixedFrameQueryContainer { using FixedFrameQueryContainer::FixedFrameQueryContainer;
#define PM_END_FIXED_QUERY private: FinalizingElement finalizer{ this }; }
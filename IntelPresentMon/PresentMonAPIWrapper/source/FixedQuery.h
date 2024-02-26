#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "../../CommonUtilities/source/str/String.h"
#include "../../PresentMonAPIWrapperCommon/source/EnumMap.h"
#include "Session.h"
#include "BlobContainer.h"
#include <vector>
#include <ranges>
#include <array>

namespace pmapi
{
	struct FixedQueryContainer_
	{
		const BlobContainer& PeekBlobContainer() const
		{
			return blobs_;
		}
		BlobContainer ExtractBlobContainer()
		{
			return std::move(blobs_);
		}
		void InjectBlobContainer(BlobContainer blobs)
		{
			blobs_ = std::move(blobs);
		}
		void SwapBlobContainers(BlobContainer& blobs)
		{
			std::swap(blobs, blobs_);
		}
		const uint8_t* PeekActiveBlob() const
		{
			return blobs_[activeBlobIndex_];;
		}
		void SetActiveBlobIndex(uint32_t blobIndex)
		{
			activeBlobIndex_ = blobIndex;
		}
		size_t GetActiveBlobIndex() const
		{
			return activeBlobIndex_;
		}
	protected:
		friend class FixedQueryElement;
		// functions
		template<typename...S>
		FixedQueryContainer_(Session& session, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			pSession_{ &session },
			slotDeviceIds_{ uint32_t(slotDeviceIds)... },
			nBlobs_{ nBlobs }
		{}
		void FinalizationPreprocess_()
		{
			// replace slot indexes with device ids
			for (auto& e : rawElements_) {
				if (e.deviceId > 0) {
					e.deviceId = slotDeviceIds_.at(e.deviceId - 1);
				}
			}
		}
		void FinalizationPostprocess_(bool isPolled);
		// data
		//   temporary construction storage
		std::vector<uint32_t> slotDeviceIds_;
		std::vector<PM_QUERY_ELEMENT> rawElements_;
		std::vector<class FixedQueryElement*> smartElements_;
		Session* pSession_ = nullptr;
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
		BlobContainer MakeBlobContainer(uint32_t nBlobs) const
		{
			return query_.MakeBlobContainer(nBlobs);
		}
		void Poll(ProcessTracker& tracker)
		{
			query_.Poll(tracker, blobs_);
		}
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

	template<class T>
	struct FixedFrameQueryContainer : public FixedQueryContainer_
	{
		template<typename...S>
		FixedFrameQueryContainer(Session& session, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			FixedQueryContainer_{ session, nBlobs, slotDeviceIds... }
		{}
		BlobContainer MakeBlobContainer(uint32_t nBlobs) const
		{
			return query_.MakeBlobContainer(nBlobs);
		}
		void Consume(ProcessTracker& tracker)
		{
			query_.Consume(tracker, blobs_);
		}
		size_t ForEachConsume(ProcessTracker& tracker, std::function<void(const T&)> frameHandler)
		{
			size_t nFramesProcessed = 0;
			do {
				Consume(tracker);
				const auto nPopulated = blobs_.GetNumBlobsPopulated();
				for (uint32_t i = 0; i < nPopulated; i++) {
					SetActiveBlobIndex(i);
					frameHandler(static_cast<const T&>(*this));
				}
				nFramesProcessed += nPopulated;
			} while (blobs_.AllBlobsPopulated());
			return nFramesProcessed;
		}
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
			PM_STAT stat, uint32_t deviceSlot = 0, uint32_t index = 0)
			:
			pContainer_{ pContainer }
		{
			pContainer->rawElements_.push_back(PM_QUERY_ELEMENT{
				.metric = metric,
				.stat = stat,
				.deviceId = deviceSlot,
				.arrayIndex = index,
			});
			pContainer->smartElements_.push_back(this);
		}
		template<typename T>
		void Load(T& dest) const
		{
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

	void FixedQueryContainer_::FinalizationPostprocess_(bool isPolled)
	{
		// get introspection data
		auto pIntro = pSession_->GetIntrospectionRoot();
		// complete smart query objects
		for (auto&& [raw, smart] : std::views::zip(rawElements_, smartElements_)) {
			smart->dataOffset_ = raw.dataOffset;
			const auto dti = pIntro->FindMetric(raw.metric).GetDataTypeInfo();
			smart->dataType_ = isPolled ? dti.GetPolledType() : dti.GetFrameType();
			smart->enumId_ = dti.GetEnumId();
		}
		// cleanup temporary construction data
		smartElements_.clear();
		rawElements_.clear();
		slotDeviceIds_.clear();
		pSession_ = nullptr;
	}

	void FixedDynamicQueryContainer::Finalize_()
	{
		FinalizationPreprocess_();

		// register query
		assert(pSession_);
		query_ = pSession_->RegisterDyanamicQuery(rawElements_, winSizeMs_, metricOffsetMs_);

		// make blobs
		blobs_ = query_.MakeBlobContainer(nBlobs_);

		FinalizationPostprocess_(true);
	}

	template<class T>
	void FixedFrameQueryContainer<T>::Finalize_()
	{
		FinalizationPreprocess_();

		// register query
		assert(pSession_);
		query_ = pSession_->RegisterFrameQuery(rawElements_);

		// make blobs
		blobs_ = query_.MakeBlobContainer(nBlobs_);

		FinalizationPostprocess_(false);
	}
}

#define PM_BEGIN_FIXED_DYNAMIC_QUERY(type) struct type : FixedDynamicQueryContainer { using FixedDynamicQueryContainer::FixedDynamicQueryContainer;
#define PM_BEGIN_FIXED_FRAME_QUERY(type) struct type : FixedFrameQueryContainer<type> { using FixedFrameQueryContainer<type>::FixedFrameQueryContainer;
#define PM_END_FIXED_QUERY private: FinalizingElement finalizer{ this }; }
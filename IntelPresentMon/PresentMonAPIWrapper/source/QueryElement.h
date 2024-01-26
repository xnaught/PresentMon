#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "Session.h"
#include "BlobContainer.h"
#include <vector>
#include <ranges>
#include <array>

namespace pmapi
{
	struct QueryContainer_
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
		friend class QueryElement;
		// functions
		template<typename...S>
		QueryContainer_(Session& session, uint32_t nBlobs, S&&...slotDeviceIds)
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
		void FinalizationPostprocess_();
		// data
		//   temporary construction storage
		std::vector<uint32_t> slotDeviceIds_;
		std::vector<PM_QUERY_ELEMENT> rawElements_;
		std::vector<class QueryElement*> smartElements_;
		Session* pSession_ = nullptr;
		uint32_t nBlobs_;
		//   retained storage
		BlobContainer blobs_;
		size_t activeBlobIndex_ = 0;
	};

	struct DynamicQueryContainer : public QueryContainer_
	{
		template<typename...S>
		DynamicQueryContainer(Session& session, double winSizeMs, double metricOffsetMs, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			QueryContainer_{ session, nBlobs, slotDeviceIds... },
			winSizeMs_{ winSizeMs },
			metricOffsetMs_{ metricOffsetMs }
		{}
		BlobContainer MakeBlobContainer(uint32_t nBlobs) const
		{
			return query_.MakeBlobContainer(nBlobs);
		}
		void Poll(pmapi::ProcessTracker& tracker)
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

	template<PM_DATA_TYPE dt, typename DestType>
	struct QEReadBridger
	{
		using SourceType = typename pmon::ipc::intro::DataTypeToStaticType<dt>::type;
		static void Invoke(DestType& dest, const uint8_t* pBlobBytes, uint64_t dataOffset)
		{
			// if src is convertible to dest, then doit
			if constexpr (dt == PM_DATA_TYPE_VOID) {
				return;
			}
			else if constexpr (dt == PM_DATA_TYPE_STRING) {
				return;
			}
			else {
				dest = static_cast<DestType>(reinterpret_cast<const SourceType&>(pBlobBytes[dataOffset]));
			}
		}
		static void Default(DestType& dest, const uint8_t* pBlobBytes, uint64_t dataOffset) {}
	};

	template<typename T>
	struct QEReadBridgerAdapter {
		template<PM_DATA_TYPE dt>
		using Bridger = QEReadBridger<dt, T>;
	};

	class QueryElement
	{
		friend QueryContainer_;
	public:
		QueryElement(QueryContainer_* pContainer, PM_METRIC metric, PM_STAT stat, uint32_t deviceSlot = 0, uint32_t index = 0)
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
			pmon::ipc::intro::BridgeDataType<typename QEReadBridgerAdapter<T>::Bridger>(
				dataType_, dest, pContainer_->PeekActiveBlob(), dataOffset_);
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
		const QueryContainer_* pContainer_ = nullptr;
		uint64_t dataOffset_ = 0ull;
		PM_DATA_TYPE dataType_ = PM_DATA_TYPE_VOID;
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

	void QueryContainer_::FinalizationPostprocess_()
	{
		// get introspection data
		auto pIntro = pSession_->GetIntrospectionRoot();
		// complete smart query objects
		for (auto&& [raw, smart] : std::views::zip(rawElements_, smartElements_)) {
			smart->dataOffset_ = raw.dataOffset;
			smart->dataType_ = pIntro->FindMetric(raw.metric).GetDataTypeInfo().GetPolledType();
		}
		// cleanup temporary construction data
		smartElements_.clear();
		rawElements_.clear();
		slotDeviceIds_.clear();
		pSession_ = nullptr;
	}

	void DynamicQueryContainer::Finalize_()
	{
		FinalizationPreprocess_();

		// register query
		assert(pSession_);
		query_ = pSession_->RegisterDyanamicQuery(rawElements_, winSizeMs_, metricOffsetMs_);

		// make blobs
		blobs_ = query_.MakeBlobContainer(nBlobs_);

		FinalizationPostprocess_();
	}
}
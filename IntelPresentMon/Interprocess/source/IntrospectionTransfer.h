#pragma once
#include <string>
#include <vector>
#include <memory>
#include <span>
#include <ranges>
#include <cstring>
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonMiddleware/source/ApiHelpers.h"

namespace pmon::ipc::intro
{
	namespace vi = std::views;

	template<typename T>
	concept IsApiClonable = requires {
		typename T::ApiType;
	};

	template<typename T>
	size_t GetPadding(size_t byteIndex)
	{
		constexpr auto alignment = alignof(T);
		const auto partialBytes = byteIndex % alignment;
		const auto padding = (alignment - partialBytes) % alignment;
		return padding;
	}

	template<typename T>
	class ProbeAllocator
	{
		template<typename T2>
		friend class ProbeAllocator;
	public:
		using ProbeTag = std::true_type;
		using value_type = T;
		ProbeAllocator() = default;
		ProbeAllocator(const ProbeAllocator<void>& other)
			: pTotalSize(other.pTotalSize)
		{}
		T* allocate(size_t count)
		{
			*pTotalSize += sizeof(T) * count + GetPadding<T>(*pTotalSize);
			return nullptr;
		}
		void deallocate(T*);
		size_t GetTotalSize() const
		{
			return *pTotalSize;
		}
	private:
		std::shared_ptr<size_t> pTotalSize = std::make_shared<size_t>();
	};

	template<typename T>
	class BlockAllocator
	{
		template<typename T2>
		friend class BlockAllocator;
	public:
		using value_type = T;
		BlockAllocator(size_t nBytes) : pBytes{ reinterpret_cast<char*>(malloc(nBytes)) } {}
		BlockAllocator(const BlockAllocator<void>& other)
			:
			pTotalSize(other.pTotalSize),
			pBytes{ other.pBytes }
		{}
		T* allocate(size_t count)
		{
			*pTotalSize += GetPadding<T>(*pTotalSize);
			const auto pStart = reinterpret_cast<T*>(pBytes + *pTotalSize);
			*pTotalSize += sizeof(T) * count;
			return pStart;
		}
		void deallocate(T*);
	private:
		std::shared_ptr<size_t> pTotalSize = std::make_shared<size_t>();
		char* pBytes = nullptr;
	};

	struct IntrospectionString : PM_INTROSPECTION_STRING
	{
		IntrospectionString(std::string s) : buffer_{ std::move(s) }
		{
			pData = buffer_.data();
		}
		IntrospectionString& operator=(std::string rhs)
		{
			buffer_ = std::move(rhs);
			pData = buffer_.data();
			return *this;
		}
		using ApiType = PM_INTROSPECTION_STRING;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			using CA = std::allocator_traits<V>::template rebind_alloc<char>;
			CA charAlloc{ voidAlloc };
			const auto bufferSize = buffer_.size() + 1;
			content.pData = charAlloc.allocate(bufferSize);
			if (content.pData) {
				strcpy_s(const_cast<char*>(content.pData), bufferSize, buffer_.c_str());
			}
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		std::string buffer_;
	};

	template<typename T>
	struct IntrospectionObjArray : PM_INTROSPECTION_OBJARRAY
	{
		IntrospectionObjArray()
			:
			PM_INTROSPECTION_OBJARRAY{ nullptr, 0 }
		{}
		IntrospectionObjArray(std::vector<T*> v)
			:
			buffer_{ std::move(v) }
		{
			Sync_();
		}
		~IntrospectionObjArray()
		{
			for (auto pObj : buffer_) {
				delete pObj;
			}
		}
		IntrospectionObjArray& operator=(std::vector<T*> rhs)
		{
			buffer_ = std::move(rhs);
			Sync_();
			return *this;
		}
		void PushBack(std::unique_ptr<T> pObj)
		{
			buffer_.push_back(pObj.release());
			Sync_();
		}
		using ApiType = PM_INTROSPECTION_OBJARRAY;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			// allocator to construct pointers inside this container
			using VPA = std::allocator_traits<V>::template rebind_alloc<void*>;
			VPA voidPtrAlloc{ voidAlloc };
			// allocator to construct objects to be pointed to (if not ApiClonable)
			using TA = std::allocator_traits<V>::template rebind_alloc<T>;
			TA tAlloc{ voidAlloc };
			content.size = buffer_.size();
			content.pData = const_cast<const void**>(voidPtrAlloc.allocate(content.size));
			// clone each element from shm to Api struct in heap
			for (size_t i = 0; i < content.size; i++) {
				void* pElement = nullptr;
				if constexpr (IsApiClonable<T>) {
					pElement = buffer_[i]->ApiClone(voidAlloc);
				}
				else {
					auto pNonApiClonableElement = tAlloc.allocate(1);
					if (pNonApiClonableElement) {
						std::allocator_traits<TA>::construct(tAlloc, pNonApiClonableElement, *buffer_[i]);
					}
					pElement = pNonApiClonableElement;
				}
				if (content.pData) {
					content.pData[i] = pElement;
				}
			}
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		void Sync_()
		{
			pData = (const void**)buffer_.data();
			size = buffer_.size();
		}
		std::vector<T*> buffer_;
	};

	struct IntrospectionEnumKey : PM_INTROSPECTION_ENUM_KEY
	{
		IntrospectionEnumKey(PM_ENUM enumId_in, int value_in, std::string symbol_in, std::string name_in, std::string shortName_in, std::string description_in)
			:
			enumId_{ enumId_in},
			value_{ value_in },
			symbol_{ std::move(symbol_in) },
			name_{ std::move(name_in) },
			shortName_{ std::move(shortName_in) },
			description_{ std::move(description_in) }
		{
			enumId = enumId_in;
			value = value_in;
			pSymbol = &symbol_;
			pName = &name_;
			pShortName = &shortName_;
			pDescription = &description_;
		}
		using ApiType = PM_INTROSPECTION_ENUM_KEY;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.enumId = enumId_;
			content.value = value_;
			content.pSymbol = symbol_.ApiClone(voidAlloc);
			content.pName = name_.ApiClone(voidAlloc);
			content.pShortName = shortName_.ApiClone(voidAlloc);
			content.pDescription = description_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		PM_ENUM enumId_;
		int value_;
		IntrospectionString symbol_;
		IntrospectionString name_;
		IntrospectionString shortName_;
		IntrospectionString description_;
	};

	struct IntrospectionEnum : PM_INTROSPECTION_ENUM
	{
		IntrospectionEnum(PM_ENUM id_in, std::string symbol_in, std::string description_in)
			:
			id_{ id_in },
			symbol_{ std::move(symbol_in) },
			description_{ std::move(description_in) }
		{
			id = id_in;
			pSymbol = &symbol_;
			pDescription = &description_;
			pKeys = &keys_;
		}
		void AddKey(std::unique_ptr<IntrospectionEnumKey> pKey)
		{
			keys_.PushBack(std::move(pKey));
		}
		using ApiType = PM_INTROSPECTION_ENUM;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.id = id_;
			content.pSymbol = symbol_.ApiClone(voidAlloc);
			content.pDescription = description_.ApiClone(voidAlloc);
			content.pKeys = keys_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		PM_ENUM id_;
		IntrospectionString symbol_;
		IntrospectionString description_;
		IntrospectionObjArray<IntrospectionEnumKey> keys_;
	};

	struct IntrospectionDevice : PM_INTROSPECTION_DEVICE
	{
		IntrospectionDevice(uint32_t id_in, PM_DEVICE_TYPE type_in, PM_DEVICE_VENDOR vendor_in, std::string name_in)
			:
			id_{ id_in },
			type_{ type_in },
			vendor_{ vendor_in },
			name_{ std::move(name_in) }
		{
			id = id_in;
			type = type_in;
			vendor = vendor_in;
			pName = &name_;
		}
		using ApiType = PM_INTROSPECTION_DEVICE;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.id = id_;
			content.type = type_;
			content.vendor = vendor_;
			content.pName = name_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		uint32_t id_;
		PM_DEVICE_TYPE type_;
		PM_DEVICE_VENDOR vendor_;
		IntrospectionString name_;
	};

	struct IntrospectionDeviceMetricInfo : PM_INTROSPECTION_DEVICE_METRIC_INFO
	{
		IntrospectionDeviceMetricInfo(uint32_t deviceId_in, PM_METRIC_AVAILABILITY availability_in, uint32_t arraySize_in)
			:
			deviceId_{ deviceId_in },
			availability_{ availability_in },
			arraySize_{ arraySize_in }
		{
			this->deviceId = deviceId_;
			this->availability = availability_;
			this->arraySize = arraySize_;
		}
		using ApiType = PM_INTROSPECTION_DEVICE_METRIC_INFO;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.deviceId = deviceId_;
			content.availability = availability_;
			content.arraySize = arraySize_;
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		uint32_t deviceId_;
		PM_METRIC_AVAILABILITY availability_;
		uint32_t arraySize_;
	};

	struct IntrospectionDataTypeInfo : PM_INTROSPECTION_DATA_TYPE_INFO
	{
		IntrospectionDataTypeInfo(PM_DATA_TYPE type_in, PM_ENUM enumId_in)
			:
			type_{ type_in },
			enumId_{ enumId_in }
		{
			this->type = type_in;
			this->enumId = enumId_in;
		}
		using ApiType = PM_INTROSPECTION_DATA_TYPE_INFO;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.type = type_;
			content.enumId = enumId_;
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		PM_DATA_TYPE type_;
		PM_ENUM enumId_;
	};

	struct IntrospectionMetric : PM_INTROSPECTION_METRIC
	{
		IntrospectionMetric(PM_METRIC id_in, PM_METRIC_TYPE type_in, PM_UNIT unit_in, const IntrospectionDataTypeInfo& typeInfo_in, std::vector<PM_STAT> stats_in = {})
			:
			id_{ id_in },
			type_{ type_in },
			unit_{ unit_in },
			typeInfo_{ typeInfo_in }
		{
			this->id = id_in;
			this->type = type_in;
			this->unit = unit_in;
			this->typeInfo = static_cast<const PM_INTROSPECTION_DATA_TYPE_INFO&>(typeInfo_in);
			AddStats(std::move(stats_in));
			pStats = &stats_;
			pDeviceMetricInfo = &deviceMetricInfo_;
		}
		void AddStat(PM_STAT stat)
		{
			stats_.PushBack(std::make_unique<PM_STAT>(stat));
		}
		void AddStats(std::vector<PM_STAT> stats)
		{
			for (auto stat : stats) {
				stats_.PushBack(std::make_unique<PM_STAT>(stat));
			}
		}
		void AddDeviceMetricInfo(IntrospectionDeviceMetricInfo info)
		{
			deviceMetricInfo_.PushBack(std::make_unique<IntrospectionDeviceMetricInfo>(info));
		}
		using ApiType = PM_INTROSPECTION_METRIC;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.id = id_;
			content.type = type_;
			content.unit = unit_;
			content.typeInfo = static_cast<const PM_INTROSPECTION_DATA_TYPE_INFO&>(typeInfo_);
			content.pStats = stats_.ApiClone(voidAlloc);
			content.pDeviceMetricInfo = deviceMetricInfo_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		PM_METRIC id_;
		PM_METRIC_TYPE type_;
		PM_UNIT unit_;
		IntrospectionDataTypeInfo typeInfo_;
		IntrospectionObjArray<PM_STAT> stats_;
		IntrospectionObjArray<IntrospectionDeviceMetricInfo> deviceMetricInfo_;
	};

	struct IntrospectionRoot : PM_INTROSPECTION_ROOT
	{
		IntrospectionRoot()
		{
			pMetrics = &metrics_;
			pEnums = &enums_;
			pDevices = &devices_;
		}
		void AddEnum(std::unique_ptr<IntrospectionEnum> pEnum)
		{
			enums_.PushBack(std::move(pEnum));
		}
		void AddMetric(std::unique_ptr<IntrospectionMetric> pMetric)
		{
			metrics_.PushBack(std::move(pMetric));
		}
		void AddDevice(std::unique_ptr<IntrospectionDevice> pDevice)
		{
			devices_.PushBack(std::move(pDevice));
		}
		using ApiType = PM_INTROSPECTION_ROOT;
		template<class V>
		mid::UniqueApiRootPtr ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// TODO: prepare contents
			content.pMetrics = metrics_.ApiClone(voidAlloc);
			content.pEnums = enums_.ApiClone(voidAlloc);
			content.pDevices = devices_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return mid::UniqueApiRootPtr(pSelf);
		}
	private:
		IntrospectionObjArray<IntrospectionMetric> metrics_;
		IntrospectionObjArray<IntrospectionEnum> enums_;
		IntrospectionObjArray<IntrospectionDevice> devices_;
	};
}
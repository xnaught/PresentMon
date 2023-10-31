#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include <iterator>
#include <string>
#include <ranges>
#include <memory>
#include <unordered_map>

namespace pmapi
{
	namespace intro
	{

        template<class T>
        class ObjArrayViewIterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using base_type = typename T::BaseType;
            using reference = value_type&;
            using pointer = value_type*;
            using difference_type = ptrdiff_t;

            ObjArrayViewIterator() = default;
            ObjArrayViewIterator(const class Dataset* pDataset_, const PM_INTROSPECTION_OBJARRAY* pObjArray, difference_type offset = 0u) noexcept
                :
                pDataset{ pDataset_ },
                pArray{ (const base_type* const*)pObjArray->pData + offset }
            {}
            ObjArrayViewIterator(const ObjArrayViewIterator& rhs) noexcept : pDataset{ rhs.pDataset }, pArray{ rhs.pArray } {}
            ObjArrayViewIterator& operator=(const ObjArrayViewIterator& rhs) noexcept
            {
                // Self-assignment check
                if (this != &rhs) {
                    pDataset = rhs.pDataset;
                    pArray = rhs.pArray;
                }
                return *this;
            }
            ObjArrayViewIterator& operator+=(difference_type rhs) noexcept
            {
                pArray += rhs;
                return *this;
            }
            ObjArrayViewIterator& operator-=(difference_type rhs) noexcept
            {
                pArray -= rhs;
                return *this;
            }
            value_type operator*() const noexcept
            {
                return value_type{ pDataset, *pArray };
            }
            value_type operator->() const noexcept
            {
                return **this;
            }
            value_type operator[](size_t idx) const noexcept
            {
                return value_type{ pDataset, pArray[idx] };
            }

            ObjArrayViewIterator& operator++() noexcept
            {
                ++pArray;
                return *this;
            }
            ObjArrayViewIterator& operator--() noexcept
            {
                --pArray;
                return *this;
            }
            ObjArrayViewIterator operator++(int) noexcept { ObjArrayViewIterator tmp(*this); ++(*this); return tmp; }
            ObjArrayViewIterator operator--(int) noexcept { ObjArrayViewIterator tmp(*this); --(*this); return tmp; }
            difference_type operator-(const ObjArrayViewIterator& rhs) const noexcept
            {
                return difference_type(pArray - rhs.pArray);
            }
            ObjArrayViewIterator operator+(difference_type rhs) const noexcept
            {
                auto dup = *this;
                return dup += rhs;
            }
            ObjArrayViewIterator operator-(difference_type rhs) const noexcept
            {
                auto dup = *this;
                return dup -= rhs;
            }

            bool operator==(const ObjArrayViewIterator& rhs) const noexcept { return pArray == rhs.pArray; }
            bool operator!=(const ObjArrayViewIterator& rhs) const noexcept { return pArray != rhs.pArray; }
            bool operator>(const ObjArrayViewIterator& rhs) const noexcept { return pArray > rhs.pArray; }
            bool operator<(const ObjArrayViewIterator& rhs) const noexcept { return pArray < rhs.pArray; }
            bool operator>=(const ObjArrayViewIterator& rhs) const noexcept { return pArray >= rhs.pArray; }
            bool operator<=(const ObjArrayViewIterator& rhs) const noexcept { return pArray <= rhs.pArray; }
        private:
            // data
            const base_type* const* pArray = nullptr;
            const class Dataset* pDataset = nullptr;
        };

        template<class V>
        using ViewRange = std::ranges::subrange<ObjArrayViewIterator<V>, ObjArrayViewIterator<V>>;


		class DataTypeInfoView
		{

		};

		class DeviceMetricInfoView
		{

		};

		class DeviceView
		{
        public:
        private:

		};

		class EnumKeyView
		{
            using BaseType = PM_INTROSPECTION_ENUM_KEY;
            using SelfType = EnumKeyView;
            friend class ObjArrayViewIterator<SelfType>;
            friend class Dataset;
        public:
            int GetValue() const
            {
                return pBase->value;
            }
            std::string GetSymbol() const
            {
                return pBase->pSymbol->pData;
            }
            std::string GetName() const
            {
                return pBase->pName->pData;
            }
            std::string GetShortName() const
            {
                return pBase->pShortName->pData;
            }
            std::string GetDescription() const
            {
                return pBase->pDescription->pData;
            }
            const SelfType* operator->() const
            {
                return this;
            }
            const BaseType* GetBasePtr() const
            {
                return pBase;
            }
        private:
            // functions
            EnumKeyView(const class Dataset* pDataset_, const BaseType* pBase_)
                :
                pDataset{ pDataset_ },
                pBase{ pBase_ }
            {}
            // data
            const class Dataset* pDataset = nullptr;
            const BaseType* pBase = nullptr;
		};

		class EnumView
		{
            using BaseType = PM_INTROSPECTION_ENUM;
            using SelfType = EnumView;
            friend class ObjArrayViewIterator<SelfType>;
            friend class Dataset;
        public:
            PM_ENUM GetID() const
            {
                return pBase->id;
            }
            std::string GetSymbol() const
            {
                return pBase->pSymbol->pData;
            }
            std::string GetDescription() const
            {
                return pBase->pDescription->pData;
            }
            ViewRange<EnumKeyView> GetKeys() const
            {
                // trying to deduce the template params for subrange causes intellisense to crash
                // workaround this by providing them explicitly as the return type (normally would use auto)
                return { GetKeysBegin_(), GetKeysEnd_() };
            }
            const SelfType* operator->() const
            {
                return this;
            }
        private:
            // functions
            EnumView(const class Dataset* pDataset_, const BaseType* pBase_)
                :
                pDataset{ pDataset_ },
                pBase{ pBase_ }
            {}
            ObjArrayViewIterator<EnumKeyView> GetKeysBegin_() const
            {
                return ObjArrayViewIterator<EnumKeyView>{ pDataset, pBase->pKeys };
            }
            ObjArrayViewIterator<EnumKeyView> GetKeysEnd_() const
            {
                return ObjArrayViewIterator<EnumKeyView>{ pDataset, pBase->pKeys, (int64_t)pBase->pKeys->size };
            }
            // data
            const class Dataset* pDataset;
            const BaseType* pBase = nullptr;
		};

        class MetricView
        {
            using BaseType = PM_INTROSPECTION_METRIC;
            using SelfType = MetricView;
            friend class ObjArrayViewIterator<SelfType>;
            friend class Dataset;
        public:
            EnumKeyView GetMetricKey() const;
            EnumKeyView GetUnit() const;
            const SelfType* operator->() const
            {
                return this;
            }
        private:
            // functions
            MetricView(const class Dataset* pDataset_, const BaseType* pBase_)
                :
                pDataset{ pDataset_ },
                pBase{ pBase_ }
            {}
            // data
            const class Dataset* pDataset;
            const BaseType* pBase = nullptr;
        };


		class Dataset
		{
		public:
            Dataset(const PM_INTROSPECTION_ROOT* pRoot_) : pRoot{ pRoot_ }
            {
                for (auto e : GetEnums()) {
                    for (auto k : e.GetKeys()) {
                        enumKeyMap[MakeEnumKeyMapKey_(e.GetID(), k.GetValue())] = k.GetBasePtr();
                    }
                }
            }
            Dataset(Dataset&& rhs) noexcept
                :
                pRoot{ rhs.pRoot },
                enumKeyMap{ std::move(rhs.enumKeyMap) }
            {
                rhs.pRoot = nullptr;
            }
            Dataset& operator=(Dataset&& rhs) noexcept
            {
                pRoot = rhs.pRoot;
                rhs.pRoot = nullptr;
                enumKeyMap = std::move(rhs.enumKeyMap);
                return *this;
            }
            ~Dataset()
            {
                pmFreeInterface(pRoot);
            }
            ViewRange<EnumView> GetEnums() const
            {
                // trying to deduce the template params for subrange causes intellisense to crash
                // workaround this by providing them explicitly as the return type (normally would use auto)
                return { GetEnumsBegin_(), GetEnumsEnd_() };
            }
            ViewRange<MetricView> GetMetrics() const
            {
                // trying to deduce the template params for subrange causes intellisense to crash
                // workaround this by providing them explicitly as the return type (normally would use auto)
                return { GetMetricsBegin_(), GetMetricsEnd_() };
            }
            EnumKeyView FindEnumKey(PM_ENUM enumId, int keyValue) const
            {
                // TODO: exception for bad lookup
                auto i = enumKeyMap.find(MakeEnumKeyMapKey_(enumId, keyValue));
                return { this, i->second };
            }
			//virtual DeviceView FindDevice(int id) const = 0;
			//virtual MetricView FindMetric(int id) const = 0;
        private:
            // functions
            static uint64_t MakeEnumKeyMapKey_(PM_ENUM enumId, int keyValue)
            {
                // pack 64-bit hash key as upper and lower 32-bit values
                return (uint64_t(enumId) << 32) | uint64_t(keyValue);
            }
            ObjArrayViewIterator<EnumView> GetEnumsBegin_() const
            {
                return ObjArrayViewIterator<EnumView>{ this, pRoot->pEnums };
            }
            ObjArrayViewIterator<EnumView> GetEnumsEnd_() const
            {
                return ObjArrayViewIterator<EnumView>{ this, pRoot->pEnums, (int64_t)pRoot->pEnums->size };
            }
            ObjArrayViewIterator<MetricView> GetMetricsBegin_() const
            {
                return ObjArrayViewIterator<MetricView>{ this, pRoot->pMetrics };
            }
            ObjArrayViewIterator<MetricView> GetMetricsEnd_() const
            {
                return ObjArrayViewIterator<MetricView>{ this, pRoot->pMetrics, (int64_t)pRoot->pMetrics->size };
            }
            // data
            const PM_INTROSPECTION_ROOT* pRoot = nullptr;
            std::unordered_map<uint64_t, const PM_INTROSPECTION_ENUM_KEY*> enumKeyMap;
		};
	}

	class Session
	{
    public:
        Session()
        {
            // throw exception on error
            pmOpenSession();
        }
        Session(Session&& rhs) noexcept
            :
            token{ rhs.token }
        {
            rhs.token = false;
        }
        Session& operator=(Session&& rhs) noexcept
        {
            token = rhs.token;
            rhs.token = false;
            return *this;
        }
        ~Session()
        {
            if (token) {
                pmCloseSession();
            }
        }
        intro::Dataset GetIntrospectionDataset() const
        {
            // throw an exception on error or non-token
            const PM_INTROSPECTION_ROOT* pRoot{};
            pmEnumerateInterface(&pRoot);
            return { pRoot };
        }
    private:
        bool token = true;
	};
}
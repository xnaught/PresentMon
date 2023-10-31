#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include <iterator>
#include <string>
#include <ranges>
#include <memory>

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
            ObjArrayViewIterator(std::shared_ptr<const class Dataset> pDataset_, const PM_INTROSPECTION_OBJARRAY* pObjArray, difference_type offset = 0u) noexcept
                :
                pDataset{ std::move(pDataset_) },
                pArray{ (const base_type* const*)pObjArray->pData + offset }
            {}
            ObjArrayViewIterator(const ObjArrayViewIterator& rhs) noexcept : pArray{ rhs.pArray } {}
            ObjArrayViewIterator& operator=(const ObjArrayViewIterator& rhs) noexcept
            {
                // Self-assignment check
                if (this != &rhs) {
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
            std::shared_ptr<const class Dataset> pDataset;
        };

        template<class V>
        using ViewRange = std::ranges::subrange<ObjArrayViewIterator<V>, ObjArrayViewIterator<V>>;

		class MetricView
		{

		};

		class DataTypeInfoView
		{

		};

		class DeviceMetricInfoView
		{

		};

		class DeviceView
		{

		};

		class EnumKeyView
		{
            using BaseType = PM_INTROSPECTION_ENUM_KEY;
            using SelfType = EnumKeyView;
            friend class ObjArrayViewIterator<SelfType>;
            friend class Dataset;
        public:
            std::string GetSymbol() const
            {
                return pBase->pSymbol->pData;
            }
            std::string GetDescription() const
            {
                return pBase->pDescription->pData;
            }
            const SelfType* operator->() const
            {
                return this;
            }
        private:
            // functions
            EnumKeyView(std::shared_ptr<const class Dataset> pDataset_, const BaseType* pBase_)
                :
                pDataset{ pDataset },
                pBase{ pBase_ }
            {}
            // data
            std::shared_ptr<const class Dataset> pDataset;
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
            EnumView(std::shared_ptr<const class Dataset> pDataset_, const BaseType* pBase_)
                :
                pDataset{ pDataset },
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
            std::shared_ptr<const class Dataset> pDataset;
            const BaseType* pBase = nullptr;
		};

		class Dataset : public std::enable_shared_from_this<Dataset>
		{
		public:
            Dataset(const PM_INTROSPECTION_ROOT* pRoot_) : pRoot{ pRoot_ } {}
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
			//virtual EnumView FindEnum(int id) const = 0;
			//virtual DeviceView FindDevice(int id) const = 0;
			//virtual MetricView FindMetric(int id) const = 0;
        private:
            // functions
            ObjArrayViewIterator<EnumView> GetEnumsBegin_() const
            {
                return ObjArrayViewIterator<EnumView>{ shared_from_this(), pRoot->pEnums };
            }
            ObjArrayViewIterator<EnumView> GetEnumsEnd_() const
            {
                return ObjArrayViewIterator<EnumView>{ shared_from_this(), pRoot->pEnums, (int64_t)pRoot->pEnums->size };
            }
            // data
            const PM_INTROSPECTION_ROOT* pRoot = nullptr;
		};
	}

	class Session
	{

	};
}
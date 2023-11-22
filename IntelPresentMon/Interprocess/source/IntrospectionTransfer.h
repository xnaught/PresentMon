#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../../PresentMonAPI2/source/PresentMonAPI.h"

namespace pmon::ipc::intro
{
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
	private:
		std::string buffer_;
	};

	template<class T>
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
	private:
		IntrospectionString symbol_;
		IntrospectionString name_;
		IntrospectionString shortName_;
		IntrospectionString description_;
	};

	struct IntrospectionEnum : PM_INTROSPECTION_ENUM
	{
		IntrospectionEnum(PM_ENUM id_in, std::string symbol_in, std::string description_in)
			:
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
	private:
		IntrospectionString symbol_;
		IntrospectionString description_;
		IntrospectionObjArray<IntrospectionEnumKey> keys_;
	};

	struct IntrospectionDevice : PM_INTROSPECTION_DEVICE
	{
		IntrospectionDevice(uint32_t id_in, PM_DEVICE_TYPE type_in, PM_DEVICE_VENDOR vendor_in, std::string name_in)
			:
			name_{ std::move(name_in) }
		{
			id = id_in;
			type = type_in;
			vendor = vendor_in;
			pName = &name_;
		}
	private:
		IntrospectionString name_;
	};

	struct IntrospectionDeviceMetricInfo : PM_INTROSPECTION_DEVICE_METRIC_INFO {};

	struct IntrospectionDataTypeInfo : PM_INTROSPECTION_DATA_TYPE_INFO {};

	struct IntrospectionMetric : PM_INTROSPECTION_METRIC
	{
		IntrospectionMetric(PM_METRIC id_in, PM_METRIC_TYPE type_in, PM_UNIT unit_in, const IntrospectionDataTypeInfo& typeInfo_in, std::vector<PM_STAT> stats_in = {})
		{
			id = id_in;
			type = type_in;
			unit = unit_in;
			typeInfo = typeInfo_in;
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
	private:
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
	private:
		IntrospectionObjArray<IntrospectionMetric> metrics_;
		IntrospectionObjArray<IntrospectionEnum> enums_;
		IntrospectionObjArray<IntrospectionDevice> devices_;
	};
}
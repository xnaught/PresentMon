#include "MockMiddleware.h"
#include <cstring>
#include <string>
#include <vector>
#include <memory>

namespace pmid
{
	MockMiddleware::MockMiddleware() = default;

	void MockMiddleware::Speak(char* buffer) const
	{
		strcpy_s(buffer, 256, "mock-middle");
	}

	// implement intro string
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
	// implement intro objarr
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
	// implement intro enum structs
	struct IntrospectionEnumKey : PM_INTROSPECTION_ENUM_KEY
	{
		IntrospectionEnumKey(PM_ENUM enumId_, int value_, std::string symbol, std::string name, std::string shortName, std::string abbreviation, std::string description)
		{
			enumId = enumId_;
			value = value_;
			pSymbol = new IntrospectionString{ std::move(symbol) };
			pName = new IntrospectionString{ std::move(name) };
			pShortName = new IntrospectionString{ std::move(shortName) };
			pAbbreviation = new IntrospectionString{ std::move(abbreviation) };
			pDescription = new IntrospectionString{ std::move(description) };
		}
		~IntrospectionEnumKey()
		{
			delete static_cast<IntrospectionString*>(pSymbol);
			delete static_cast<IntrospectionString*>(pName);
			delete static_cast<IntrospectionString*>(pShortName);
			delete static_cast<IntrospectionString*>(pAbbreviation);
			delete static_cast<IntrospectionString*>(pDescription);
		}
	};
	struct IntrospectionEnum : PM_INTROSPECTION_ENUM
	{
		IntrospectionEnum(PM_ENUM id_, std::string symbol, std::string description)
		{
			id = id_;
			pSymbol = new IntrospectionString{ std::move(symbol) };
			pDescription = new IntrospectionString{ std::move(description) };
			pKeys = new IntrospectionObjArray<IntrospectionEnumKey>();
		}
		~IntrospectionEnum()
		{
			delete static_cast<IntrospectionString*>(pSymbol);
			delete static_cast<IntrospectionString*>(pDescription);
			delete &Keys_();
		}
		void AddKey(std::unique_ptr<IntrospectionEnumKey> pKey)
		{
			Keys_().PushBack(std::move(pKey));
		}
	private:
		IntrospectionObjArray<IntrospectionEnumKey>& Keys_()
		{
			return *static_cast<IntrospectionObjArray<IntrospectionEnumKey>*>(pKeys);
		}
	};
	struct IntrospectionDevice : PM_INTROSPECTION_DEVICE
	{
		IntrospectionDevice(uint32_t id_, PM_DEVICE_TYPE type_, PM_DEVICE_VENDOR vendor_, std::string name_)
		{
			id = id_;
			type = type_;
			vendor = vendor_;
			pName = new IntrospectionString{ std::move(name_) };
		}
		~IntrospectionDevice()
		{
			delete static_cast<IntrospectionString*>(pName);
		}
	};
	struct IntrospectionDeviceMetricInfo : PM_INTROSPECTION_DEVICE_METRIC_INFO {};
	struct IntrospectionDataTypeInfo : PM_INTROSPECTION_DATA_TYPE_INFO {};
	struct IntrospectionMetric : PM_INTROSPECTION_METRIC
	{
		IntrospectionMetric(PM_METRIC id_, PM_UNIT unit_, IntrospectionDataTypeInfo typeInfo_, std::vector<PM_STAT> stats_ = {})
			:
			PM_INTROSPECTION_METRIC{ id_, unit_, typeInfo_, new IntrospectionObjArray<PM_STAT>, new IntrospectionObjArray<IntrospectionDeviceMetricInfo> }
		{
			AddStats(std::move(stats_));
		}
		~IntrospectionMetric()
		{
			delete &Stats_();
			delete &DeviceMetricInfo_();
		}
		void AddStat(PM_STAT stat)
		{
			Stats_().PushBack(std::make_unique<PM_STAT>(stat));
		}
		void AddStats(std::vector<PM_STAT> stats)
		{
			for (auto stat : stats) {
				Stats_().PushBack(std::make_unique<PM_STAT>(stat));
			}
		}
		void AddDeviceMetricInfo(IntrospectionDeviceMetricInfo info)
		{
			DeviceMetricInfo_().PushBack(std::make_unique<IntrospectionDeviceMetricInfo>(info));
		}
	private:
		IntrospectionObjArray<PM_STAT>& Stats_()
		{
			return *static_cast<IntrospectionObjArray<PM_STAT>*>(pStats);
		}
		IntrospectionObjArray<IntrospectionDeviceMetricInfo>& DeviceMetricInfo_()
		{
			return *static_cast<IntrospectionObjArray<IntrospectionDeviceMetricInfo>*>(pDeviceMetricInfo);
		}
	};
	struct IntrospectionRoot : PM_INTROSPECTION_ROOT
	{
		IntrospectionRoot()
		{
			pMetrics = new IntrospectionObjArray<IntrospectionMetric>();
			pEnums = new IntrospectionObjArray<IntrospectionEnum>();
			pDevices = new IntrospectionObjArray<IntrospectionDevice>();
		}
		~IntrospectionRoot()
		{
			delete &Enums_();
			delete &Metrics_();
			delete &Devices_();
		}
		void AddEnum(std::unique_ptr<IntrospectionEnum> pEnum)
		{
			Enums_().PushBack(std::move(pEnum));
		}
		void AddMetric(std::unique_ptr<IntrospectionMetric> pMetric)
		{
			Metrics_().PushBack(std::move(pMetric));
		}
		void AddDevice(std::unique_ptr<IntrospectionDevice> pDevice)
		{
			Devices_().PushBack(std::move(pDevice));
		}
	private:
		IntrospectionObjArray<IntrospectionEnum>& Enums_()
		{
			return *static_cast<IntrospectionObjArray<IntrospectionEnum>*>(pEnums);
		}
		IntrospectionObjArray<IntrospectionMetric>& Metrics_()
		{
			return *static_cast<IntrospectionObjArray<IntrospectionMetric>*>(pMetrics);
		}
		IntrospectionObjArray<IntrospectionDevice>& Devices_()
		{
			return *static_cast<IntrospectionObjArray<IntrospectionDevice>*>(pDevices);
		}
	};

	// enum annotations (enum_name_fragment, key_name_fragment, name, short_name, abbreviation, description)
	#define ENUM_KEY_LIST_STATUS(X_) \
		X_(STATUS, SUCCESS, "Success", "", "", "Operation succeeded") \
		X_(STATUS, FAILURE, "Failure", "", "", "Operation failed") \
		X_(STATUS, SESSION_NOT_OPEN, "Session Not Open", "", "", "Operation requires an open session")
	#define ENUM_KEY_LIST_METRIC(X_) \
		X_(METRIC, DISPLAYED_FPS, "Displayed FPS", "", "", "Rate of frame change measurable at display") \
		X_(METRIC, PRESENTED_FPS, "Presented FPS", "", "", "Rate of application calls to a Present() function") \
		X_(METRIC, FRAME_TIME, "Frame Time", "", "", "Time taken to generate a frame") \
		X_(METRIC, GPU_POWER, "GPU Power", "", "", "Power consumed by the graphics adapter") \
		X_(METRIC, CPU_UTILIZATION, "CPU Utilization", "", "", "Amount of CPU processing capacity being used") \
		X_(METRIC, GPU_FAN_SPEED, "GPU Fan Speed", "", "", "Rate at which a GPU cooler fan is rotating")
	#define ENUM_KEY_LIST_DEVICE_VENDOR(X_) \
		X_(DEVICE_VENDOR, INTEL, "Intel", "", "", "Device vendor Intel") \
		X_(DEVICE_VENDOR, NVIDIA, "NVIDIA", "", "", "Device vendor NVIDIA") \
		X_(DEVICE_VENDOR, AMD, "AMD", "", "", "Device vendor AMD") \
		X_(DEVICE_VENDOR, UNKNOWN, "Unknown", "", "", "Unknown device vendor")
	#define ENUM_KEY_LIST_PRESENT_MODE(X_) \
		X_(PRESENT_MODE, HARDWARE_LEGACY_FLIP, "Hardware Legacy Flip", "", "", "Legacy flip present with direct control of hardware scanout frame buffer (fullscreen exclusive)") \
		X_(PRESENT_MODE, HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER, "Hardware Legacy Copy to Front Buffer", "", "", "Legacy bitblt model present copying from backbuffer to scanout frame buffer (fullscreen exclusive)") \
		X_(PRESENT_MODE, HARDWARE_INDEPENDENT_FLIP, "Hardware Independent Flip", "", "", "Application window client area covers display output and its swap chain is being used directly for scanout without DWM intervention") \
		X_(PRESENT_MODE, COMPOSED_FLIP, "Composed Flip", "", "", "Application front buffer is being composed by DWM to final scanout frame buffer") \
		X_(PRESENT_MODE, HARDWARE_COMPOSED_INDEPENDENT_FLIP, "Hardware Composed Independent Flip", "", "", "Application is able to directly write into final scanout frame buffer to compose itself without DWM intervention") \
		X_(PRESENT_MODE, COMPOSED_COPY_WITH_GPU_GDI, "Composed Copy with GPU GDI", "", "", "GDI bitblt composition using the GPU") \
		X_(PRESENT_MODE, COMPOSED_COPY_WITH_CPU_GDI, "Composed Copy with CPU GDI", "", "", "GDI bitblt composition using the CPU") \
		X_(PRESENT_MODE, UNKNOWN, "Unknown", "", "", "Unknown present mode")
	#define ENUM_KEY_LIST_PSU_TYPE(X_) \
		X_(PSU_TYPE, NONE, "None", "", "", "No power supply information") \
		X_(PSU_TYPE, PCIE, "PCIE", "", "", "Power supplied from PCIE bus") \
		X_(PSU_TYPE, 6PIN, "6PIN", "", "", "Power supplied from 6-pin power connector") \
		X_(PSU_TYPE, 8PIN, "8PIN", "", "", "Power supplied from 8-pin power connector")
	#define ENUM_KEY_LIST_UNIT(X_) \
		X_(UNIT, DIMENSIONLESS, "Dimensionless", "", "", "Dimensionless numeric metric") \
		X_(UNIT, BOOLEAN, "Boolean", "", "", "Boolean value with 1 indicating present/active and 0 indicating vacant/inactive") \
		X_(UNIT, FPS, "Frames Per Second", "", "fps", "Rate of application frames being presented per unit time") \
		X_(UNIT, MILLISECONDS, "Milliseconds", "", "ms", "Time duration in milliseconds") \
		X_(UNIT, PERCENT, "Percent", "", "%", "Proportion or ratio represented as a fraction of 100") \
		X_(UNIT, WATTS, "Watts", "", "W", "Power in watts (Joules per second)") \
		X_(UNIT, SYNC_INTERVAL, "Sync Intervals", "", "vblk", "Value indicating a count/duration of display vsync intervals (also known as vertical blanks)") \
		X_(UNIT, VOLTS, "Volts", "", "V", "Electric potential") \
		X_(UNIT, MEGAHERTZ, "Megahertz", "", "MHz", "Frequency in millions of cycles per second") \
		X_(UNIT, CELSIUS, "Degrees Celsius", "", "C", "Temperature in degrees Celsius") \
		X_(UNIT, RPM, "Revolutions per Minute", "", "RPM", "Angular speed in revolutions per minute") \
		X_(UNIT, BPS, "Bits per Second", "", "bps", "Bandwidth / data throughput in bits per second") \
		X_(UNIT, BYTES, "Bytes", "", "B", "Bandwidth / data throughput in bits per second")
	#define ENUM_KEY_LIST_STAT(X_) \
		X_(STAT, AVG, "Average", "", "avg", "Average or mean of frame samples over the sliding window") \
		X_(STAT, PERCENTILE_99, "99th Percentile", "", "99%", "Value below which 99% of the observations within the sliding window fall (worst 1% value)") \
		X_(STAT, PERCENTILE_95, "95th Percentile", "", "95%", "Value below which 95% of the observations within the sliding window fall (worst 5% value)") \
		X_(STAT, PERCENTILE_90, "90th Percentile", "", "90%", "Value below which 90% of the observations within the sliding window fall (worst 10% value)") \
		X_(STAT, MAX, "Maximum", "", "max", "Maximum value of frame samples within the sliding window") \
		X_(STAT, MIN, "Minimum", "", "min", "Minimum value of frame samples within the sliding window") \
		X_(STAT, RAW, "Raw", "", "raw", "Point sample of the frame data with timestamp nearest to polling timestamp (does not use sliding window)")
	#define ENUM_KEY_LIST_DATA_TYPE(X_) \
		X_(DATA_TYPE, DOUBLE, "Double Precision Floating Point", "double", "", "64-bit double precision floating point number in IEEE 754 format") \
		X_(DATA_TYPE, INT32, "32-bit Signed Integer", "int32_t", "", "32-bit signed integer") \
		X_(DATA_TYPE, UINT32, "32-bit Unsigned Integer", "uint32_t", "", "32-bit unsigned integer") \
		X_(DATA_TYPE, ENUM, "Enumeration", "enum", "", "Integral value of an enum key, guaranteed to fit within a 32-bit signed integer") \
		X_(DATA_TYPE, STRING, "String", "const char*", "", "Textual value, typically for non-numeric data")
	#define ENUM_KEY_LIST_GRAPHICS_RUNTIME(X_) \
		X_(GRAPHICS_RUNTIME, UNKNOWN, "Unknown", "", "", "Unknown graphics runtime") \
		X_(GRAPHICS_RUNTIME, DXGI, "DXGI", "", "", "DirectX Graphics Infrastructure runtime") \
		X_(GRAPHICS_RUNTIME, D3D9, "Direct3D 9", "", "", "Direct3D 9 runtime")
	#define ENUM_KEY_LIST_DEVICE_TYPE(X_) \
		X_(DEVICE_TYPE, INDEPENDENT, "Device Independent", "", "", "This device type is used for special device ID 0 which is reserved for metrics independent of any specific hardware device (e.g. FPS metrics)") \
		X_(DEVICE_TYPE, GRAPHICS_ADAPTER, "Graphics Adapter", "", "", "Graphics adapter or GPU device")
	#define ENUM_KEY_LIST_METRIC_AVAILABILITY(X_) \
		X_(METRIC_AVAILABILITY, AVAILABLE, "Available", "", "", "Metric is available on the indicated device") \
		X_(METRIC_AVAILABILITY, UNAVAILABLE, "Unavailable", "", "", "Metric is unavailable on the indicated device")
	// master list of enums as an enum giving each enum a unique id
	#define ENUM_KEY_LIST_ENUM(X_) \
		X_(ENUM, STATUS, "Statuses", "", "", "List of all status/error codes returned by API functions") \
		X_(ENUM, METRIC, "Metrics", "", "", "List of all metrics that are pollable via the API") \
		X_(ENUM, DEVICE_VENDOR, "Vendors", "", "", "List of all known hardware vendors (IHVs) for GPUs and other hardware") \
		X_(ENUM, PRESENT_MODE, "Present Modes", "", "", "List of all known modes of frame presentation") \
		X_(ENUM, PSU_TYPE, "PSU Types", "", "", "Type of power supply used by GPUs") \
		X_(ENUM, UNIT, "Units", "", "", "List of all units of measure used for metrics") \
		X_(ENUM, STAT, "Statistics", "", "", "List of all statistical variations of the data (average, 99th percentile, etc.)") \
		X_(ENUM, DATA_TYPE, "Data Types", "", "", "List of all C++ language data types used for metrics") \
		X_(ENUM, GRAPHICS_RUNTIME, "Graphics Runtime", "", "", "Graphics runtime subsystem used to make the present call") \
		X_(ENUM, DEVICE_TYPE, "Device Type", "", "", "Type of device in the list of devices associated with metrics") \
		X_(ENUM, METRIC_AVAILABILITY, "Metric Availability", "", "", "Availability status of a metric with respect to a given device")
	// list of metrics
	#define FULL_STATS PM_STAT_AVG, PM_STAT_PERCENTILE_99, PM_STAT_PERCENTILE_95, PM_STAT_PERCENTILE_90, PM_STAT_MAX, PM_STAT_MIN, PM_STAT_RAW
	#define METRIC_LIST(X_) \
		X_(PM_METRIC_DISPLAYED_FPS, PM_UNIT_FPS, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_PRESENTED_FPS, PM_UNIT_FPS, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_FRAME_TIME, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_GPU_POWER, PM_UNIT_WATTS, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_CPU_UTILIZATION, PM_UNIT_PERCENT, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_GPU_FAN_SPEED, PM_UNIT_RPM, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS)

#define XSTR_(macro) #macro
#define STRINGIFY_MACRO_CALL(macro) XSTR_(macro)
#define MAKE_MASTER_SYMBOL(enum_frag) PM_ENUM_##enum_frag
#define MAKE_ENUM_SYMBOL(enum_frag) PM_##enum_frag
#define CREATE_INTROSPECTION_ENUM(enum_frag, description) \
	std::make_unique<IntrospectionEnum>(MAKE_MASTER_SYMBOL(enum_frag), STRINGIFY_MACRO_CALL(MAKE_ENUM_SYMBOL(enum_frag)), description)
#define MAKE_LIST_SYMBOL(enum_frag) ENUM_KEY_LIST_##enum_frag
#define MAKE_KEY_SYMBOL(enum_frag, key_frag) PM_##enum_frag##_##key_frag
#define REGISTER_ENUM_KEY(p_enum_obj, enum_frag, key_frag, name, short_name, abbreviation, description) p_enum_obj->AddKey(std::make_unique<IntrospectionEnumKey>(MAKE_MASTER_SYMBOL(enum_frag), MAKE_KEY_SYMBOL(enum_frag, key_frag), STRINGIFY_MACRO_CALL(MAKE_KEY_SYMBOL(enum_frag, key_frag)), name, short_name, abbreviation, description))

	
	// function not reference or called, but compiling this allows compiler to check if all enums/enum keys have coverage
	bool ValidateEnumCompleteness()
	{
		// validate each registered enum's keys
#define X_REG_KEYS(enum_frag, key_frag, name, short_name, abbreviation, description) case MAKE_KEY_SYMBOL(enum_frag, key_frag): return false;
#define X_REG_ENUMS(master_frag, enum_frag, name, short_name, abbreviation, description) \
		switch (MAKE_ENUM_SYMBOL(enum_frag)(0)) { \
			MAKE_LIST_SYMBOL(enum_frag)(X_REG_KEYS) \
		}

		ENUM_KEY_LIST_ENUM(X_REG_ENUMS)

#undef X_REG_ENUMS
#undef X_REG_KEYS

		// validate the enumeration that records introspectable enums
#define X_REG_KEYS(enum_frag, key_frag, name, short_name, abbreviation, description) case MAKE_KEY_SYMBOL(enum_frag, key_frag): return false;
		switch (PM_ENUM(0)) {
			ENUM_KEY_LIST_ENUM(X_REG_KEYS)
		}
#undef X_REG_KEYS

		// validate the list of metrics against the metrics enum
#define X_REG_METRICS(metric, ...) case metric: return false;
		switch (PM_METRIC(0)) {
			METRIC_LIST(X_REG_METRICS)
		}
#undef X_REG_METRICS

		return true;
	}

	const PM_INTROSPECTION_ROOT* MockMiddleware::GetIntrospectionData() const
	{		
		auto pRoot = std::make_unique<IntrospectionRoot>();

		// do enum population
#define X_REG_KEYS(enum_frag, key_frag, name, short_name, abbreviation, description) REGISTER_ENUM_KEY(pEnum, enum_frag, key_frag, name, short_name, abbreviation, description);
#define X_REG_ENUMS(master_frag, enum_frag, name, short_name, abbreviation, description) { \
		auto pEnum = CREATE_INTROSPECTION_ENUM(enum_frag, description); \
		MAKE_LIST_SYMBOL(enum_frag)(X_REG_KEYS) \
		pRoot->AddEnum(std::move(pEnum)); }

		ENUM_KEY_LIST_ENUM(X_REG_ENUMS)

#undef X_REG_ENUMS
#undef X_REG_KEYS

		// do device population
		pRoot->AddDevice(std::make_unique<IntrospectionDevice>(0, PM_DEVICE_TYPE_INDEPENDENT, PM_DEVICE_VENDOR_UNKNOWN, "Device-independent"));
		pRoot->AddDevice(std::make_unique<IntrospectionDevice>(1, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_DEVICE_VENDOR_INTEL, "Arc 750"));
		pRoot->AddDevice(std::make_unique<IntrospectionDevice>(2, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_DEVICE_VENDOR_NVIDIA, "GeForce RTX 2080 ti"));

		// helper to generate device-metric info for metrics (simulating runtime population of availability and dimensions)
		const auto PopulateDeviceMetricInfo = [](IntrospectionMetric& metric, PM_DEVICE_TYPE deviceType) {
			if (deviceType == PM_DEVICE_TYPE_INDEPENDENT) {
				metric.AddDeviceMetricInfo({ 0, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
			}
			else if (metric.id == PM_METRIC_GPU_FAN_SPEED) {
				metric.AddDeviceMetricInfo({ 1, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
				metric.AddDeviceMetricInfo({ 2, PM_METRIC_AVAILABILITY_AVAILABLE, 2 });
			}
			else {
				metric.AddDeviceMetricInfo({ 1, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
				metric.AddDeviceMetricInfo({ 2, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
			}
		};

		// do metric population
#define X_REG_METRIC(metric, unit, data_type, enum_id, device_type, ...) { \
		auto pMetric = std::make_unique<IntrospectionMetric>( \
			metric, unit, IntrospectionDataTypeInfo{ data_type, (PM_ENUM)enum_id }, std::vector{ __VA_ARGS__ }); \
		PopulateDeviceMetricInfo(*pMetric, device_type); \
		pRoot->AddMetric(std::move(pMetric)); }

		METRIC_LIST(X_REG_METRIC)

#undef X_REG_METRIC

		return pRoot.release();
	}

	void MockMiddleware::FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) const
	{
		delete static_cast<const IntrospectionRoot*>(pRoot);
	}
}
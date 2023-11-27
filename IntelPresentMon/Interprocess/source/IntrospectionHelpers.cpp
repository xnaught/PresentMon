#include "IntrospectionHelpers.h"
#include "IntrospectionMetadata.h"
#include "IntrospectionTransfer.h"

namespace pmon::ipc::intro
{
	size_t GetDataTypeSize(PM_DATA_TYPE v)
	{
#define X_REG_KEYS(enum_frag, key_frag, name, short_name, description) case MAKE_KEY_SYMBOL(enum_frag, key_frag): return EnumToStaticType_sz<MAKE_KEY_SYMBOL(enum_frag, key_frag)>;
		switch (v) {
			ENUM_KEY_LIST_DATA_TYPE(X_REG_KEYS)
		}
#undef X_REG_KEYS
		return 0;
	}

	void PopulateEnums(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root)
	{
		auto charAlloc = pSegmentManager->get_allocator<char>();

#define X_REG_KEYS(enum_frag, key_frag, name, short_name, description) REGISTER_ENUM_KEY(pSegmentManager, pEnum, enum_frag, key_frag, name, short_name, description);
#define X_REG_ENUMS(master_frag, enum_frag, name, short_name, description) { \
		auto pEnum = CREATE_INTROSPECTION_ENUM(pSegmentManager, enum_frag, description); \
		\
		MAKE_LIST_SYMBOL(enum_frag)(X_REG_KEYS) \
		root.AddEnum(std::move(pEnum)); }

		ENUM_KEY_LIST_ENUM(X_REG_ENUMS)

#undef X_REG_ENUMS
#undef X_REG_KEYS
	}

	void PopulateDevices(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root)
	{
		auto charAlloc = pSegmentManager->get_allocator<char>();

		root.AddDevice(ShmMakeUnique<IntrospectionDevice>(pSegmentManager, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_DEVICE_VENDOR_UNKNOWN, ShmString{ "Device-independent", charAlloc }));
		root.AddDevice(ShmMakeUnique<IntrospectionDevice>(pSegmentManager, 1, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_DEVICE_VENDOR_INTEL, ShmString{ "Arc 750", charAlloc }));
		root.AddDevice(ShmMakeUnique<IntrospectionDevice>(pSegmentManager, 2, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_DEVICE_VENDOR_NVIDIA, ShmString{ "GeForce RTX 2080 ti", charAlloc }));
	}

	void PopulateMetrics(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root)
	{
		// helper to generate mock per-device metric data
		const auto PopulateDeviceMetricInfo = [](IntrospectionMetric& metric, PM_DEVICE_TYPE deviceType) {
			if (deviceType == PM_DEVICE_TYPE_INDEPENDENT) {
				metric.AddDeviceMetricInfo({ 0, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
			}
			else if (metric.GetId() == PM_METRIC_GPU_FAN_SPEED) {
				metric.AddDeviceMetricInfo({ 1, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
				metric.AddDeviceMetricInfo({ 2, PM_METRIC_AVAILABILITY_AVAILABLE, 2 });
			}
			else {
				metric.AddDeviceMetricInfo({ 1, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
				metric.AddDeviceMetricInfo({ 2, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
			}
		};

		// do metric population
#define X_REG_METRIC(metric, metric_type, unit, data_type, enum_id, device_type, ...) { \
		auto pMetric = ShmMakeUnique<IntrospectionMetric>(pSegmentManager, pSegmentManager, \
			metric, metric_type, unit, IntrospectionDataTypeInfo{ data_type, (PM_ENUM)enum_id }, std::vector{ __VA_ARGS__ }); \
		PopulateDeviceMetricInfo(*pMetric, device_type); \
		root.AddMetric(std::move(pMetric)); }

		METRIC_LIST(X_REG_METRIC)

#undef X_REG_METRIC
	}
}
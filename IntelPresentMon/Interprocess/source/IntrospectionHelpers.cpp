#include "IntrospectionHelpers.h"
#include "IntrospectionMetadata.h"
#include "IntrospectionTransfer.h"
#include <array>
#include <ranges>
#include <optional>

namespace rn = std::ranges;
namespace vi = rn::views;

namespace pmon::ipc::intro
{
	// mapping of caps to metrics
	template<PM_METRIC metric> struct IntrospectionCapsLookup { using Universal = std::true_type; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_POWER> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_power; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_FAN_SPEED> { static constexpr auto gpuCapBitArray = std::array{
		GpuTelemetryCapBits::fan_speed_0, GpuTelemetryCapBits::fan_speed_1, GpuTelemetryCapBits::fan_speed_2, GpuTelemetryCapBits::fan_speed_3, GpuTelemetryCapBits::fan_speed_4, };
	};
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_UTILIZATION> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_utilization; };

	// concepts to help determine device-metric mapping type
	template<class T> concept IsUniversalMetric = requires { typename T::Universal; };
	template<class T> concept IsGpuDeviceMetric = requires { T::gpuCapBit; };
	template<class T> concept IsGpuDeviceMetricArray = requires { T::gpuCapBitArray; };
	template<class T> concept IsCpuMetric = requires { T::cpuCapBit; };

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

	template<PM_METRIC metricId>
	void RegisterUniversalMetricDeviceInfo_(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root, IntrospectionMetric& metric)
	{
		using Lookup = IntrospectionCapsLookup<metricId>;
		if constexpr (IsUniversalMetric<Lookup>) {
			metric.AddDeviceMetricInfo(IntrospectionDeviceMetricInfo{
				0, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
		}
	}

	void PopulateMetrics(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root)
	{
#define X_REG_METRIC(metric, metric_type, unit, data_type, enum_id, device_type, ...) { \
		auto pMetric = ShmMakeUnique<IntrospectionMetric>(pSegmentManager, pSegmentManager, \
			metric, metric_type, unit, IntrospectionDataTypeInfo{ data_type, (PM_ENUM)enum_id }, std::vector{ __VA_ARGS__ }); \
		RegisterUniversalMetricDeviceInfo_<metric>(pSegmentManager, root, *pMetric); \
		root.AddMetric(std::move(pMetric)); }

		METRIC_LIST(X_REG_METRIC)

#undef X_REG_METRIC
	}

	template<PM_METRIC metric>
	void RegisterGpuMetricDeviceInfo_(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		uint32_t deviceId, const GpuTelemetryBitset& gpuCaps)
	{
		using Lookup = IntrospectionCapsLookup<metric>;
		std::optional<IntrospectionDeviceMetricInfo> info;
		if constexpr (IsUniversalMetric<Lookup>) {}
		else if constexpr (IsGpuDeviceMetric<Lookup>) {
			const auto availability = gpuCaps[size_t(Lookup::gpuCapBit)] ?
				PM_METRIC_AVAILABILITY_AVAILABLE : PM_METRIC_AVAILABILITY_UNAVAILABLE;
			info.emplace(deviceId, availability, 1);
		}
		else if constexpr (IsGpuDeviceMetricArray<Lookup>) {
			const auto nAvailable = rn::count_if(Lookup::gpuCapBitArray, [&](auto bit){ return gpuCaps[size_t(bit)]; });
			const auto availability = nAvailable > 0 ?
				PM_METRIC_AVAILABILITY_AVAILABLE : PM_METRIC_AVAILABILITY_UNAVAILABLE;
			info.emplace(deviceId, availability, nAvailable);
		}
		if (info) {
			if (auto i = rn::find(root.GetMetrics(), metric, [](const ShmUniquePtr<IntrospectionMetric>& pMetric) {
				return pMetric->GetId();
			}); i != root.GetMetrics().end()) {
				(*i)->AddDeviceMetricInfo(std::move(*info));
			}
			// TODO: log metric not found
		}
	}

	void PopulateGpuDevice(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root, uint32_t deviceId,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const GpuTelemetryBitset& gpuCaps)
	{
		// add the device
		auto charAlloc = pSegmentManager->get_allocator<char>();
		root.AddDevice(ShmMakeUnique<IntrospectionDevice>(pSegmentManager, deviceId,
			PM_DEVICE_TYPE_GRAPHICS_ADAPTER, vendor, ShmString{ deviceName.c_str(), charAlloc}));

		// populate device-metric info for this device
#define X_WALK_METRIC(metric, metric_type, unit, data_type, enum_id, device_type, ...) \
		RegisterGpuMetricDeviceInfo_<metric>(pSegmentManager, root, deviceId, gpuCaps);

		METRIC_LIST(X_WALK_METRIC)

#undef X_WALK_METRIC
	}

	template<PM_METRIC metric>
	void RegisterCpuMetricDeviceInfo_(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		const CpuTelemetryBitset& cpuCaps)
	{
		using Lookup = IntrospectionCapsLookup<metric>;
		if constexpr (IsCpuMetric<Lookup>) {
			if (auto i = rn::find(root.GetMetrics(), metric, [](const ShmUniquePtr<IntrospectionMetric>& pMetric) {
				return pMetric->GetId();
			}); i != root.GetMetrics().end()) {
				const auto availability = cpuCaps[size_t(Lookup::cpuCapBit)] ?
					PM_METRIC_AVAILABILITY_AVAILABLE : PM_METRIC_AVAILABILITY_UNAVAILABLE;
				(*i)->AddDeviceMetricInfo(IntrospectionDeviceMetricInfo{ 0, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
			}
			// TODO: log metric not found
		}
	}

	void PopulateCpu(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const CpuTelemetryBitset& cpuCaps)
	{
		// populate device-metric info for the cpu
#define X_WALK_METRIC(metric, metric_type, unit, data_type, enum_id, device_type, ...) \
		RegisterCpuMetricDeviceInfo_<metric>(pSegmentManager, root, cpuCaps);

		METRIC_LIST(X_WALK_METRIC)

#undef X_WALK_METRIC
	}
}
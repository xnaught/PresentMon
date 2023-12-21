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
	// GPU caps
	//template<> struct IntrospectionCapsLookup<> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::time_stamp; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_POWER> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_power; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_SUSTAINED_POWER_LIMIT> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_sustained_power_limit; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_VOLTAGE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_voltage; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_FREQUENCY> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_frequency; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_UTILIZATION> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_utilization; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_render_compute_utilization; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEDIA_UTILIZATION> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_media_utilization; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_POWER> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_power; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_VOLTAGE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_voltage; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_FREQUENCY> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_frequency; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_EFFECTIVE_FREQUENCY> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_effective_frequency; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_TEMPERATURE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_temperature; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_FAN_SPEED> { static constexpr auto gpuCapBitArray = std::array{
		GpuTelemetryCapBits::fan_speed_0, GpuTelemetryCapBits::fan_speed_1, GpuTelemetryCapBits::fan_speed_2, GpuTelemetryCapBits::fan_speed_3, GpuTelemetryCapBits::fan_speed_4, }; };
	//template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_FAN_SPEED> { static constexpr auto gpuCapBitArray = std::array{
	//	GpuTelemetryCapBits::psu_info_0, GpuTelemetryCapBits::psu_info_1, GpuTelemetryCapBits::psu_info_2, GpuTelemetryCapBits::psu_info_3, GpuTelemetryCapBits::psu_info_4, }; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_SIZE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_size; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_USED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_used; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_MAX_BANDWIDTH> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_max_bandwidth; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_WRITE_BANDWIDTH> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_write_bandwidth; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_READ_BANDWIDTH> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_read_bandwidth; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_POWER_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_power_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_TEMPERATURE_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_temperature_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_CURRENT_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_current_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_VOLTAGE_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_voltage_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_UTILIZATION_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_utilization_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_POWER_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_power_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_TEMPERATURE_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_temperature_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_CURRENT_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_current_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_VOLTAGE_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_voltage_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_VRAM_UTILIZATION_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_utilization_limited; };
	// static GPU
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_NAME> { using GpuDeviceStatic = std::true_type; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_VENDOR> { using GpuDeviceStatic = std::true_type; };
	// CPU caps
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_UTILIZATION> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_utilization; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_POWER> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_power; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_TEMPERATURE> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_temperature; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_FREQUENCY> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_frequency; };
	//template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_CORE_UTILITY> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu; };


	// concepts to help determine device-metric mapping type
	template<class T> concept IsUniversalMetric = requires { typename T::Universal; };
	template<class T> concept IsGpuDeviceMetric = requires { T::gpuCapBit; };
	template<class T> concept IsGpuDeviceMetricArray = requires { T::gpuCapBitArray; };
	template<class T> concept IsGpuDeviceStaticMetric = requires { typename T::GpuDeviceStatic; };
	template<class T> concept IsCpuMetric = requires { T::cpuCapBit; };
	
	// TODO: compile-time verify that all cap bits are covered (how?)


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
		std::unordered_map<PM_METRIC, PM_UNIT> preferredMetricOverrides;
#define X_PREF_UNIT(metric, unit) preferredMetricOverrides[metric] = unit;

		PREFERRED_UNIT_LIST(X_PREF_UNIT)

#undef X_REG_METRIC

#define X_REG_METRIC(metric, metric_type, unit, data_type_polled, data_type_frame, enum_id, device_type, ...) { \
		auto pMetric = ShmMakeUnique<IntrospectionMetric>(pSegmentManager, pSegmentManager, \
			metric, metric_type, unit, IntrospectionDataTypeInfo{ data_type_polled, data_type_frame, (PM_ENUM)enum_id }, std::vector{ __VA_ARGS__ }); \
		RegisterUniversalMetricDeviceInfo_<metric>(pSegmentManager, root, *pMetric); \
		if (preferredMetricOverrides.contains(metric)) pMetric->SetPreferredUnitHint(preferredMetricOverrides[metric]); \
		root.AddMetric(std::move(pMetric)); }

		METRIC_LIST(X_REG_METRIC)

#undef X_REG_METRIC
	}



	void PopulateUnits(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root)
	{
#define X_REG_UNIT(unit, baseUnit, scale) { \
		auto pUnit = ShmMakeUnique<IntrospectionUnit>(pSegmentManager, unit, baseUnit, scale); \
		root.AddUnit(std::move(pUnit)); }

		UNIT_LIST(X_REG_UNIT)

#undef X_REG_UNIT
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
		else if constexpr (IsGpuDeviceStaticMetric<Lookup>) {
			info.emplace(deviceId, PM_METRIC_AVAILABILITY_AVAILABLE, 1);
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
		}

		// TODO: log metric not found
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
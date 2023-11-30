// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#ifdef PRESENTMONAPI2_EXPORTS
#define PRESENTMON_API_EXPORT __declspec(dllexport)
#else
#define PRESENTMON_API_EXPORT __declspec(dllimport)
#endif

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

	enum PM_STATUS // **
	{
		PM_STATUS_SUCCESS,
		PM_STATUS_FAILURE,
		PM_STATUS_SESSION_NOT_OPEN,
		PM_STATUS_SERVICE_ERROR,
		PM_STATUS_INVALID_ETL_FILE,
		PM_STATUS_DATA_LOSS,
		PM_STATUS_NO_DATA,
		PM_STATUS_INVALID_PID,
		PM_STATUS_STREAM_ALREADY_EXISTS,
		PM_STATUS_UNABLE_TO_CREATE_NSM,
		PM_STATUS_INVALID_ADAPTER_ID,
		PM_STATUS_OUT_OF_RANGE,
	};

	enum PM_METRIC // **
	{
		PM_METRIC_DISPLAYED_FPS,
		PM_METRIC_PRESENTED_FPS,
		PM_METRIC_FRAME_TIME,
		PM_METRIC_PRESENT_MODE,
		PM_METRIC_GPU_POWER,
		PM_METRIC_CPU_UTILIZATION,
		PM_METRIC_GPU_FAN_SPEED,
		PM_METRIC_PROCESS_NAME,
		PM_METRIC_GPU_BUSY_TIME,
		PM_METRIC_CPU_BUSY_TIME,
		PM_METRIC_CPU_WAIT_TIME,
		PM_METRIC_CPU_CORE_UTILITY,
		PM_METRIC_INPUT_LATENCY_TIME,
		PM_METRIC_DISPLAY_BUSY_TIME,
	};

	enum PM_METRIC_TYPE
	{
		PM_METRIC_TYPE_DYNAMIC,
		PM_METRIC_TYPE_STATIC,
	};

	enum PM_DEVICE_VENDOR
	{
		PM_DEVICE_VENDOR_INTEL,
		PM_DEVICE_VENDOR_NVIDIA,
		PM_DEVICE_VENDOR_AMD,
		PM_DEVICE_VENDOR_UNKNOWN
	};

	enum PM_PRESENT_MODE
	{
		PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP,
		PM_PRESENT_MODE_HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER,
		PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP,
		PM_PRESENT_MODE_COMPOSED_FLIP,
		PM_PRESENT_MODE_HARDWARE_COMPOSED_INDEPENDENT_FLIP,
		PM_PRESENT_MODE_COMPOSED_COPY_WITH_GPU_GDI,
		PM_PRESENT_MODE_COMPOSED_COPY_WITH_CPU_GDI,
		PM_PRESENT_MODE_UNKNOWN
	};

	enum PM_PSU_TYPE
	{
		PM_PSU_TYPE_NONE,
		PM_PSU_TYPE_PCIE,
		PM_PSU_TYPE_6PIN,
		PM_PSU_TYPE_8PIN
	};

	enum PM_UNIT
	{
		PM_UNIT_DIMENSIONLESS,
		PM_UNIT_BOOLEAN,
		PM_UNIT_FPS,
		PM_UNIT_MILLISECONDS,
		PM_UNIT_PERCENT,
		PM_UNIT_WATTS,
		PM_UNIT_SYNC_INTERVAL,
		PM_UNIT_VOLTS,
		PM_UNIT_MEGAHERTZ,
		PM_UNIT_CELSIUS,
		PM_UNIT_RPM,
		PM_UNIT_BPS,
		PM_UNIT_BYTES,
	};

	enum PM_STAT
	{
		PM_STAT_AVG,
		PM_STAT_PERCENTILE_99,
		PM_STAT_PERCENTILE_95,
		PM_STAT_PERCENTILE_90,
		PM_STAT_MAX,
		PM_STAT_MIN,
		PM_STAT_RAW,
	};

	enum PM_DATA_TYPE
	{
		PM_DATA_TYPE_DOUBLE,
		PM_DATA_TYPE_INT32,
		PM_DATA_TYPE_UINT32,
		PM_DATA_TYPE_ENUM,
		PM_DATA_TYPE_STRING,
	};

	enum PM_GRAPHICS_RUNTIME
	{
		PM_GRAPHICS_RUNTIME_UNKNOWN,
		PM_GRAPHICS_RUNTIME_DXGI,
		PM_GRAPHICS_RUNTIME_D3D9,
	};

	enum PM_DEVICE_TYPE
	{
		PM_DEVICE_TYPE_INDEPENDENT,
		PM_DEVICE_TYPE_GRAPHICS_ADAPTER,
	};

	enum PM_METRIC_AVAILABILITY
	{
		PM_METRIC_AVAILABILITY_AVAILABLE,
		PM_METRIC_AVAILABILITY_UNAVAILABLE,
	};

	enum PM_ENUM
	{
		PM_ENUM_STATUS,
		PM_ENUM_METRIC,
		PM_ENUM_METRIC_TYPE,
		PM_ENUM_DEVICE_VENDOR,
		PM_ENUM_PRESENT_MODE,
		PM_ENUM_PSU_TYPE,
		PM_ENUM_UNIT,
		PM_ENUM_STAT,
		PM_ENUM_DATA_TYPE,
		PM_ENUM_GRAPHICS_RUNTIME,
		PM_ENUM_DEVICE_TYPE,
		PM_ENUM_METRIC_AVAILABILITY,
	};

	struct PM_INTROSPECTION_STRING
	{
		const char* pData;
	};

	struct PM_INTROSPECTION_OBJARRAY
	{
		const void** pData;
		size_t size;
	};

	struct PM_INTROSPECTION_ENUM_KEY
	{
		PM_ENUM enumId;
		int value;
		PM_INTROSPECTION_STRING* pSymbol;
		PM_INTROSPECTION_STRING* pName;
		PM_INTROSPECTION_STRING* pShortName;
		PM_INTROSPECTION_STRING* pDescription;
	};

	struct PM_INTROSPECTION_ENUM
	{
		PM_ENUM id;
		PM_INTROSPECTION_STRING* pSymbol;
		PM_INTROSPECTION_STRING* pDescription;
		PM_INTROSPECTION_OBJARRAY* pKeys;
	};

	struct PM_INTROSPECTION_DEVICE
	{
		uint32_t id;
		PM_DEVICE_TYPE type;
		PM_DEVICE_VENDOR vendor;
		PM_INTROSPECTION_STRING* pName;
	};

	struct PM_INTROSPECTION_DEVICE_METRIC_INFO
	{
		uint32_t deviceId;
		PM_METRIC_AVAILABILITY availability;
		uint32_t arraySize;
	};

	struct PM_INTROSPECTION_DATA_TYPE_INFO
	{
		PM_DATA_TYPE type;
		PM_ENUM enumId;
	};

	struct PM_INTROSPECTION_STAT_INFO
	{
		PM_STAT stat;
	};

	struct PM_INTROSPECTION_METRIC
	{
		PM_METRIC id;
		PM_METRIC_TYPE type;
		PM_UNIT unit;
		PM_INTROSPECTION_DATA_TYPE_INFO* pTypeInfo;
		PM_INTROSPECTION_OBJARRAY* pStatInfo;
		PM_INTROSPECTION_OBJARRAY* pDeviceMetricInfo;
	};

	struct PM_INTROSPECTION_ROOT
	{
		PM_INTROSPECTION_OBJARRAY* pMetrics;
		PM_INTROSPECTION_OBJARRAY* pEnums;
		PM_INTROSPECTION_OBJARRAY* pDevices;
	};

	struct PM_QUERY_ELEMENT
	{
		PM_METRIC metric;
		PM_STAT stat;
		uint32_t deviceId;
		uint32_t arrayIndex;
		uint64_t dataOffset;
		uint64_t dataSize;
	};

	typedef struct PM_DYNAMIC_QUERY* PM_DYNAMIC_QUERY_HANDLE;
	PRESENTMON_API_EXPORT PM_STATUS pmInitialize();
	PRESENTMON_API_EXPORT PM_STATUS pmShutdown();
	PRESENTMON_API_EXPORT PM_STATUS pmOpenSession(uint32_t process_id);
	PRESENTMON_API_EXPORT PM_STATUS pmCloseSession(uint32_t process_id);
	PRESENTMON_API_EXPORT PM_STATUS pmEnumerateInterface(const PM_INTROSPECTION_ROOT** ppInterface);
	PRESENTMON_API_EXPORT PM_STATUS pmFreeInterface(const PM_INTROSPECTION_ROOT* pInterface);
	PRESENTMON_API_EXPORT PM_STATUS pmRegisterDynamicQuery(PM_DYNAMIC_QUERY_HANDLE* pHandle, PM_QUERY_ELEMENT* pElements, uint64_t numElements, double windowSizeMs, double metricOffsetMs = 0.f);
	PRESENTMON_API_EXPORT PM_STATUS pmFreeDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle);
	PRESENTMON_API_EXPORT PM_STATUS pmPollDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle, uint8_t* pBlob, uint32_t* numSwapChains);
	PRESENTMON_API_EXPORT PM_STATUS pmPollStaticQuery(const PM_QUERY_ELEMENT* pElement, uint8_t* pBlob);
	

	/////// draft functions ///////
	
	//// process tracking (start "streaming" frame events) function
	// start streaming
	// stop streaming
	
	//// consuming frame events
	// read frames
	
	//// setting global middleware settings
	// offset (could this be per call? or set in the dynamic query registration?)
	
	//// setting global service settings
	// rate of polling telemetry (set per device?)
	
	//// offline etl streaming session
	// input the file
	// 

	/////// compilation fixes that can be REMOVED ///////
	#define MAX_PM_ADAPTER_NAME 64
	#define MAX_PM_CPU_NAME 256
	#define MAX_RUNTIME_LENGTH 7
	#define MAX_PM_PATH 260
	#define MAX_PM_FAN_COUNT 5
	#define MAX_PM_PSU_COUNT 5
	#define MIN_PM_TELEMETRY_PERIOD 1
	#define MAX_PM_TELEMETRY_PERIOD 1000

	struct PM_ADAPTER_INFO
	{
		uint32_t id;
		PM_DEVICE_VENDOR vendor;
		char name[MAX_PM_ADAPTER_NAME];
	};

	struct PM_FRAME_DATA_OPT_DOUBLE {
		double data;
		bool valid;
	};

	struct PM_FRAME_DATA_OPT_UINT64 {
		uint64_t data;
		bool valid;
	};

	struct PM_FRAME_DATA_OPT_INT {
		int data;
		bool valid;
	};

	struct PM_FRAME_DATA_OPT_PSU_TYPE {
		PM_PSU_TYPE data;
		bool valid;
	};

	struct PM_FRAME_DATA
	{
		// @brief The name of the process that called Present().
		char application[MAX_PM_PATH];

		// @brief The process ID of the process that called Present().
		uint32_t process_id;
		// @brief The address of the swap chain that was presented into.
		uint64_t swap_chain_address;
		// @brief The runtime used to present (e.g., D3D9 or DXGI).
		char runtime[MAX_RUNTIME_LENGTH];
		// @brief The sync interval provided by the application in the Present()
		// call. This value may be modified later by the driver, e.g., based on
		// control panel overrides.
		int32_t sync_interval;
		// @brief Flags used in the Present() call.
		uint32_t present_flags;
		// @brief Whether the frame was dropped (1) or displayed (0).  Note, if
		// dropped, msUntilDisplayed will be 0.
		uint32_t dropped;
		// @brief The time of the Present() call, in seconds, relative to when the
		// PresentMon started recording.
		double time_in_seconds;
		// @brief The time spent inside the Present() call, in milliseconds.
		double ms_in_present_api;
		// @brief The time between this Present() call and the previous one, in
		// milliseconds.
		double ms_between_presents;
		// @brief Whether tearing is possible (1) or not (0).
		uint32_t allows_tearing;
		// @brief The presentation mode used by the system for this Present().
		PM_PRESENT_MODE present_mode;
		// @brief The time between the Present() call and when the GPU work
		// completed, in milliseconds.
		double ms_until_render_complete;
		// @brief The time between the Present() call and when the frame was
		// displayed, in milliseconds.
		double ms_until_displayed;
		// @brief How long the previous frame was displayed before this Present()
		// was displayed, in milliseconds.
		double ms_between_display_change;
		// @brief The time between the Present() call and when the GPU work
		// started, in milliseconds.
		double ms_until_render_start;
		// @brief The time of the Present() call, as a performance counter value.
		uint64_t qpc_time;

		// @brief The time between the Present() call and the earliest keyboard or
		// mouse interaction that contributed to this frame.
		double ms_since_input;
		// @brief The time that any GPU engine was active working on this frame,
		// in milliseconds. Not supported on Win7
		double ms_gpu_active;
		// @brief The time video encode/decode was active separate from the other
		// engines in milliseconds. Not supported on Win7
		double ms_gpu_video_active;

		// Power telemetry
		PM_FRAME_DATA_OPT_DOUBLE gpu_power_w;
		PM_FRAME_DATA_OPT_DOUBLE gpu_sustained_power_limit_w;
		PM_FRAME_DATA_OPT_DOUBLE gpu_voltage_v;
		PM_FRAME_DATA_OPT_DOUBLE gpu_frequency_mhz;
		PM_FRAME_DATA_OPT_DOUBLE gpu_temperature_c;
		PM_FRAME_DATA_OPT_DOUBLE gpu_utilization;
		PM_FRAME_DATA_OPT_DOUBLE gpu_render_compute_utilization;
		PM_FRAME_DATA_OPT_DOUBLE gpu_media_utilization;

		PM_FRAME_DATA_OPT_DOUBLE vram_power_w;
		PM_FRAME_DATA_OPT_DOUBLE vram_voltage_v;
		PM_FRAME_DATA_OPT_DOUBLE vram_frequency_mhz;
		PM_FRAME_DATA_OPT_DOUBLE vram_effective_frequency_gbs;
		PM_FRAME_DATA_OPT_DOUBLE vram_temperature_c;

		PM_FRAME_DATA_OPT_DOUBLE fan_speed_rpm[MAX_PM_FAN_COUNT];

		PM_FRAME_DATA_OPT_PSU_TYPE psu_type[MAX_PM_PSU_COUNT];
		PM_FRAME_DATA_OPT_DOUBLE psu_power[MAX_PM_PSU_COUNT];
		PM_FRAME_DATA_OPT_DOUBLE psu_voltage[MAX_PM_PSU_COUNT];

		// Gpu memory telemetry
		PM_FRAME_DATA_OPT_UINT64 gpu_mem_total_size_b;
		PM_FRAME_DATA_OPT_UINT64 gpu_mem_used_b;
		PM_FRAME_DATA_OPT_UINT64 gpu_mem_max_bandwidth_bps;
		PM_FRAME_DATA_OPT_DOUBLE gpu_mem_read_bandwidth_bps;
		PM_FRAME_DATA_OPT_DOUBLE gpu_mem_write_bandwidth_bps;

		// Throttling flags
		PM_FRAME_DATA_OPT_INT gpu_power_limited;
		PM_FRAME_DATA_OPT_INT gpu_temperature_limited;
		PM_FRAME_DATA_OPT_INT gpu_current_limited;
		PM_FRAME_DATA_OPT_INT gpu_voltage_limited;
		PM_FRAME_DATA_OPT_INT gpu_utilization_limited;

		PM_FRAME_DATA_OPT_INT vram_power_limited;
		PM_FRAME_DATA_OPT_INT vram_temperature_limited;
		PM_FRAME_DATA_OPT_INT vram_current_limited;
		PM_FRAME_DATA_OPT_INT vram_voltage_limited;
		PM_FRAME_DATA_OPT_INT vram_utilization_limited;

		// Cpu Telemetry
		PM_FRAME_DATA_OPT_DOUBLE cpu_utilization;
		PM_FRAME_DATA_OPT_DOUBLE cpu_power_w;
		PM_FRAME_DATA_OPT_DOUBLE cpu_power_limit_w;
		PM_FRAME_DATA_OPT_DOUBLE cpu_temperature_c;
		PM_FRAME_DATA_OPT_DOUBLE cpu_frequency;
	};

	struct PM_METRIC_DOUBLE_DATA {
		double avg;
		double percentile_99;
		double percentile_95;
		double percentile_90;
		double high;
		double low;
		double raw;
		bool valid;
	};

	struct PM_FPS_DATA
	{
		uint64_t swap_chain;

		PM_METRIC_DOUBLE_DATA displayed_fps;
		PM_METRIC_DOUBLE_DATA presented_fps;
		PM_METRIC_DOUBLE_DATA frame_time_ms;

		PM_METRIC_DOUBLE_DATA gpu_busy;
		PM_METRIC_DOUBLE_DATA percent_dropped_frames;

		uint32_t num_presents;

		int32_t sync_interval;
		PM_PRESENT_MODE present_mode;
		int32_t allows_tearing;
	};

	struct PM_GFX_LATENCY_DATA
	{
		uint64_t swap_chain;
		PM_METRIC_DOUBLE_DATA render_latency_ms;
		PM_METRIC_DOUBLE_DATA display_latency_ms;
	};

#ifdef __cplusplus
} // extern "C"
#endif
#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "IntrospectionMacroHelpers.h"

namespace pmon::ipc::intro {
	// enum annotations (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_STATUS(X_) \
		X_(STATUS, SUCCESS, "Success", "", "Operation succeeded") \
		X_(STATUS, FAILURE, "Failure", "", "Operation failed") \
		X_(STATUS, SESSION_NOT_OPEN, "Session Not Open", "", "Operation requires an open session") \
		X_(STATUS, SERVICE_ERROR, "", "", "") \
		X_(STATUS, INVALID_ETL_FILE, "", "", "") \
		X_(STATUS, DATA_LOSS, "", "", "") \
		X_(STATUS, NO_DATA, "", "", "") \
		X_(STATUS, INVALID_PID, "", "", "") \
		X_(STATUS, STREAM_ALREADY_EXISTS, "", "", "") \
		X_(STATUS, UNABLE_TO_CREATE_NSM, "", "", "") \
		X_(STATUS, INVALID_ADAPTER_ID, "", "", "") \
		X_(STATUS, OUT_OF_RANGE, "", "", "") \
		X_(STATUS, INSUFFICIENT_BUFFER, "", "", "")
#define ENUM_KEY_LIST_METRIC(X_) \
		X_(METRIC, SWAP_CHAIN, "Swap Chain Address", "", "Address of the swap chain used to present, useful as a unique identifier") \
		X_(METRIC, DISPLAYED_FPS, "Displayed FPS", "", "Rate of frame change measurable at display") \
		X_(METRIC, PRESENTED_FPS, "Presented FPS", "", "Rate of application calls to a Present() function") \
		X_(METRIC, FRAME_TIME, "Frame Time", "", "Time taken to generate a frame") \
		X_(METRIC, GPU_BUSY_TIME, "GPU Busy Time", "", "Time GPU was busy") \
		X_(METRIC, DISPLAY_BUSY_TIME, "Display Busy Time", "", "Time display was busy") \
		X_(METRIC, CPU_BUSY_TIME, "CPU Busy Time", "", "Time CPU was busy") \
		X_(METRIC, CPU_WAIT_TIME, "CPU Wait Time", "", "Time CPU was waiting") \
		X_(METRIC, DROPPED_FRAMES, "Dropped Frames", "", "Indicates if the frame was not displayed") \
		X_(METRIC, NUM_PRESENTS, "Number of Presents", "", "Number of present calls") \
		X_(METRIC, SYNC_INTERVAL, "Sync Interval", "", "The application's requested interval between presents measured in vertical sync/vblank events") \
		X_(METRIC, PRESENT_MODE, "Present Mode", "", "Method used to present the frame") \
		X_(METRIC, PRESENT_RUNTIME, "Present Runtime", "", "The graphics runtime used for the present operation (DXGI, D3D9, etc.)") \
		X_(METRIC, PRESENT_QPC, "Present QPC Timestamp", "", "The high performance timestamp corresponding to the present") \
		X_(METRIC, ALLOWS_TEARING, "Allows Tearing", "", "Indicates if the frame allows tearing") \
		X_(METRIC, RENDER_LATENCY, "Render Latency", "", "Time between frame submission and frame completion") \
		X_(METRIC, DISPLAY_LATENCY, "Display Latency", "", "Time between frame submission and scan out to display") \
		X_(METRIC, INPUT_LATENCY, "Input Latency", "", "Time between input and display (click to photon)") \
		\
		X_(METRIC, GPU_POWER, "GPU Power", "", "Power consumed by the graphics adapter") \
		X_(METRIC, GPU_VOLTAGE, "GPU Voltage", "", "Voltage consumet by the graphics adapter") \
		X_(METRIC, GPU_FREQUENCY, "GPU Frequency", "", "Clock speed of the GPU cores") \
		X_(METRIC, GPU_TEMPERATURE, "GPU Temperature", "", "Temperature of the GPU") \
		X_(METRIC, GPU_FAN_SPEED, "GPU Fan Speed", "", "Rate at which a GPU cooler fan is rotating") \
		X_(METRIC, GPU_UTILIZATION, "GPU Utilization", "", "Amount of GPU processing capacity being used") \
		X_(METRIC, GPU_RENDER_COMPUTE_UTILIZATION, "3D/Compute Utilization", "", "Amount of 3D/Compute processing capacity being used") \
		X_(METRIC, GPU_MEDIA_UTILIZATION, "Media Utilization", "", "Amount of media processing capacity being used") \
		X_(METRIC, VRAM_POWER, "VRAM Power", "", "Power consumed by the VRAM") \
		X_(METRIC, VRAM_VOLTAGE, "VRAM Voltage", "", "Voltage consumet by the VRAM") \
		X_(METRIC, VRAM_FREQUENCY, "VRAM Frequency", "", "Clock speed of the VRAM") \
		X_(METRIC, VRAM_EFFECTIVE_FREQUENCY, "VRAM Effective Frequency", "", "Effective data transfer rate VRAM can sustain") \
		X_(METRIC, VRAM_TEMPERATURE, "VRAM Temperature", "", "Temperature of the VRAM") \
		X_(METRIC, GPU_MEM_USED, "GPU Memory Size Used", "", "Amount of used GPU memory") \
		X_(METRIC, GPU_MEM_UTILIZATION, "GPU Memory Utilization", "", "Percent of GPU memory used") \
		X_(METRIC, GPU_MEM_WRITE_BANDWIDTH, "GPU Memory Write Bandwidth", "", "Maximum GPU memory bandwidth for writing") \
		X_(METRIC, GPU_MEM_READ_BANDWIDTH, "GPU Memory Read Bandwidth", "", "Maximum GPU memory bandwidth for reading") \
		X_(METRIC, GPU_POWER_LIMITED, "GPU Power Limited", "", "GPU frequency is being limited because GPU is exceeding maximum power limits") \
		X_(METRIC, GPU_TEMPERATURE_LIMITED, "GPU Temperature Limited", "", "GPU frequency is being limited because GPU is exceeding maximum temperature limits") \
		X_(METRIC, GPU_CURRENT_LIMITED, "GPU Current Limited", "", "GPU frequency is being limited because GPU is exceeding maximum current limits") \
		X_(METRIC, GPU_VOLTAGE_LIMITED, "GPU Voltage Limited", "", "GPU frequency is being limited because GPU is exceeding maximum voltage limits") \
		X_(METRIC, GPU_UTILIZATION_LIMITED, "GPU Utilization Limited", "", "GPU frequency is being limited due to low GPU utilization") \
		X_(METRIC, VRAM_POWER_LIMITED, "VRAM Power Limited", "", "Memory frequency is being limited because the memory modules are exceeding the maximum power limits") \
		X_(METRIC, VRAM_TEMPERATURE_LIMITED, "VRAM Temperature Limited", "", "Memory frequency is being limited because the memory modules are exceeding the maximum temperature limits") \
		X_(METRIC, VRAM_CURRENT_LIMITED, "VRAM Current Limited", "", "Memory frequency is being limited because the memory modules are exceeding the maximum current limits") \
		X_(METRIC, VRAM_VOLTAGE_LIMITED, "VRAM Voltage", "", "Memory frequency is being limited because the memory modules are exceeding the maximum voltage limits") \
		X_(METRIC, VRAM_UTILIZATION_LIMITED, "VRAM Utilization Limited", "", "Memory frequency is being limited due to low memory traffic") \
		\
		X_(METRIC, CPU_UTILIZATION, "CPU Utilization", "", "Amount of CPU processing capacity being used") \
		X_(METRIC, CPU_POWER, "CPU Power", "", "Power consumed by the CPU") \
		X_(METRIC, CPU_TEMPERATURE, "CPU Temperature", "", "Temperature of the CPU") \
		X_(METRIC, CPU_FREQUENCY, "CPU Frequency", "", "Clock speed of the CPU") \
		X_(METRIC, CPU_CORE_UTILITY, "CPU Core Utility", "", "Amount of CPU processing utility being used per core") \
		\
		X_(METRIC, GPU_SUSTAINED_POWER_LIMIT, "GPU Sustained Power Limit", "", "Sustain power limit of the GPU") \
		X_(METRIC, GPU_MEM_SIZE, "GPU Memory Size", "", "Size of the GPU memory") \
		X_(METRIC, GPU_MEM_MAX_BANDWIDTH, "GPU Memory Max Bandwidth", "", "Maximum total GPU memory bandwidth") \
		X_(METRIC, CPU_POWER_LIMIT, "CPU Power Limit", "", "Power limit of the CPU") \
		X_(METRIC, PROCESS_NAME, "Process Name", "", "Name of the process being targetted") \
		X_(METRIC, GPU_VENDOR, "GPU Vendor", "", "Vendor name of the GPU") \
		X_(METRIC, GPU_NAME, "GPU Name", "", "Device name of the GPU") \
		X_(METRIC, CPU_VENDOR, "CPU Vendor", "", "Vendor name of the CPU") \
		X_(METRIC, CPU_NAME, "CPU Name", "", "Device name of the CPU") \
		\
		X_(METRIC, PRESENT_FLAGS, "Present Flags", "", "Flags used to configure the present operation") \
		X_(METRIC, TIME, "Time", "", "Time elapsed since the start of ETW event tracing") \
		X_(METRIC, TIME_IN_PRESENT_API, "Time in Present API", "", "Time between calling the present function and return") \
		X_(METRIC, TIME_BETWEEN_PRESENTS, "Time Between Presents", "", "Time between this present and the previous one") \
		X_(METRIC, TIME_UNTIL_RENDER_COMPLETE, "Time Until Render Complete", "", "Time between the Present() call and when the GPU work is complete") \
		X_(METRIC, TIME_UNTIL_DISPLAYED, "Time Until Displayed", "", "The time between the Present() call and when the frame was displayed") \
		X_(METRIC, TIME_BETWEEN_DISPLAY_CHANGE, "Time Between Display Change", "", "Time between display scanout of previous frame and this frame") \
		X_(METRIC, TIME_UNTIL_RENDER_START, "Time Until Render Start", "", "The time between the Present() call and when the GPU work started") \
		X_(METRIC, TIME_SINCE_INPUT, "Time Since Input", "", "The time between the Present() call and the earliest keyboard or mouse interaction that contributed to this frame") \
		X_(METRIC, GPU_VIDEO_BUSY_TIME, "GPU Video Busy Time", "", "The time video encode/decode was active separate from the other engines in milliseconds")
#define ENUM_KEY_LIST_METRIC_TYPE(X_) \
		X_(METRIC_TYPE, DYNAMIC, "Dynamic Metric", "", "Metric that changes over time and requires polling using a registered query") \
		X_(METRIC_TYPE, STATIC, "Static Metric", "", "Metric that never changes and can be polled without registering a query") \
		X_(METRIC_TYPE, FRAME_EVENT, "Frame Event Metric", "", "Metric that is not polled, but rather consumed from a queue of frame events") \
		X_(METRIC_TYPE, DYNAMIC_FRAME, "Dynamic and Frame Event Metric", "", "Metric that can either be polled with statisics, or consumed from frame event queue")
#define ENUM_KEY_LIST_DEVICE_VENDOR(X_) \
		X_(DEVICE_VENDOR, INTEL, "Intel", "", "Device vendor Intel") \
		X_(DEVICE_VENDOR, NVIDIA, "NVIDIA", "", "Device vendor NVIDIA") \
		X_(DEVICE_VENDOR, AMD, "AMD", "", "Device vendor AMD") \
		X_(DEVICE_VENDOR, UNKNOWN, "Unknown", "", "Unknown device vendor")
#define ENUM_KEY_LIST_PRESENT_MODE(X_) \
		X_(PRESENT_MODE, HARDWARE_LEGACY_FLIP, "Hardware Legacy Flip", "", "Legacy flip present with direct control of hardware scanout frame buffer (fullscreen exclusive)") \
		X_(PRESENT_MODE, HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER, "Hardware Legacy Copy to Front Buffer", "", "Legacy bitblt model present copying from backbuffer to scanout frame buffer (fullscreen exclusive)") \
		X_(PRESENT_MODE, HARDWARE_INDEPENDENT_FLIP, "Hardware Independent Flip", "", "Application window client area covers display output and its swap chain is being used directly for scanout without DWM intervention") \
		X_(PRESENT_MODE, COMPOSED_FLIP, "Composed Flip", "", "Application front buffer is being composed by DWM to final scanout frame buffer") \
		X_(PRESENT_MODE, HARDWARE_COMPOSED_INDEPENDENT_FLIP, "Hardware Composed Independent Flip", "", "Application is able to directly write into final scanout frame buffer to compose itself without DWM intervention") \
		X_(PRESENT_MODE, COMPOSED_COPY_WITH_GPU_GDI, "Composed Copy with GPU GDI", "", "GDI bitblt composition using the GPU") \
		X_(PRESENT_MODE, COMPOSED_COPY_WITH_CPU_GDI, "Composed Copy with CPU GDI", "", "GDI bitblt composition using the CPU") \
		X_(PRESENT_MODE, UNKNOWN, "Unknown", "", "Unknown present mode")
#define ENUM_KEY_LIST_PSU_TYPE(X_) \
		X_(PSU_TYPE, NONE, "None", "", "No power supply information") \
		X_(PSU_TYPE, PCIE, "PCIE", "", "Power supplied from PCIE bus") \
		X_(PSU_TYPE, 6PIN, "6PIN", "", "Power supplied from 6-pin power connector") \
		X_(PSU_TYPE, 8PIN, "8PIN", "", "Power supplied from 8-pin power connector")
#define ENUM_KEY_LIST_UNIT(X_) \
		X_(UNIT, DIMENSIONLESS, "Dimensionless", "", "Dimensionless numeric metric") \
		X_(UNIT, RATIO, "Ratio", "", "Ratio of one value to another (such as used / total memory)") \
		X_(UNIT, BOOLEAN, "Boolean", "", "Boolean value with 1 indicating present/active and 0 indicating vacant/inactive") \
		X_(UNIT, PERCENT, "Percent", "%", "Proportion or ratio represented as a fraction of 100") \
		X_(UNIT, FPS, "Frames Per Second", "fps", "Rate of application frames being presented per unit time") \
		X_(UNIT, MICROSECONDS, "Microseconds", "us", "Time duration in microseconds") \
		X_(UNIT, MILLISECONDS, "Milliseconds", "ms", "Time duration in milliseconds") \
		X_(UNIT, SECONDS, "Seconds", "s", "Time duration in seconds") \
		X_(UNIT, MINUTES, "Minutes", "m", "Time duration in minutes") \
		X_(UNIT, HOURS, "Hours", "h", "Time duration in hours") \
		X_(UNIT, MILLIWATTS, "Milliwatts", "mW", "Power in milliwatts (millijoules per second)") \
		X_(UNIT, WATTS, "Watts", "W", "Power in watts (Joules per second)") \
		X_(UNIT, KILOWATTS, "Kilowatts", "kW", "Power in kilowatts (kilojoules per second)") \
		X_(UNIT, SYNC_INTERVAL, "Sync Intervals", "vblk", "Value indicating a count/duration of display vsync intervals (also known as vertical blanks)") \
		X_(UNIT, MILLIVOLTS, "Millivolts", "mV", "Electric potential 0.001 V") \
		X_(UNIT, VOLTS, "Volts", "V", "Electric potential") \
		X_(UNIT, HERTZ, "Hertz", "Hz", "Frequency in cycles per second") \
		X_(UNIT, KILOHERTZ, "Kilohertz", "kHz", "Frequency in thousands of cycles per second") \
		X_(UNIT, MEGAHERTZ, "Megahertz", "MHz", "Frequency in millions of cycles per second") \
		X_(UNIT, GIGAHERTZ, "Gigahertz", "GHz", "Frequency in billions of cycles per second") \
		X_(UNIT, CELSIUS, "Degrees Celsius", "C", "Temperature in degrees Celsius") \
		X_(UNIT, RPM, "Revolutions per Minute", "RPM", "Angular speed in revolutions per minute") \
		X_(UNIT, BITS_PER_SECOND, "Bits per Second", "bps", "Bandwidth / data throughput in bits per second") \
		X_(UNIT, KILOBITS_PER_SECOND, "Kilobits per Second", "kbps", "Bandwidth / data throughput in kilobits per second") \
		X_(UNIT, MEGABITS_PER_SECOND, "Megabits per Second", "Mbps", "Bandwidth / data throughput in megabits per second") \
		X_(UNIT, GIGABITS_PER_SECOND, "Gigabits per Second", "Gbps", "Bandwidth / data throughput in gigabits per second") \
		X_(UNIT, BYTES, "Bytes", "B", "Data volume in bytes") \
		X_(UNIT, KILOBYTES, "Kilobytes", "kB", "Data volume in kilobytes") \
		X_(UNIT, MEGABYTES, "Megabytes", "MB", "Data volume in megabytes") \
		X_(UNIT, GIGABYTES, "Gigabytes", "GB", "Data volume in gigabytes") \
		X_(UNIT, QPC, "High-performance timestamp", "qpc", "Timestamp obtained via QueryPerformanceCounter (or compatible)")
#define ENUM_KEY_LIST_STAT(X_) \
		X_(STAT, NONE, "None", "", "Null stat, typically used when querying static or consuming frame events") \
		X_(STAT, AVG, "Average", "avg", "Average or mean of frame samples over the sliding window") \
		X_(STAT, PERCENTILE_99, "99th Percentile", "99%", "Value below which 99% of the observations within the sliding window fall (worst 1% value)") \
		X_(STAT, PERCENTILE_95, "95th Percentile", "95%", "Value below which 95% of the observations within the sliding window fall (worst 5% value)") \
		X_(STAT, PERCENTILE_90, "90th Percentile", "90%", "Value below which 90% of the observations within the sliding window fall (worst 10% value)") \
		X_(STAT, MAX, "Maximum", "max", "Maximum value of frame samples within the sliding window") \
		X_(STAT, MIN, "Minimum", "min", "Minimum value of frame samples within the sliding window") \
		X_(STAT, MID_POINT, "Midpoint", "mpt", "Point sample of the frame data nearest to the middle of the sliding window") \
		X_(STAT, MID_LERP, "Mid Lerp", "mlp", "Linear interpolation between the two points nearest to the middle of the sliding window") \
		X_(STAT, NEWEST_POINT, "Newest Point", "npt", "Value in the most recent frame in the sliding window") \
		X_(STAT, OLDEST_POINT, "Oldest Point", "opt", "Value in the least recent frame in the sliding window") \
		X_(STAT, COUNT, "Count", "cnt", "Count of frames in the sliding window matching a predicate (e.g. counting # of frames for which a field is boolean true)")
#define ENUM_KEY_LIST_DATA_TYPE(X_) \
		X_(DATA_TYPE, DOUBLE, "Double Precision Floating Point Value", "double", "64-bit double precision floating point number in IEEE 754 format") \
		X_(DATA_TYPE, INT32, "32-bit Signed Integer", "int32_t", "32-bit signed integer") \
		X_(DATA_TYPE, UINT32, "32-bit Unsigned Integer", "uint32_t", "32-bit unsigned integer") \
		X_(DATA_TYPE, ENUM, "Enumeration", "int", "Integral value of an enum key, guaranteed to fit within a 32-bit signed integer") \
		X_(DATA_TYPE, STRING, "String", "char[260]", "Textual value, typically for non-numeric data") \
		X_(DATA_TYPE, UINT64, "64-bit Unsigned Integer", "uint64_t", "64-bit unsigned integer") \
		X_(DATA_TYPE, BOOL, "Boolean Value", "bool", "8-bit boolean flag value") \
		X_(DATA_TYPE, VOID, "Void Value", "void", "Value does not exist, is not accessible")
#define ENUM_KEY_LIST_GRAPHICS_RUNTIME(X_) \
		X_(GRAPHICS_RUNTIME, UNKNOWN, "Unknown", "", "Unknown graphics runtime") \
		X_(GRAPHICS_RUNTIME, DXGI, "DXGI", "", "DirectX Graphics Infrastructure runtime") \
		X_(GRAPHICS_RUNTIME, D3D9, "Direct3D 9", "", "Direct3D 9 runtime")
#define ENUM_KEY_LIST_DEVICE_TYPE(X_) \
		X_(DEVICE_TYPE, INDEPENDENT, "Device Independent", "", "This device type is used for special device ID 0 which is reserved for metrics independent of any specific hardware device (e.g. FPS metrics)") \
		X_(DEVICE_TYPE, GRAPHICS_ADAPTER, "Graphics Adapter", "", "Graphics adapter or GPU device")
#define ENUM_KEY_LIST_METRIC_AVAILABILITY(X_) \
		X_(METRIC_AVAILABILITY, AVAILABLE, "Available", "", "Metric is available on the indicated device") \
		X_(METRIC_AVAILABILITY, UNAVAILABLE, "Unavailable", "", "Metric is unavailable on the indicated device")
// master list of enums as an enum giving each enum a unique id
#define ENUM_KEY_LIST_ENUM(X_) \
		X_(ENUM, STATUS, "Statuses", "", "List of all status/error codes returned by API functions") \
		X_(ENUM, METRIC, "Metrics", "", "List of all metrics that are pollable via the API") \
		X_(ENUM, METRIC_TYPE, "Metric Types", "", "List of all metric types (dynamic polled, static, etc.)") \
		X_(ENUM, DEVICE_VENDOR, "Vendors", "", "List of all known hardware vendors (IHVs) for GPUs and other hardware") \
		X_(ENUM, PRESENT_MODE, "Present Modes", "", "List of all known modes of frame presentation") \
		X_(ENUM, PSU_TYPE, "PSU Types", "", "Type of power supply used by GPUs") \
		X_(ENUM, UNIT, "Units", "", "List of all units of measure used for metrics") \
		X_(ENUM, STAT, "Statistics", "", "List of all statistical variations of the data (average, 99th percentile, etc.)") \
		X_(ENUM, DATA_TYPE, "Data Types", "", "List of all C++ language data types used for metrics") \
		X_(ENUM, GRAPHICS_RUNTIME, "Graphics Runtime", "", "Graphics runtime subsystem used to make the present call") \
		X_(ENUM, DEVICE_TYPE, "Device Type", "", "Type of device in the list of devices associated with metrics") \
		X_(ENUM, METRIC_AVAILABILITY, "Metric Availability", "", "Availability status of a metric with respect to a given device")
// list of metrics
#define METRIC_LIST(X_) \
		X_(PM_METRIC_SWAP_CHAIN, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_UINT64, PM_DATA_TYPE_UINT64, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		X_(PM_METRIC_DISPLAYED_FPS, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_FPS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_PRESENTED_FPS, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_FPS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_FRAME_TIME, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_GPU_BUSY_TIME, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_DISPLAY_BUSY_TIME, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_CPU_BUSY_TIME, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_CPU_WAIT_TIME, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_DROPPED_FRAMES, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_NUM_PRESENTS, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_INT32, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		X_(PM_METRIC_SYNC_INTERVAL, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_SYNC_INTERVAL, PM_DATA_TYPE_UINT32, PM_DATA_TYPE_UINT32, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		X_(PM_METRIC_PRESENT_MODE, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_ENUM, PM_DATA_TYPE_ENUM, PM_ENUM_PRESENT_MODE, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		X_(PM_METRIC_PRESENT_RUNTIME, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_ENUM, PM_DATA_TYPE_ENUM, PM_ENUM_GRAPHICS_RUNTIME, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		X_(PM_METRIC_PRESENT_QPC, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_QPC, PM_DATA_TYPE_UINT64, PM_DATA_TYPE_UINT64, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		X_(PM_METRIC_ALLOWS_TEARING, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_RENDER_LATENCY, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_DISPLAY_LATENCY, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_INPUT_LATENCY, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		\
		X_(PM_METRIC_GPU_POWER, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_WATTS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_VOLTAGE, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_VOLTS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_FREQUENCY, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_MEGAHERTZ, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_TEMPERATURE, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_CELSIUS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_FAN_SPEED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_RPM, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_UTILIZATION, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_PERCENT, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_PERCENT, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_MEDIA_UTILIZATION, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_PERCENT, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_POWER, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_WATTS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_VOLTAGE, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_VOLTS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_FREQUENCY, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_MEGAHERTZ, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_EFFECTIVE_FREQUENCY, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_MEGAHERTZ, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_TEMPERATURE, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_CELSIUS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_MEM_USED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BYTES, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_UINT64, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_MEM_UTILIZATION, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_BYTES, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_MEM_WRITE_BANDWIDTH, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BITS_PER_SECOND, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_MEM_READ_BANDWIDTH, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BITS_PER_SECOND, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_POWER_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_TEMPERATURE_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_CURRENT_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_VOLTAGE_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_GPU_UTILIZATION_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_POWER_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_TEMPERATURE_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_CURRENT_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_VOLTAGE_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_VRAM_UTILIZATION_LIMITED, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_BOOLEAN, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_BOOL, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		\
		X_(PM_METRIC_CPU_UTILIZATION, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_PERCENT, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		X_(PM_METRIC_CPU_POWER, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_WATTS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_CPU_TEMPERATURE, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_CELSIUS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_CPU_FREQUENCY, PM_METRIC_TYPE_DYNAMIC_FRAME, PM_UNIT_MEGAHERTZ, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, FULL_STATS) \
		X_(PM_METRIC_CPU_CORE_UTILITY, PM_METRIC_TYPE_DYNAMIC, PM_UNIT_PERCENT, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, FULL_STATS) \
		\
		X_(PM_METRIC_GPU_SUSTAINED_POWER_LIMIT, PM_METRIC_TYPE_STATIC, PM_UNIT_WATTS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_STAT_MID_POINT) \
		X_(PM_METRIC_GPU_MEM_SIZE, PM_METRIC_TYPE_STATIC, PM_UNIT_BYTES, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_UINT64, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_STAT_MID_POINT) \
		X_(PM_METRIC_GPU_MEM_MAX_BANDWIDTH, PM_METRIC_TYPE_STATIC, PM_UNIT_BITS_PER_SECOND, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_UINT64, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_STAT_MID_POINT) \
		X_(PM_METRIC_CPU_POWER_LIMIT, PM_METRIC_TYPE_STATIC, PM_UNIT_WATTS, PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_STAT_MID_POINT) \
		X_(PM_METRIC_PROCESS_NAME, PM_METRIC_TYPE_STATIC, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_STRING, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		X_(PM_METRIC_CPU_VENDOR, PM_METRIC_TYPE_STATIC, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_ENUM, PM_DATA_TYPE_VOID, PM_ENUM_DEVICE_VENDOR, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		X_(PM_METRIC_GPU_NAME, PM_METRIC_TYPE_STATIC, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_STRING, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_STAT_MID_POINT) \
		X_(PM_METRIC_GPU_VENDOR, PM_METRIC_TYPE_STATIC, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_ENUM, PM_DATA_TYPE_VOID, PM_ENUM_DEVICE_VENDOR, PM_DEVICE_TYPE_GRAPHICS_ADAPTER, PM_STAT_MID_POINT) \
		X_(PM_METRIC_CPU_NAME, PM_METRIC_TYPE_STATIC, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_STRING, PM_DATA_TYPE_VOID, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_MID_POINT) \
		\
		X_(PM_METRIC_PRESENT_FLAGS, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_DIMENSIONLESS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_UINT32, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_TIME, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_SECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_TIME_IN_PRESENT_API, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_TIME_BETWEEN_PRESENTS, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_TIME_UNTIL_RENDER_COMPLETE, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_TIME_UNTIL_DISPLAYED, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_TIME_BETWEEN_DISPLAY_CHANGE, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_TIME_UNTIL_RENDER_START, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_TIME_SINCE_INPUT, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) \
		X_(PM_METRIC_GPU_VIDEO_BUSY_TIME, PM_METRIC_TYPE_FRAME_EVENT, PM_UNIT_MILLISECONDS, PM_DATA_TYPE_VOID, PM_DATA_TYPE_DOUBLE, 0, PM_DEVICE_TYPE_INDEPENDENT, PM_STAT_NONE) 

// metric preferred unit overrides
#define PREFERRED_UNIT_LIST(X_) \
		X_(PM_METRIC_GPU_MEM_USED, PM_UNIT_GIGABYTES) \
		X_(PM_METRIC_GPU_MEM_WRITE_BANDWIDTH, PM_UNIT_GIGABITS_PER_SECOND) \
		X_(PM_METRIC_GPU_MEM_READ_BANDWIDTH, PM_UNIT_GIGABITS_PER_SECOND) \
		X_(PM_METRIC_GPU_MEM_SIZE, PM_UNIT_GIGABYTES) \
		X_(PM_METRIC_GPU_MEM_MAX_BANDWIDTH, PM_UNIT_GIGABITS_PER_SECOND)

// defining unit groups and conversions X_(id, baseId, scale) [scale: 1 of these = scale of base]
#define UNIT_LIST(X_) \
	X_(PM_UNIT_RATIO, PM_UNIT_RATIO, 1.)  \
	X_(PM_UNIT_BOOLEAN, PM_UNIT_RATIO, 1.) \
	X_(PM_UNIT_PERCENT, PM_UNIT_RATIO, 100.) \
	X_(PM_UNIT_MICROSECONDS, PM_UNIT_SECONDS, 0.000'001) \
	X_(PM_UNIT_MILLISECONDS, PM_UNIT_SECONDS, 0.001) \
	X_(PM_UNIT_SECONDS, PM_UNIT_SECONDS, 1.) \
	X_(PM_UNIT_MINUTES, PM_UNIT_SECONDS, 60.) \
	X_(PM_UNIT_HOURS, PM_UNIT_SECONDS, 3600.) \
	X_(PM_UNIT_MILLIWATTS, PM_UNIT_WATTS, 3600.) \
	X_(PM_UNIT_WATTS, PM_UNIT_WATTS, 3600.) \
	X_(PM_UNIT_KILOWATTS, PM_UNIT_WATTS, 3600.) \
	X_(PM_UNIT_MILLIVOLTS, PM_UNIT_VOLTS, 0.001) \
	X_(PM_UNIT_VOLTS, PM_UNIT_VOLTS, 1.) \
	X_(PM_UNIT_HERTZ, PM_UNIT_HERTZ, 1.) \
	X_(PM_UNIT_KILOHERTZ, PM_UNIT_HERTZ, 1'000.) \
	X_(PM_UNIT_MEGAHERTZ, PM_UNIT_HERTZ, 1'000'000.) \
	X_(PM_UNIT_GIGAHERTZ, PM_UNIT_HERTZ, 1'000'000'000.) \
	X_(PM_UNIT_BITS_PER_SECOND, PM_UNIT_BITS_PER_SECOND, 1.) \
	X_(PM_UNIT_KILOBITS_PER_SECOND, PM_UNIT_BITS_PER_SECOND, 1'024.) \
	X_(PM_UNIT_MEGABITS_PER_SECOND, PM_UNIT_BITS_PER_SECOND, 1'048'576.) \
	X_(PM_UNIT_GIGABITS_PER_SECOND, PM_UNIT_BITS_PER_SECOND, 1'073'741'824.) \
	X_(PM_UNIT_BYTES, PM_UNIT_BYTES, 1.) \
	X_(PM_UNIT_KILOBYTES, PM_UNIT_BYTES, 1'024.) \
	X_(PM_UNIT_MEGABYTES, PM_UNIT_BYTES, 1'048'576.) \
	X_(PM_UNIT_GIGABYTES, PM_UNIT_BYTES, 1'073'741'824.)


// static mapping of datatype enum to static type
	template<PM_DATA_TYPE T>
	struct EnumToStaticType;
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_DOUBLE> { using type = double; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_INT32> { using type = int32_t; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_UINT32> { using type = uint32_t; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_ENUM> { using type = int; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_STRING> { using type = char[260]; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_UINT64> { using type = uint64_t; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_BOOL> { using type = bool; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_VOID> { using type = bool; };
	// TODO: find better place for this template glue
	template<PM_DATA_TYPE T>
	using EnumToStaticType_t = typename EnumToStaticType<T>::type;
	template<PM_DATA_TYPE T>
	constexpr size_t EnumToStaticType_sz = sizeof(EnumToStaticType_t<T>);
}
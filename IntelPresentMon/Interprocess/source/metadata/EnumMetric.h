#pragma once
#include "../../../PresentMonAPI2/source/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_METRIC(X_) \
		X_(METRIC, SWAP_CHAIN_ADDRESS, "Swap Chain Address", "", "Address of the swap chain used to present, useful as a unique identifier") \
		X_(METRIC, DISPLAYED_FPS, "Displayed FPS", "", "Rate of frame change measurable at display") \
		X_(METRIC, PRESENTED_FPS, "Presented FPS", "", "Rate of application calls to a Present() function") \
		X_(METRIC, FRAME_TIME, "Frame Time", "", "The total amount of time in between frames on the CPU") \
		X_(METRIC, GPU_BUSY, "GPU Busy", "", "How long the GPU spent working on this frame") \
		X_(METRIC, GPU_WAIT, "GPU Wait", "", "How long the GPU spent waiting while working on this frame") \
		X_(METRIC, DISPLAYED_TIME, "Displayed Time", "", "How long this frame was displayed on screen") \
		X_(METRIC, DROPPED_FRAMES, "Dropped Frames", "", "Indicates if the frame was not displayed") \
		X_(METRIC, SYNC_INTERVAL, "Sync Interval", "", "The application's requested interval between presents measured in vertical sync/vblank events") \
		X_(METRIC, PRESENT_MODE, "Present Mode", "", "Method used to present the frame") \
		X_(METRIC, PRESENT_RUNTIME, "Present Runtime", "", "The graphics runtime used for the present operation (DXGI, D3D9, etc.)") \
		X_(METRIC, CPU_FRAME_QPC, "CPU Frame QPC", "", "The QueryPerformanceCounter timestamp when the CPU started working on the frame") \
		X_(METRIC, ALLOWS_TEARING, "Allows Tearing", "", "Indicates if the frame allows tearing") \
		X_(METRIC, GPU_LATENCY, "GPU Latency", "", "How long it took until GPU work for this frame started") \
		X_(METRIC, DISPLAY_LATENCY, "Display Latency", "", "Time between frame submission and scan out to display") \
		X_(METRIC, CLICK_TO_PHOTON_LATENCY, "Click to Photon Latency", "", "Time between input and display") \
		X_(METRIC, INPUT_LATENCY, "Input to Frame Start Latency", "", "Time between input and display") \
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
		X_(METRIC, APPLICATION, "Application", "", "Name of the executable of the process being targetted") \
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
		X_(METRIC, GPU_VIDEO_BUSY_TIME, "GPU Video Busy Time", "", "The time video encode/decode was active separate from the other engines in milliseconds") \
		X_(METRIC, CPU_WAIT, "CPU Wait", "", "How long the CPU spent waiting before it could start generating the frame in milliseconds") \
		X_(METRIC, GPU_TIME, "GPU Time", "", "Total amount of time between when GPU started frame and when it finished in milliseconds. The GPU may not have been fully busy during this time")
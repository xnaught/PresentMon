// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include <windows.h>
#include <string>
#include "Console.h"
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <TlHelp32.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <format>
#include <chrono>
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include "../PresentMonAPI2/source/Internal.h"
#include "CliOptions.h"
#include "../PresentMonAPIWrapper/source/PresentMonAPIWrapper.h"

using namespace std::chrono;
using namespace std::chrono_literals;

HANDLE gCloseEvent;
bool gQuit = false;
uint32_t gCurrentPid = 0;
std::string g_process_name;
std::string g_etl_file_name;
int32_t g_metrics_offset = 0;
bool g_record_frames = false;
const double kWindowSize = 2000.0;
const uint32_t kSleepTime = 4;
const uint32_t kNumFramesInBuf = 1000;
std::vector<char> g_cpu_name;

void PrintError(PM_STATUS status) {
  std::string s{};
#define PROCESS_VAL(p) \
  case (p):            \
    s = #p;            \
    break;

  switch (status) {
    PROCESS_VAL(PM_STATUS::PM_STATUS_SUCCESS);
    PROCESS_VAL(PM_STATUS::PM_STATUS_NO_DATA);
    PROCESS_VAL(PM_STATUS::PM_STATUS_DATA_LOSS);
    PROCESS_VAL(PM_STATUS::PM_STATUS_SERVICE_ERROR);
    PROCESS_VAL(PM_STATUS::PM_STATUS_INVALID_PID);
    PROCESS_VAL(PM_STATUS::PM_STATUS_INVALID_ETL_FILE);
    PROCESS_VAL(PM_STATUS::PM_STATUS_FAILURE);
  }
#undef PROCESS_VAL
  if (s.length() > 0) {
    ConsolePrintLn(s.c_str());
    CommitConsole();
  }
}

std::string TranslatePresentMode(PM_PRESENT_MODE present_mode) {
  switch (present_mode) {
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP:
      return "Hardware: Legacy Flip";

    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER:
        return "Hardware: Legacy Copy to front buffer";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP:
        return "Hardware: Independent Flip";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_FLIP:
        return "Composed: Flip";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_COMPOSED_INDEPENDENT_FLIP:
        return "Hardware Composed: Independent Flip";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_COPY_WITH_GPU_GDI:
        return "Composed: Copy with GPU GDI";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_COPY_WITH_CPU_GDI:
        return "Composed: Copy with CPU GDI";
    default:
        return("Present Mode: Unknown");
  }
}

std::string TranslateDeviceVendor(PM_DEVICE_VENDOR deviceVendor) {
    switch (deviceVendor) {
    case PM_DEVICE_VENDOR_INTEL:
        return "PM_DEVICE_VENDOR_INTEL";
    case PM_DEVICE_VENDOR_NVIDIA:
        return "PM_DEVICE_VENDOR_NVIDIA";
    case PM_DEVICE_VENDOR_AMD:
        return "PM_DEVICE_VENDOR_AMD";
    case PM_DEVICE_VENDOR_UNKNOWN:
        return "PM_DEVICE_VENDOR_UNKNOWN";
    default:
        return "PM_DEVICE_VENDOR_UNKNOWN";
    }
}

std::string TranslateGraphicsRuntime(PM_GRAPHICS_RUNTIME graphicsRuntime) {
    switch (graphicsRuntime) {
    case PM_GRAPHICS_RUNTIME_UNKNOWN:
        return "UNKNOWN";
    case PM_GRAPHICS_RUNTIME_DXGI:
        return "DXGI";
    case PM_GRAPHICS_RUNTIME_D3D9:
        return "D3D9";
    default:
        return "UNKNOWN";
    }
}

void OutputString(const char* output) {
  try {
    std::cout << output;
  } catch (const std::exception& e) {
    std::cout << "a standard exception was caught, with message '" << e.what()
              << "'" << std::endl;
    return;
  }
}

void PrintMetric(char const* format, double metric_data, bool valid) {
  if (valid) {
    ConsolePrintLn(format, metric_data);
  }
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
  switch (fdwCtrlType) {
    case CTRL_C_EVENT:
      SetEvent(gCloseEvent);
      gQuit = true;
      return TRUE;
    default:
      return FALSE;
  }
}

bool caseInsensitiveCompare(std::string str1, std::string str2) {
    std::for_each(str1.begin(), str1.end(), [](char& c)
    {
        c = std::tolower(static_cast<unsigned char>(c));
    });
    std::for_each(str2.begin(), str2.end(), [](char& c)
    {
        c = std::tolower(static_cast<unsigned char>(c));
    });
    if (str1.compare(str2) == 0)
        return true;
    return false;
}

void GetProcessInformation(std::optional<std::string>& processName, std::optional<unsigned int>& processId) {
    if (!processName.has_value() && !processId.has_value()) {
        return;
    }

    HANDLE processes_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processes_snapshot == INVALID_HANDLE_VALUE) {
        processId = std::nullopt;
        processName = std::nullopt;
        return;
    }

    PROCESSENTRY32 process_info;
    process_info.dwSize = sizeof(process_info);

    if (!Process32First(processes_snapshot, &process_info)) {
        // Unable to retrieve the first process
        CloseHandle(processes_snapshot);
        processId = std::nullopt;
        processName = std::nullopt;
        return;
    }

    do {
        if ((processName.has_value() && caseInsensitiveCompare(process_info.szExeFile, processName.value())) ||
            (processId.has_value() && process_info.th32ProcessID == processId.value())) {
            CloseHandle(processes_snapshot);
            processId = process_info.th32ProcessID;
            processName = process_info.szExeFile;
            return;
        }
    } while (Process32Next(processes_snapshot, &process_info));

    CloseHandle(processes_snapshot);

    // If no match found, reset optional values
    processId = std::nullopt;
    processName = std::nullopt;
}

#ifdef ENABLE_ETL
void ProcessEtl() {
  bool valid_selection = false;
  PM_STATUS status;

  while (valid_selection == false) {
    OutputString("Enter ETL filename:");
    if (GetUserInput(g_etl_file_name) == true) {
      if (g_etl_file_name.length() != 0) {
        status = pmStartStreamEtl(g_etl_file_name.c_str());
        if (status == PM_STATUS::PM_STATUS_SUCCESS) {
          break;
        }
        OutputString("Invalid ETL file.\n");
      } else {
        OutputString("No ETL file specified, exiting.\n");
        return;
      }
    }
  }
  RecordFrames(true);
}
#endif

void PollMetrics(const std::unique_ptr<pmapi::Session>& pSession, pmapi::ProcessTracker& processTracker, double metricsOffset)
{
    PM_QUERY_ELEMENT elements[]{
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_DURATION, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_DURATION, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_DURATION, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_DURATION, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_DURATION, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_DURATION, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_DURATION, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FRAME_PACING_STALL, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FRAME_PACING_STALL, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FRAME_PACING_STALL, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FRAME_PACING_STALL, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FRAME_PACING_STALL, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FRAME_PACING_STALL, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FRAME_PACING_STALL, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_DURATION, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_DURATION, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_DURATION, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_DURATION, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_DURATION, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_DURATION, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_DURATION, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAY_DURATION, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_FREQUENCY, .stat = PM_STAT_AVG, .deviceId = 1, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_TEMPERATURE, .stat = PM_STAT_AVG, .deviceId = 1, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_VOLTAGE, .stat = PM_STAT_AVG, .deviceId = 1, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_UTILIZATION, .stat = PM_STAT_AVG, .deviceId = 1, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_POWER, .stat = PM_STAT_AVG, .deviceId = 1, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_FAN_SPEED, .stat = PM_STAT_AVG, .deviceId = 1, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_FAN_SPEED, .stat = PM_STAT_AVG, .deviceId = 1, .arrayIndex = 1},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_VRAM_FREQUENCY, .stat = PM_STAT_AVG, .deviceId = 1, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_UTILIZATION, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FREQUENCY, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENT_MODE, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENT_RUNTIME, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_FRAME_QPC, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_SYNC_INTERVAL, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_NUM_FRAMES, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DROPPED_FRAMES, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_ALLOWS_TEARING, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
    };
    auto dynamicQuery1 = pSession->RegisterDyanamicQuery(elements, 2000., metricsOffset);

    /*
    PM_QUERY_ELEMENT staticQueryElement{ .metric = PM_METRIC_APPLICATION, .deviceId = 0, .arrayIndex = 0 };
    auto processName = std::make_unique<uint8_t[]>(260);
    pmPollStaticQuery(g_hSession, &staticQueryElement, processId, processName.get());
    staticQueryElement = { .metric = PM_METRIC_CPU_VENDOR, .deviceId = 0, .arrayIndex = 0 };
    auto cpuVendor = std::make_unique<uint8_t[]>(4);
    pmPollStaticQuery(g_hSession, &staticQueryElement, processId, cpuVendor.get());
    staticQueryElement = { .metric = PM_METRIC_CPU_NAME, .deviceId = 0, .arrayIndex = 0 };
    auto cpuName = std::make_unique<uint8_t[]>(260);
    pmPollStaticQuery(g_hSession, &staticQueryElement, processId, cpuName.get());
    staticQueryElement = { .metric = PM_METRIC_GPU_VENDOR, .deviceId = 0, .arrayIndex = 0 };
    auto gpuVendor = std::make_unique<uint8_t[]>(4);
    pmPollStaticQuery(g_hSession, &staticQueryElement, processId, gpuVendor.get());
    staticQueryElement = { .metric = PM_METRIC_GPU_NAME, .deviceId = 0, .arrayIndex = 0 };
    auto gpuName = std::make_unique<uint8_t[]>(260);
    pmPollStaticQuery(g_hSession, &staticQueryElement, processId, gpuName.get());
    staticQueryElement = { .metric = PM_METRIC_GPU_MEM_MAX_BANDWIDTH, .deviceId = 0, .arrayIndex = 0 };
    auto gpuMemMaxBw = std::make_unique<uint8_t[]>(8);
    pmPollStaticQuery(g_hSession, &staticQueryElement, processId, gpuMemMaxBw.get());
    staticQueryElement = { .metric = PM_METRIC_GPU_MEM_SIZE, .deviceId = 0, .arrayIndex = 0 };
    auto gpuMemSize = std::make_unique<uint8_t[]>(8);
    pmPollStaticQuery(g_hSession, &staticQueryElement, processId, gpuMemSize.get());
    */

    auto blobs1 = dynamicQuery1.MakeBlobContainer(1u);

    for (;;)
    {
        dynamicQuery1.Poll(processTracker, blobs1);
        
        /*
        ConsolePrintLn("Static Process Name = %s", reinterpret_cast<char*>(processName.get()));
        PrintDeviceVendor("Static CPU Vendor = ", reinterpret_cast<PM_DEVICE_VENDOR&>(cpuVendor[0]));
        ConsolePrintLn("Static CPU Name = %s", reinterpret_cast<char*>(cpuName.get()));
        PrintDeviceVendor("Static GPU Vendor = ", reinterpret_cast<PM_DEVICE_VENDOR&>(gpuVendor[0]));
        ConsolePrintLn("Static GPU Name = %s", reinterpret_cast<char*>(gpuName.get()));
        PrintMetric("Static GPU Memory Max Bandwidth = %f", reinterpret_cast<double&>(gpuMemMaxBw[0]), true);
        ConsolePrintLn("Static GPU Memory Size = = %lld", reinterpret_cast<uint64_t&>(gpuMemSize[0]));
        */
        for (auto pBlob : blobs1) {
            PrintMetric("Presented FPS Average = %f", *reinterpret_cast<double*>(&pBlob[elements[0].dataOffset]), true);
            PrintMetric("Presented FPS 90% = %f", *reinterpret_cast<double*>(&pBlob[elements[1].dataOffset]), true);
            PrintMetric("Presented FPS 95% = %f", *reinterpret_cast<double*>(&pBlob[elements[2].dataOffset]), true);
            PrintMetric("Presented FPS 99% = %f", *reinterpret_cast<double*>(&pBlob[elements[3].dataOffset]), true);
            PrintMetric("Presented FPS Max = %f", *reinterpret_cast<double*>(&pBlob[elements[4].dataOffset]), true);
            PrintMetric("Presented FPS Min = %f", *reinterpret_cast<double*>(&pBlob[elements[5].dataOffset]), true);
            PrintMetric("Presented FPS Raw = %f", *reinterpret_cast<double*>(&pBlob[elements[6].dataOffset]), true);

            PrintMetric("Frame Time Average = %f", *reinterpret_cast<double*>(&pBlob[elements[7].dataOffset]), true);
            PrintMetric("Frame Time 90% = %f", *reinterpret_cast<double*>(&pBlob[elements[8].dataOffset]), true);
            PrintMetric("Frame Time 95% = %f", *reinterpret_cast<double*>(&pBlob[elements[9].dataOffset]), true);
            PrintMetric("Frame Time 99% = %f", *reinterpret_cast<double*>(&pBlob[elements[10].dataOffset]), true);
            PrintMetric("Frame Time Max = %f", *reinterpret_cast<double*>(&pBlob[elements[11].dataOffset]), true);
            PrintMetric("Frame Time Min = %f", *reinterpret_cast<double*>(&pBlob[elements[12].dataOffset]), true);
            PrintMetric("Frame Time Raw = %f", *reinterpret_cast<double*>(&pBlob[elements[13].dataOffset]), true);

            PrintMetric("Displayed FPS Average = %f", *reinterpret_cast<double*>(&pBlob[elements[14].dataOffset]), true);
            PrintMetric("Displayed FPS 90% = %f", *reinterpret_cast<double*>(&pBlob[elements[15].dataOffset]), true);
            PrintMetric("Displayed FPS 95% = %f", *reinterpret_cast<double*>(&pBlob[elements[16].dataOffset]), true);
            PrintMetric("Displayed FPS 99% = %f", *reinterpret_cast<double*>(&pBlob[elements[17].dataOffset]), true);
            PrintMetric("Displayed FPS Max = %f", *reinterpret_cast<double*>(&pBlob[elements[18].dataOffset]), true);
            PrintMetric("Displayed FPS Min = %f", *reinterpret_cast<double*>(&pBlob[elements[19].dataOffset]), true);
            PrintMetric("Displayed FPS Raw = %f", *reinterpret_cast<double*>(&pBlob[elements[20].dataOffset]), true);

            PrintMetric("CPU Wait Time Average = %f", *reinterpret_cast<double*>(&pBlob[elements[21].dataOffset]), true);
            PrintMetric("CPU Wait Time 90% = %f", *reinterpret_cast<double*>(&pBlob[elements[22].dataOffset]), true);
            PrintMetric("CPU Wait Time 95% = %f", *reinterpret_cast<double*>(&pBlob[elements[23].dataOffset]), true);
            PrintMetric("CPU Wait Time 99% = %f", *reinterpret_cast<double*>(&pBlob[elements[24].dataOffset]), true);
            PrintMetric("CPU Wait Time Max = %f", *reinterpret_cast<double*>(&pBlob[elements[25].dataOffset]), true);
            PrintMetric("CPU Wait Time Min = %f", *reinterpret_cast<double*>(&pBlob[elements[26].dataOffset]), true);
            PrintMetric("CPU Wait Time Raw = %f", *reinterpret_cast<double*>(&pBlob[elements[27].dataOffset]), true);

            PrintMetric("CPU Busy Time Average = %f", *reinterpret_cast<double*>(&pBlob[elements[28].dataOffset]), true);
            PrintMetric("CPU Busy Time 90% = %f", *reinterpret_cast<double*>(&pBlob[elements[29].dataOffset]), true);
            PrintMetric("CPU Busy Time 95% = %f", *reinterpret_cast<double*>(&pBlob[elements[30].dataOffset]), true);
            PrintMetric("CPU Busy Time 99% = %f", *reinterpret_cast<double*>(&pBlob[elements[31].dataOffset]), true);
            PrintMetric("CPU Busy Time Max = %f", *reinterpret_cast<double*>(&pBlob[elements[32].dataOffset]), true);
            PrintMetric("CPU Busy Time Min = %f", *reinterpret_cast<double*>(&pBlob[elements[33].dataOffset]), true);
            PrintMetric("CPU Busy Time Raw = %f", *reinterpret_cast<double*>(&pBlob[elements[34].dataOffset]), true);

            PrintMetric("GPU Busy Time Average = %f", *reinterpret_cast<double*>(&pBlob[elements[35].dataOffset]), true);
            PrintMetric("GPU Busy Time 90% = %f", *reinterpret_cast<double*>(&pBlob[elements[36].dataOffset]), true);
            PrintMetric("GPU Busy Time 95% = %f", *reinterpret_cast<double*>(&pBlob[elements[37].dataOffset]), true);
            PrintMetric("GPU Busy Time 99% = %f", *reinterpret_cast<double*>(&pBlob[elements[38].dataOffset]), true);
            PrintMetric("GPU Busy Time Max = %f", *reinterpret_cast<double*>(&pBlob[elements[39].dataOffset]), true);
            PrintMetric("GPU Busy Time Min = %f", *reinterpret_cast<double*>(&pBlob[elements[40].dataOffset]), true);
            PrintMetric("GPU Busy Time Raw = %f", *reinterpret_cast<double*>(&pBlob[elements[41].dataOffset]), true);

            PrintMetric("Display Busy Time Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[42].dataOffset]), true);

            PrintMetric("GPU Frequency Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[43].dataOffset]), true);
            PrintMetric("GPU Temperature Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[44].dataOffset]), true);
            PrintMetric("GPU Voltage Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[45].dataOffset]), true);
            PrintMetric("GPU Utilization Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[46].dataOffset]), true);
            PrintMetric("GPU Power Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[47].dataOffset]), true);

            PrintMetric("GPU Fan Speed [0] Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[48].dataOffset]), true);
            PrintMetric("GPU Fan Speed [1] Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[49].dataOffset]), true);

            PrintMetric("GPU Mem Frequency Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[50].dataOffset]), true);
            PrintMetric("CPU Utilization Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[51].dataOffset]), true);
            PrintMetric("CPU Frequency Avg = %f", *reinterpret_cast<double*>(&pBlob[elements[52].dataOffset]), true);
            ConsolePrintLn("Present Mode = %i", *reinterpret_cast<double*>(&pBlob[elements[53].dataOffset]));
            ConsolePrintLn("Present Runtime = %i", *reinterpret_cast<double*>(&pBlob[elements[54].dataOffset]));
            ConsolePrintLn("Present QPC =  %lld", *reinterpret_cast<double*>(&pBlob[elements[55].dataOffset]));
            ConsolePrintLn("Present Sync Interval = %i", *reinterpret_cast<double*>(&pBlob[elements[56].dataOffset]));
            ConsolePrintLn("Num Presents =  %i", *reinterpret_cast<double*>(&pBlob[elements[57].dataOffset]));

            PrintMetric("Dropped Frames Average = %f", *reinterpret_cast<double*>(&pBlob[elements[58].dataOffset]), true);
            PrintMetric("Allows Tearing Average = %f", *reinterpret_cast<double*>(&pBlob[elements[59].dataOffset]), true);

            CommitConsole();
        }

        if (gQuit == true) {
            break;
        }

        Sleep(kSleepTime);
    }

    return;
}

int ViewAvailableSystemMetrics(std::string controlPipe, std::string introspectionNsm)
{
    std::unique_ptr<pmapi::Session> pSession;
    pmapi::ProcessTracker processTracker;
    try {
        if (controlPipe.empty() == false) {
            pSession = std::make_unique<pmapi::Session>(controlPipe, introspectionNsm);
        }
        else {
            pSession = std::make_unique<pmapi::Session>();
        }
    }
    catch (const std::bad_array_new_length& e) {
        std::cout
            << "Creating a new PresentMon session caused bad array new length exception, with message '"
            << e.what() << "'" << std::endl;
        return -1;
    }
    catch (const std::runtime_error& e) {
        std::cout
            << "Creating a new PresentMon session caused std::runtime exception '"
            << e.what() << "'" << std::endl;
        return -1;
    }

    // Example of how to use introspection to examine ALL metrics and determine
    // their availablity
    auto pIntrospectionRoot = pSession->GetIntrospectionRoot();
    auto metricEnums = pIntrospectionRoot->FindEnum(PM_ENUM_METRIC);

    // Loop through ALL PresentMon metrics
    for (auto metric : pIntrospectionRoot->GetMetrics())
    {
        std::string metricSymbol;
        // Look through PM_ENUM_METRIC enums to gather the metric symbol
        for (auto key : metricEnums.GetKeys())
        {
            if (key.GetId() == metric.GetId())
            {
                metricSymbol = key.GetSymbol();
                break;
            }
        }

        // Go through the device metric info to determine the metric's availability
        auto metricInfo = metric.GetDeviceMetricInfo();
        for (auto mi : metricInfo)
        {
            auto device = mi.GetDevice();
            std::cout << std::format("Metric Id: {}, Metric Symbol: {}, Vendor Name: {}, Vendor Device Id: {}, Is Available: {}",
                (int)metric.GetId(), metricSymbol, device.GetName(), device.GetId(), mi.IsAvailable());
            std::cout << std::endl;
        }
    }

    return 0;
}

int PollMetricsThread(std::string controlPipe, std::string introspectionNsm, unsigned int processId)
{
    std::unique_ptr<pmapi::Session> pSession;
    pmapi::ProcessTracker processTracker;
    try {
        if (controlPipe.empty() == false) {
            pSession = std::make_unique<pmapi::Session>(controlPipe, introspectionNsm);
        }
        else {
            pSession = std::make_unique<pmapi::Session>();
        }
    }
    catch (const std::bad_array_new_length& e) {
        std::cout
            << "Creating a new PresentMon session caused bad array new length exception, with message '"
            << e.what() << "'" << std::endl;
        return -1;
    }
    catch (const std::runtime_error& e) {
        std::cout
            << "Creating a new PresentMon session caused std::runtime exception '"
            << e.what() << "'" << std::endl;
        return -1;
    }

    return 0;
}

void WriteToCSV(std::ofstream& csvFile, const std::string& processName, const unsigned int& processId,
    PM_QUERY_ELEMENT(&queryElements)[16], pmapi::BlobContainer& blobs)
{
    
    for (auto pBlob : blobs) {
        //const auto appName = *reinterpret_cast<const std::string*>(&pBlob[queryElements[0].dataOffset]);
        const auto swapChain = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[0].dataOffset]);
        const auto graphicsRuntime = *reinterpret_cast<const PM_GRAPHICS_RUNTIME*>(&pBlob[queryElements[1].dataOffset]);
        const auto syncInterval = *reinterpret_cast<const uint32_t*>(&pBlob[queryElements[2].dataOffset]);
        const auto presentFlags = *reinterpret_cast<const uint32_t*>(&pBlob[queryElements[3].dataOffset]);
        const auto allowsTearing = *reinterpret_cast<const bool*>(&pBlob[queryElements[4].dataOffset]);
        const auto presentMode = *reinterpret_cast<const PM_PRESENT_MODE*>(&pBlob[queryElements[5].dataOffset]);
        const auto cpuFrameQpc = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[6].dataOffset]);
        const auto cpuDuration = *reinterpret_cast<const double*>(&pBlob[queryElements[7].dataOffset]);
        const auto cpuFramePacingStall = *reinterpret_cast<const double*>(&pBlob[queryElements[8].dataOffset]);
        const auto gpuLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[9].dataOffset]);
        const auto gpuDuration = *reinterpret_cast<const double*>(&pBlob[queryElements[10].dataOffset]);
        const auto gpuBusyTime = *reinterpret_cast<const double*>(&pBlob[queryElements[11].dataOffset]);
        const auto gpuDisplayLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[12].dataOffset]);
        const auto gpuDisplayDuration = *reinterpret_cast<const double*>(&pBlob[queryElements[13].dataOffset]);
        const auto gpuInputLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[14].dataOffset]);
        const auto variableMetricData = *reinterpret_cast<const double*>(&pBlob[queryElements[15].dataOffset]);
        try {
            csvFile << processName << ",";
            csvFile << processId << ",";
            csvFile << std::hex << "0x" << std::dec << swapChain << ",";
            csvFile << TranslateGraphicsRuntime(graphicsRuntime) << ",";
            csvFile << syncInterval << ",";
            csvFile << presentFlags << ",";
            csvFile << allowsTearing << ",";
            csvFile << TranslatePresentMode(presentMode) << ",";
            csvFile << cpuFrameQpc << ",";
            csvFile << cpuDuration << ",";
            csvFile << cpuFramePacingStall << ",";
            csvFile << gpuLatency << ",";
            csvFile << gpuDuration << ",";
            csvFile << gpuBusyTime << ",";
            csvFile << gpuDisplayLatency << ",";
            csvFile << gpuDisplayDuration << ",";
            csvFile << gpuInputLatency << ",";
            csvFile << variableMetricData << "\n";
        }
        catch (...) {
            std::cout << "Failed CSV output of frame data.\n";
        }
    }
}

std::optional<std::ofstream> CreateCsvFile(std::string& processName, std::string& variableMetricName)
{
    // Setup csv file
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm local_time;
    localtime_s(&local_time, &now);
    std::ofstream csvFile;
    std::string csvFileName = processName;

    try {
        csvFileName += "_" + std::to_string(local_time.tm_year + 1900) +
            std::to_string(local_time.tm_mon + 1) +
            std::to_string(local_time.tm_mday + 1) +
            std::to_string(local_time.tm_hour) +
            std::to_string(local_time.tm_min) +
            std::to_string(local_time.tm_sec) + ".csv";
    }
    catch (const std::exception& e) {
        std::cout
            << "a standard exception was caught, with message '"
            << e.what() << "'" << std::endl;
        return std::nullopt;
    }

    try {
        csvFile.open(csvFileName);
    }
    catch (...) {
        std::cout << "Unabled to open csv file" << std::endl;
        return std::nullopt;
    }

    csvFile << "Application,ProcessID,SwapChainAddress,PresentRuntime,"
        "SyncInterval,PresentFlags,AllowsTearing,PresentMode,"
        "CPUFrameQPC,CPUDuration,CPUFramePacingStall,"
        "GPULatency,GPUDuration,GPUBusy,DisplayLatency,"
        "DisplayDuration,InputLatency," << variableMetricName;
    csvFile << std::endl;

    return csvFile;
}

int RecordFrames(std::string controlPipe, std::string introspectionNsm, std::string processName, unsigned int processId)
{
    std::unique_ptr<pmapi::Session> pSession;
    pmapi::ProcessTracker processTracker;
    static constexpr uint32_t numberOfBlobs = 150u;

    try {
        if (controlPipe.empty() == false) {
            pSession = std::make_unique<pmapi::Session>(controlPipe, introspectionNsm);
        }
        else {
            pSession = std::make_unique<pmapi::Session>();
        }
    }
    catch (const std::bad_array_new_length& e) {
        std::cout
            << "Creating a new PresentMon session caused bad array new length exception, with message '"
            << e.what() << "'" << std::endl;
        return -1;
    }
    catch (const std::runtime_error& e) {
        std::cout
            << "Creating a new PresentMon session caused std::runtime exception '"
            << e.what() << "'" << std::endl;
        return -1;
    }

    // Search through introspection for first available GPU
    // metric. If we happen to be on a machine with an unsupported GPU
    // capture CPU utilization instead
    auto pIntrospectionRoot = pSession->GetIntrospectionRoot();
    auto metricEnums = pIntrospectionRoot->FindEnum(PM_ENUM_METRIC);
    PM_METRIC variableMetric = PM_METRIC_CPU_UTILIZATION;
    std::string variableMetricName = "CPUUtilization";
    uint32_t variableMetricDeviceId = 0;

    // Loop through ALL PresentMon metrics
    for (auto metric : pIntrospectionRoot->GetMetrics())
    {
        // Go through the device metric info to determine the metric's availability
        auto metricInfo = metric.GetDeviceMetricInfo();
        for (auto mi : metricInfo)
        {
            auto device = mi.GetDevice();
            if (device.GetId() != 0)
            {
                variableMetric = metric.GetId();
                // Look through PM_ENUM_METRIC enums to gather the metric symbol
                for (auto key : metricEnums.GetKeys())
                {
                    if (key.GetId() == metric.GetId())
                    {
                        variableMetricName = key.GetName();
                        break;
                    }
                }
                variableMetricDeviceId = device.GetId();
                break;
            }
        }
        if (variableMetricDeviceId != 0) {
            break;
        }
    }

    // TODO: Had to comment out the application metric because when
    // calling RegisterFrameQuery we crash
    PM_QUERY_ELEMENT queryElements[]{
        //{ PM_METRIC_APPLICATION, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_SWAP_CHAIN_ADDRESS, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_SYNC_INTERVAL, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_PRESENT_FLAGS, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_ALLOWS_TEARING, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_CPU_FRAME_QPC, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_CPU_DURATION, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_CPU_FRAME_PACING_STALL, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_GPU_LATENCY, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_GPU_DURATION, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_GPU_BUSY_TIME, PM_STAT_NONE, 0, 0},
        { PM_METRIC_DISPLAY_LATENCY, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_DISPLAY_DURATION, PM_STAT_NONE, 0, 0 },
        { PM_METRIC_INPUT_LATENCY, PM_STAT_NONE, 0, 0},
        { variableMetric, PM_STAT_NONE, variableMetricDeviceId, 0}
    };

    PM_FRAME_QUERY_HANDLE hEventQuery = nullptr;
    uint32_t blobSize = 0;
    auto frameQuery = pSession->RegisterFrameQuery(queryElements);
    auto blobs = frameQuery.MakeBlobContainer(numberOfBlobs);

    try {
        processTracker = pSession->TrackProcess(processId);
    }
    catch (...) {
        std::cout
            << "Unable to track process: "
            << processId << std::endl;
        return -1;
    }

    auto csvStream = CreateCsvFile(processName, variableMetricName);
    if (!csvStream.has_value())
    {
        return -1;
    }

    while (true) {
        std::cout << "Checking for new frames...\n";
        uint32_t numFrames = numberOfBlobs;
        frameQuery.Consume(processTracker, blobs);
        if (blobs.GetNumBlobsPopulated() == 0) {
            std::this_thread::sleep_for(200ms);
        }
        else {
            std::cout << std::format("Dumping [{}] frames...\n", numFrames);
            WriteToCSV(csvStream.value(), processName, processId, queryElements, blobs);
        }

        if (gQuit){
            break;
        }
    }

    //processTracker.Reset();
    //pSession->Reset();
    csvStream.value().close();

    return 0;
}

int main(int argc, char* argv[])
{
    if (auto e = clio::Options::Init(argc, argv)) {
        return *e;
    }
    auto& opt = clio::Options::Get();
    // validate options, better to do this with CLI11 validation but framework needs upgrade...
    if (bool(opt.controlPipe) != bool(opt.introNsm)) {
        OutputString("Must set both control pipe and intro NSM, or neither.\n");
        return -1;
    }

    // determine requested activity
    if (opt.viewAvailableMetrics ^ opt.pollMetrics ^ opt.recordFrames) {
        if (opt.viewAvailableMetrics) {
            return ViewAvailableSystemMetrics(*opt.controlPipe, *opt.introNsm);
        }
        else {
            if (bool(opt.processName) ^ bool(opt.processId)) {
                std::optional<unsigned int> processId;
                std::optional<std::string> processName;
                if (bool(opt.processName))
                {
                    processName = *opt.processName;
                }
                if (bool(opt.processId))
                {
                    processId = *opt.processId;
                }
                GetProcessInformation(processName, processId);
                if (!processName.has_value() || !processId.has_value())
                {
                    OutputString("Unable to track requested process.\n");
                    return -1;
                }
                // Create an event
                gCloseEvent = CreateEvent(NULL,   // default security attributes
                    TRUE,   // manual reset event
                    FALSE,  // not signaled
                    NULL);  // no name

                if (SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
                    if (opt.pollMetrics) {
                        std::thread pollMetricsThread(PollMetricsThread, *opt.controlPipe,
                            *opt.introNsm, processId.value());
                        // Wait for the metrics capture thread to finish
                        pollMetricsThread.join();
                        return 0;
                    }
                    else {
                        std::thread recordFramesThread(RecordFrames, *opt.controlPipe, *opt.introNsm,
                            processName.value(), processId.value());
                        // Wait for the metrics capture thread to finish
                        recordFramesThread.join();
                        return 0;
                    }
                }
            }
            else {
                OutputString("Must set eiter the application name or the process id:\n");
                OutputString("--poll-metrics [--process-id id | --process-name name.exe]\n");
                OutputString("--record-frames [--process-id id | --process-name name.exe]\n");
                return -1;
            }
        }
    } else {
        OutputString("SampleClient supports one action at a time. Set one of:\n");
        OutputString("--view-available-metrics\n");
        OutputString("--poll-metrics [--process-id id | --process-name name.exe]\n");
        OutputString("--record-frames [--process-id id | --process-name name.exe]\n");
        return -1;
    }
}
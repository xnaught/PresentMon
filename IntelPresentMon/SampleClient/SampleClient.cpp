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
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include "../PresentMonAPI2/source/Internal.h"
#include "CliOptions.h"

#undef ENABLE_FRAME_DATA_WRITE
#undef ENABLE_PRESENT_MODE
#undef ENABLE_METRICS
#undef ENABLE_ETL
#undef ENABLE_STATIC_QUERIES

using namespace std::chrono;

HANDLE gCloseEvent;
bool gQuit = false;
uint32_t gCurrentPid = 0;
std::ofstream g_csv_file;
std::string g_process_name;
std::string g_etl_file_name;
int32_t g_metrics_offset = 0;
bool g_record_frames = false;
const double kWindowSize = 2000.0;
const uint32_t kSleepTime = 4;
const uint32_t kNumFramesInBuf = 1000;
std::vector<char> g_cpu_name;

    // Main menu actions
enum MenuActions{
  kProcessETL = 1,
  kProcessLive,
  kQuit
};

int g_menu_action;

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

template<typename T>
std::string TranslateOptionalTelemetry(T telemetry_item) {
  std::string data;
  data = (telemetry_item.valid == true) ? std::to_string(telemetry_item.data) : "NA";
  return data;
}

#ifdef ENABLE_FRAME_DATA_WRITE
void WriteToCSV(PM_FRAME_DATA* data) {
  try {
    g_csv_file << "\n";
    g_csv_file << data->application << ",";
    g_csv_file << data->process_id << ",";
    g_csv_file << std::hex << "0x" << data->swap_chain_address << std::dec
               << ",";
    g_csv_file << data->runtime << ",";
    g_csv_file << data->sync_interval << ",";
    g_csv_file << data->present_flags << ",";
    g_csv_file << data->dropped << ",";
    g_csv_file << data->time_in_seconds << ",";
    g_csv_file << data->ms_in_present_api << ",";
    g_csv_file << data->ms_between_presents << ",";
    g_csv_file << data->allows_tearing << ",";
    g_csv_file << data->present_mode << ",";
    g_csv_file << data->ms_until_render_complete << ",";
    g_csv_file << data->ms_until_displayed << ",";
    g_csv_file << data->ms_between_display_change << ",";
    g_csv_file << data->ms_until_render_start << ",";
    g_csv_file << data->ms_gpu_active << ",";
    g_csv_file << std::to_string(data->qpc_time) << ",";
    // power telemetry
    g_csv_file << TranslateOptionalTelemetry(data->gpu_power_w).c_str() << ",";
    g_csv_file << TranslateOptionalTelemetry(data->gpu_sustained_power_limit_w).c_str() << ",";
    g_csv_file << TranslateOptionalTelemetry(data->gpu_voltage_v).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->gpu_frequency_mhz).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->gpu_temperature_c).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->gpu_utilization).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(
                      data->gpu_render_compute_utilization)
                      .c_str()
               << ",";
    g_csv_file
        << TranslateOptionalTelemetry(data->gpu_media_utilization).c_str()
        << ",";

    for (int i = 0; i < MAX_PM_FAN_COUNT; i++) {
      g_csv_file << TranslateOptionalTelemetry(data->fan_speed_rpm[i]).c_str()
                 << ",";
    }

    g_csv_file << TranslateOptionalTelemetry(data->vram_frequency_mhz).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->vram_effective_frequency_gbs)
                      .c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->vram_temperature_c).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->vram_power_w).c_str() << ",";
    g_csv_file << TranslateOptionalTelemetry(data->vram_voltage_v).c_str()
               << ",";

    for (int i = 0; i < MAX_PM_PSU_COUNT; i++) {
      g_csv_file << TranslateOptionalTelemetry(data->psu_type[i]).c_str()
                 << ",";
      g_csv_file << TranslateOptionalTelemetry(data->psu_power[i]).c_str()
                 << ",";
      g_csv_file << TranslateOptionalTelemetry(data->psu_voltage[i]).c_str()
                 << ",";
    }

    // Throttling flags
    g_csv_file << TranslateOptionalTelemetry(data->gpu_power_limited).c_str()
               << ",";
    g_csv_file
        << TranslateOptionalTelemetry(data->gpu_temperature_limited).c_str()
        << ",";
    g_csv_file << TranslateOptionalTelemetry(data->gpu_current_limited).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->gpu_voltage_limited).c_str()
               << ",";
    g_csv_file
        << TranslateOptionalTelemetry(data->gpu_utilization_limited).c_str()
        << ",";

    g_csv_file << TranslateOptionalTelemetry(data->vram_power_limited).c_str()
               << ",";
    g_csv_file
        << TranslateOptionalTelemetry(data->vram_temperature_limited).c_str()
        << ",";
    g_csv_file << TranslateOptionalTelemetry(data->vram_current_limited).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->vram_voltage_limited).c_str()
               << ",";
    g_csv_file
        << TranslateOptionalTelemetry(data->vram_utilization_limited).c_str()
        << ",";

    g_csv_file << TranslateOptionalTelemetry(data->cpu_utilization).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->cpu_power_w).c_str() << ",";
    g_csv_file << TranslateOptionalTelemetry(data->cpu_power_limit_w).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->cpu_temperature_c).c_str()
               << ",";
    g_csv_file << TranslateOptionalTelemetry(data->cpu_frequency).c_str()
               << ",";
  } catch (...) {
    std::cout << "Failed CSV output of frame data.\n";
  }
}

void RecordFrames(bool is_etl) {
  PM_STATUS pmStatus = PM_STATUS::PM_STATUS_SUCCESS;
  uint32_t in_out_num_frames = 1;
  uint32_t frames_recorded = 0;
  PM_FRAME_DATA* out_data = new PM_FRAME_DATA[kNumFramesInBuf];
  if (out_data == nullptr) {
    return;
  }

  // Setup csv file
  time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  tm local_time;
  localtime_s(&local_time, &now);
  std::string csv_filename;
  if (is_etl) {
    std::size_t found = g_etl_file_name.find_last_of("/\\");
    csv_filename = g_etl_file_name.substr(found+1);
  } else {
    csv_filename = g_process_name;
  }

  try {
    csv_filename += "_" + std::to_string(local_time.tm_year + 1900) +
                    std::to_string(local_time.tm_mon + 1) +
                    std::to_string(local_time.tm_mday + 1) +
                    std::to_string(local_time.tm_hour) +
                    std::to_string(local_time.tm_min) +
                    std::to_string(local_time.tm_sec) + ".csv";
  } catch (const std::exception& e) {
    std::cout
        << "a standard exception was caught, with message '"
        << e.what() << "'" << std::endl;
    return;
  }
  try {
    g_csv_file.open(csv_filename);
  } catch (...) {
    std::cout << "Unabled to open csv file" << std::endl;
    return;
  }

  g_csv_file << "Application,ProcessID,SwapChainAddress,Runtime,"
                "SyncInterval,PresentFlags,Dropped,TimeInSeconds,"
                "msInPresentAPI, msBetweenPresents,"
                "AllowsTearing,PresentMode,msUntilRenderComplete,"
                "msUntilDisplayed,msBetweenDisplayChange,msUntilRenderStart,"
                "msGPUActive,QPCTime,GpuPower,GpuSustainedPowerLimit,"
                "GpuVoltage,GpuFrequency,"
                "GpuTemp,GpuUtilization,GpuRenderComputeUtilization,"
                "GpuMediaUtilization,GpuFan[0],GpuFan[1],GpuFan[2],GpuFan[3],"
                "GpuFan[4],VramFrequency,VramEffectiveFreq,VramReadBandwidth,"
                "VramWriteBandwidthCounter,VramTemperature,"
                "VramPower,VramVoltage,"
                "PsuType[0],PsuPower[0],PsuVoltage[0],"
                "PsuType[1],PsuPower[1],PsuVoltage[1],"
                "PsuType[2],PsuPower[2],PsuVoltage[2],"
                "PsuType[3],PsuPower[3],PsuVoltage[3],"
                "PsuType[4],PsuPower[4],PsuVoltage[4],"
                "GpuPowerLimited,"
                "GpuTemperatureLimited,GpuCurrentLimited,"
                "GpuVoltageLimited,GpuUtilizationLimited,VramPowerLimited,"
                "VramTemperatureLimited,VramCurrentLimited,VramVoltageLimited,"
                "VramUtilzationLimited,CpuUtilization,CpuPower,CpuPowerLimit,"
                "CpuTemperature,CpuFrequency";

  for (;;) {
    Sleep(kSleepTime);
    in_out_num_frames = kNumFramesInBuf;
    if (is_etl) {
      pmStatus =
          pmGetEtlFrameData(&in_out_num_frames, out_data);

    } else {
      if (g_menu_action == MenuActions::kProcessLive) {
        pmStatus = pmGetFrameData(gCurrentPid, &in_out_num_frames, out_data);
      } else {
        pmStatus =
            pmGetStreamAllFrameData(&in_out_num_frames, out_data);
      }
    }
    if (pmStatus == PM_STATUS::PM_STATUS_DATA_LOSS) {
      ConsolePrintLn("Data loss occurred during recording. Exiting...");
      break;
    } else if (pmStatus == PM_STATUS::PM_STATUS_PROCESS_NOT_EXIST) {
      if (is_etl) {
        ConsolePrintLn("Finished processing ETL file. Exiting...");
        
      } else {
        ConsolePrintLn("Process has closed. Exiting...");
      }
      break;
    } else if (pmStatus == PM_STATUS::PM_STATUS_NO_DATA) {
      // No frames were recorded, continue along
      continue;
    } else if (pmStatus == PM_STATUS::PM_STATUS_SUCCESS) {
      frames_recorded += in_out_num_frames;
      ConsolePrintLn("Total Frames Recorded: %d", frames_recorded);
      for (uint32_t i = 0; i < in_out_num_frames; i++) {
        WriteToCSV(&out_data[i]);
      }
    } else {
      PrintError(pmStatus);
    }
    CommitConsole();
    if (gQuit) {
      ConsolePrintLn("Exiting recording mode...");
      break;
    }
  }
  CommitConsole();
  delete[] out_data;
}
#endif

#ifdef ENABLE_PRESENT_MODE
void PrintPresentMode(PM_PRESENT_MODE present_mode) {
  switch (present_mode) {
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP:
      ConsolePrintLn("Hardware: Legacy Flip");
      break;
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER:
      ConsolePrintLn("Hardware: Legacy Copy to front buffer");
      break;
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP:
      ConsolePrintLn("Hardware: Independent Flip");
      break;
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_FLIP:
      ConsolePrintLn("Composed: Flip");
      break;
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_COMPOSED_INDEPENDENT_FLIP:
      ConsolePrintLn("Hardware Composed: Independent Flip");
      break;
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_COPY_WITH_GPU_GDI:
      ConsolePrintLn("Composed: Copy with GPU GDI");
      break;
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_COPY_WITH_CPU_GDI:
      ConsolePrintLn("Composed: Copy with CPU GDI");
      break;
    default:
      ConsolePrintLn("Present Mode: Unknown");
      break;
  }
  return;
}
#endif

#ifdef ENABLE_METRICS
void PrintSwapChainMetrics(PM_FPS_DATA* fps_data, uint32_t num_gfx_swap_chains,
                           PM_GFX_LATENCY_DATA* latency_data,
                           uint32_t num_latency_swap_chains) {
  ConsolePrintLn("pid = %i", gCurrentPid);
  for (uint32_t i = 0; i < num_gfx_swap_chains; i++) {
    ConsolePrintLn("Swapchain = %016llX", fps_data[i].swap_chain);
    ConsolePrintLn("Presented fps Average = %f", fps_data[i].presented_fps.avg);
    ConsolePrintLn("Raw Frametimes = %f ms", fps_data[i].frame_time_ms.raw);
    if (num_gfx_swap_chains == 1) {
      // If we only have a single swap chain print all of the available
      // metrics.
      ConsolePrintLn("Presented fps High = %f FPS", fps_data[i].presented_fps.high);
      ConsolePrintLn("Presented fps Low = %f", fps_data[i].presented_fps.low);
      ConsolePrintLn("Presented fps 90%% = %f", fps_data[i].presented_fps.percentile_90);
      ConsolePrintLn("Presented fps 95%% = %f", fps_data[i].presented_fps.percentile_95);
      ConsolePrintLn("Presented fps 99%% = %f", fps_data[i].presented_fps.percentile_99);
      ConsolePrintLn("Displayed fps Average = %f", fps_data[i].displayed_fps.avg);
      ConsolePrintLn("Displayed fps High = %f", fps_data[i].displayed_fps.high);
      ConsolePrintLn("Displayed fps Low = %f", fps_data[i].displayed_fps.low);
      ConsolePrintLn("Displayed fps 90%% = %f",
                     fps_data[i].displayed_fps.percentile_90);
      ConsolePrintLn("Displayed fps 95%% = %f",
                     fps_data[i].displayed_fps.percentile_95);
      ConsolePrintLn("Displayed fps 99%% = %f",
                     fps_data[i].displayed_fps.percentile_99);
      ConsolePrintLn("GPU busy = %f ms", fps_data[i].gpu_busy.avg);
      ConsolePrintLn("Dropped Frames = %f %", fps_data[i].percent_dropped_frames);
      ConsolePrintLn("Vsync = %d", fps_data[i].sync_interval);
      PrintPresentMode(fps_data[i].present_mode);
    }
    for (uint32_t j = 0; j < num_latency_swap_chains; j++) {
      // Print out the latency data with the matching swapchain if
      // one exists
      if (latency_data[j].swap_chain == fps_data[i].swap_chain) {
        ConsolePrintLn("Render Latency = %f",
                       latency_data[i].render_latency_ms.avg);
        ConsolePrintLn("Display Latency = %f",
                       latency_data[i].display_latency_ms.avg);
      }
    }
  }
}
#endif

void PrintDeviceVendor(char const* vendorLabel, PM_DEVICE_VENDOR deviceVendor) {
    ConsolePrint(vendorLabel);
    switch (deviceVendor) {
    case PM_DEVICE_VENDOR_INTEL:
        ConsolePrintLn("PM_DEVICE_VENDOR_INTEL");
        break;
    case PM_DEVICE_VENDOR_NVIDIA:
        ConsolePrintLn("PM_DEVICE_VENDOR_NVIDIA");
        break;
    case PM_DEVICE_VENDOR_AMD:
        ConsolePrintLn("PM_DEVICE_VENDOR_AMD");
        break;
    case PM_DEVICE_VENDOR_UNKNOWN:
        ConsolePrintLn("PM_DEVICE_VENDOR_UNKNOWN");
        break;
    default:
        ConsolePrintLn("PM_DEVICE_VENDOR_UNKNOWN");
        break;
    }
    return;
}

bool GetUserInput(std::string& input){
  try {
    std::getline(std::cin, input);
    return true;
  } catch (const std::exception& e) {
    std::cout << "a standard exception was caught, with message '" << e.what()
              << "'" << std::endl;
    std::cout << "Exiting SampleClient" << std::endl;
    return false;
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

#ifdef ENABLE_METRICS
void ReadMetrics() {
  PM_STATUS pmStatus = PM_STATUS::PM_STATUS_SUCCESS;

  PM_FPS_DATA* fps_data;
  PM_GFX_LATENCY_DATA* latency_data;
  PM_GPU_DATA gpu_data = {};
  PM_CPU_DATA cpu_data = {};
  uint32_t num_gfx_swap_chains = 1;
  uint32_t num_latency_swap_chains = 1;
  uint32_t current_num_gfx_swap_chains = num_gfx_swap_chains;
  uint32_t current_num_latency_swap_chains = num_latency_swap_chains;

  if (g_record_frames) {
    RecordFrames(false);
    return;
  }

  pmSetMetricsOffset(g_metrics_offset);

  // Allocate single swap chains for all metrics to start
  fps_data = new PM_FPS_DATA[num_gfx_swap_chains];
  latency_data = new PM_GFX_LATENCY_DATA[num_latency_swap_chains];
  if ((fps_data == nullptr) || (latency_data == nullptr)) {
    return;
  }

  for (;;) {
    Sleep(kSleepTime);
    pmStatus = pmGetFramesPerSecondData(gCurrentPid, fps_data, kWindowSize,
                                        &num_gfx_swap_chains);

    if (pmStatus == PM_STATUS::PM_STATUS_SUCCESS) {
      pmStatus = pmGetGfxLatencyData(gCurrentPid, latency_data, kWindowSize,
                                     &num_latency_swap_chains);
      if (pmStatus == PM_STATUS::PM_STATUS_SUCCESS) {
        // Only print out the swap chain metrics if able to get both
        // fps and latency successfully.
        PrintSwapChainMetrics(fps_data, current_num_gfx_swap_chains,
                              latency_data, current_num_latency_swap_chains);
      }
    }

    pmStatus = pmGetGPUData(gCurrentPid, &gpu_data, kWindowSize);
    if (pmStatus == PM_STATUS::PM_STATUS_SUCCESS) {
      PrintMetric("GPU Mem Max Bandwidth = %f",
                  gpu_data.gpu_mem_max_bandwidth_bps.avg / 1e-6,
                  gpu_data.gpu_mem_max_bandwidth_bps.valid);
      PrintMetric("GPU Memory Utilization = %.f %%",
                  gpu_data.gpu_mem_utilization.avg,
                  gpu_data.gpu_mem_utilization.valid);
      PrintMetric("GPU Memory Used = %.f MB",
                  gpu_data.gpu_mem_used_b.avg / 1000000.,
                  gpu_data.gpu_mem_utilization.valid);
      PrintMetric("GPU Voltage = %.f mV", 1000.0 * gpu_data.gpu_voltage_v.avg,
                  gpu_data.gpu_voltage_v.valid);
      PrintMetric("GPU Clock = %.f MHz", gpu_data.gpu_frequency_mhz.avg,
                  gpu_data.gpu_frequency_mhz.valid);
      PrintMetric("GPU Temperature = %.f C", gpu_data.gpu_temperature_c.avg,
                  gpu_data.gpu_temperature_c.valid);
      PrintMetric("GPU Power = %.f W", gpu_data.gpu_power_w.avg,
                  gpu_data.gpu_power_w.valid);
      PrintMetric("GPU Power Limit = %.f W",
                  gpu_data.gpu_sustained_power_limit_w.avg,
                  gpu_data.gpu_sustained_power_limit_w.valid);
      PrintMetric("GPU Activity = %.f %%", gpu_data.gpu_utilization.avg,
                  gpu_data.gpu_utilization.valid);
      PrintMetric("VRAM Clock = %.f MHz", gpu_data.vram_frequency_mhz.avg,
                  gpu_data.vram_frequency_mhz.valid);
      PrintMetric("VRAM Effective Frequency = %.f GTs",
                  gpu_data.vram_effective_frequency_gbps.avg,
                  gpu_data.vram_effective_frequency_gbps.valid);
      PrintMetric("VRAM Temperature = %.f C", gpu_data.vram_temperature_c.avg,
                  gpu_data.vram_temperature_c.valid);
      PrintMetric("VRAM Read Bandwidth = %.f MBs",
                  gpu_data.vram_read_bandwidth_bps.avg / 1e-6,
                  gpu_data.vram_read_bandwidth_bps.valid);
      PrintMetric("VRAM Write Bandwidth = %.f MBs",
                  gpu_data.vram_write_bandwidth_bps.avg / 1e-6,
                  gpu_data.vram_write_bandwidth_bps.valid);
      PrintMetric("Fan Speed = %.f RPM", gpu_data.gpu_fan_speed_rpm[0].avg,
                  gpu_data.gpu_fan_speed_rpm[0].valid);
      PrintMetric("Render Activity = %.f %%",
                  gpu_data.gpu_render_compute_utilization.avg,
                  gpu_data.gpu_render_compute_utilization.valid);
      PrintMetric("Media Activity = %.f %%", gpu_data.gpu_media_utilization.avg,
                  gpu_data.gpu_media_utilization.valid);
      PrintMetric("VRAM Core Power = %f W", gpu_data.vram_power_w.avg,
                  gpu_data.vram_power_w.valid);
      PrintMetric("VRAM Core Voltage = %f V", gpu_data.vram_voltage_v.avg,
                  gpu_data.vram_voltage_v.valid);

      // The limited flags return back a value between 0 and 1.
      PrintMetric("GPU Power Limited = %.f %%",
                  gpu_data.gpu_power_limited.avg * 100.0,
                  gpu_data.gpu_power_limited.valid);
      PrintMetric("GPU Temperature Limited = %.f %%",
                  gpu_data.gpu_temperature_limited.avg * 100.0,
                  gpu_data.gpu_temperature_limited.valid);
      PrintMetric("GPU Current Limited = %.f %%",
                  gpu_data.gpu_current_limited.avg * 100.0,
                  gpu_data.gpu_current_limited.valid);
      PrintMetric("GPU Voltage Limited = %.f %%",
                  gpu_data.gpu_voltage_limited.avg * 100.0,
                  gpu_data.gpu_voltage_limited.valid);
      PrintMetric("GPU Utilization Limited = %.f %%",
                  gpu_data.gpu_utilization_limited.avg * 100.0,
                  gpu_data.gpu_utilization_limited.valid);

      PrintMetric("VRAM Power Limited = %.f %%",
                  gpu_data.vram_power_limited.avg * 100.0,
                  gpu_data.vram_power_limited.valid);
      PrintMetric("VRAM Temperature Limited = %.f %%",
                  gpu_data.vram_temperature_limited.avg * 100.0,
                  gpu_data.vram_temperature_limited.valid);
      PrintMetric("VRAM Current Limited = %.f %%",
                  gpu_data.vram_current_limited.avg * 100.0,
                  gpu_data.vram_current_limited.valid);
      PrintMetric("VRAM Voltage Limited = %.f %%",
                  gpu_data.vram_voltage_limited.avg * 100.0,
                  gpu_data.vram_voltage_limited.valid);
      PrintMetric("VRAM Utilization Limited = %.f %%",
                  gpu_data.vram_utilization_limited.avg * 100.0,
                  gpu_data.vram_utilization_limited.valid);
    } else {
      PrintError(pmStatus);
    }

    // Print CPU name
    if (g_cpu_name.size() > 0) {
      ConsolePrintLn(g_cpu_name.data());
    }
    pmStatus = pmGetCPUData(gCurrentPid, &cpu_data, kWindowSize);
    if (pmStatus == PM_STATUS::PM_STATUS_SUCCESS) {
      PrintMetric("CPU Utilization = %.f %%", cpu_data.cpu_utilization.avg,
                  cpu_data.cpu_utilization.valid);
      PrintMetric("CPU Power = %.f W", cpu_data.cpu_power_w.avg,
                  cpu_data.cpu_power_w.valid);
      PrintMetric("CPU Power Limit 1 = %.f W", cpu_data.cpu_power_limit_w.avg,
                  cpu_data.cpu_power_limit_w.valid);
      PrintMetric("CPU Temperature = %.f C", cpu_data.cpu_temperature_c.avg,
                  cpu_data.cpu_temperature_c.valid);
      PrintMetric("CPU Frequency = %.f MHz", cpu_data.cpu_frequency.avg,
                  cpu_data.cpu_frequency.valid);
    } else {
      PrintError(pmStatus);
    }

    CommitConsole();

    if (gQuit == true) {
      break;
    }

    if (current_num_gfx_swap_chains < num_gfx_swap_chains) {
      delete[] fps_data;
      // Update the allocated number of swap chains for all metrics
      fps_data = new PM_FPS_DATA[num_gfx_swap_chains];
      if (fps_data == nullptr) {
        return;
      }
      current_num_gfx_swap_chains = num_gfx_swap_chains;
    }

    if (current_num_latency_swap_chains < num_latency_swap_chains) {
      delete[] latency_data;
      // Update the allocated number of swap chains for all metrics
      latency_data = new PM_GFX_LATENCY_DATA[num_latency_swap_chains];
      if (latency_data == nullptr) {
        return;
      }
      current_num_latency_swap_chains = num_latency_swap_chains;
    }
  }

  delete[] fps_data;
  delete[] latency_data;
}
#endif

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

void SetRecordFrames() {
  bool valid_selection = false;
  std::string action;

  while (valid_selection == false) {
    OutputString("Select Action:\n");
    OutputString("(1) Display Metrics\n");
    OutputString("(2) Record Frames\n");
    if (GetUserInput(action) == false) {
      gQuit = true;
      return;
    }
    if (action.length() != 0) {
      int action_num = std::stoi(action);
      if (action_num == 1) {
        g_record_frames = false;
        return;
      }
      if (action_num == 2) {
        g_record_frames = true;
        return;
      }
    } else {
      gQuit = true;
    }
  }
}

int32_t DisplayMainMenu() {
  bool valid_selection = false;
  std::string action;
  int action_num = MenuActions::kQuit;

  while (valid_selection == false) {
    OutputString("Set Action:\n");
    OutputString("(1) Process ETL File\n");
    OutputString("(2) Real Time PresentMon for Single Process\n");
    OutputString("(4) Quit\n");
    if (GetUserInput(action) == false) {
      break;
    }
    if (action.length() != 0) {
      action_num = std::stoi(action);
      if (action_num >= MenuActions::kProcessETL &&
          action_num <= MenuActions::kQuit) {
        break;
      }
    }
  }
  return action_num;
}

int32_t GetMetricsOffset() {
  bool valid_selection = false;
  std::string action;
  int metrics_offset;

  while (valid_selection == false) {
    OutputString("Set Metrics Offset(ms) (Enter 0 for most recent metrics):");
    if (GetUserInput(action) == true) {
      if (action.length() != 0) {
        // Ensure the metric input is valid
        try {
          metrics_offset = std::stoi(action);
        } catch (std::invalid_argument) {
          OutputString("Invalid offset.\n");
          continue;
        } catch (std::out_of_range) {
          OutputString("Invalid offset.\n");
          continue;
        }
        // Metrics offsets must be positive
        if (metrics_offset < 0) {
          OutputString("Invalid offset.\n");
          continue;
        }
        valid_selection = true;
      }
    }
  }
  return metrics_offset;
}

DWORD FindProcessId(const std::string& process_name) {
  PROCESSENTRY32 process_info;
  process_info.dwSize = sizeof(process_info);

  HANDLE processes_snapshot =
      CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  if (processes_snapshot == INVALID_HANDLE_VALUE) {
    return 0;
  }

  Process32First(processes_snapshot, &process_info);

  if (strcmp(process_info.szExeFile, process_name.c_str()) == 0) {
    CloseHandle(processes_snapshot);
    return process_info.th32ProcessID;
  }

  while (Process32Next(processes_snapshot, &process_info)) {
    if (strcmp(process_info.szExeFile, process_name.c_str()) == 0) {
      CloseHandle(processes_snapshot);
      return process_info.th32ProcessID;
    }
  }

  CloseHandle(processes_snapshot);
  return 0;
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

void PollMetrics(uint32_t processId, double metricsOffset)
{
    PM_DYNAMIC_QUERY_HANDLE q = nullptr;
    /*
    PM_QUERY_ELEMENT elements[]{
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAYED_FPS, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAY_BUSY_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAY_BUSY_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAY_BUSY_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAY_BUSY_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAY_BUSY_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAY_BUSY_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_DISPLAY_BUSY_TIME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
    };
    */

    PM_QUERY_ELEMENT elements[]{
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_FRAME_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_WAIT_TIME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_BUSY_TIME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_BUSY_TIME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_UTILIZATION, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_UTILIZATION, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_UTILIZATION, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_UTILIZATION, .stat = PM_STAT_PERCENTILE_99, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_TEMPERATURE, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_UTILIZATION, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_PROCESS_NAME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_VENDOR, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_CPU_NAME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_VENDOR, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_NAME, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_MEM_MAX_BANDWIDTH, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
        PM_QUERY_ELEMENT{.metric = PM_METRIC_GPU_MEM_SIZE, .stat = PM_STAT_RAW, .deviceId = 0, .arrayIndex = 0},
    };
    auto result = pmRegisterDynamicQuery(&q, elements, std::size(elements), 2000., metricsOffset);
    auto pBlob = std::make_unique<uint8_t[]>(elements[std::size(elements) - 1].dataOffset + elements[std::size(elements) - 1].dataSize);
    uint32_t numSwapChains = 1;

    PM_QUERY_ELEMENT staticQueryElement{ .metric = PM_METRIC_PROCESS_NAME, .deviceId = 0, .arrayIndex = 0 };
    auto processName = std::make_unique<uint8_t[]>(260);
    pmPollStaticQuery(&staticQueryElement, processId, processName.get());
    staticQueryElement = { .metric = PM_METRIC_CPU_VENDOR, .deviceId = 0, .arrayIndex = 0 };
    auto cpuVendor = std::make_unique<uint8_t[]>(4);
    pmPollStaticQuery(&staticQueryElement, processId, cpuVendor.get());
    staticQueryElement = { .metric = PM_METRIC_CPU_NAME, .deviceId = 0, .arrayIndex = 0 };
    auto cpuName = std::make_unique<uint8_t[]>(260);
    pmPollStaticQuery(&staticQueryElement, processId, cpuName.get());
    staticQueryElement = { .metric = PM_METRIC_GPU_VENDOR, .deviceId = 0, .arrayIndex = 0 };
    auto gpuVendor = std::make_unique<uint8_t[]>(4);
    pmPollStaticQuery(&staticQueryElement, processId, gpuVendor.get());
    staticQueryElement = { .metric = PM_METRIC_GPU_NAME, .deviceId = 0, .arrayIndex = 0 };
    auto gpuName = std::make_unique<uint8_t[]>(260);
    pmPollStaticQuery(&staticQueryElement, processId, gpuName.get());
    staticQueryElement = { .metric = PM_METRIC_GPU_MEM_MAX_BANDWIDTH, .deviceId = 0, .arrayIndex = 0 };
    auto gpuMemMaxBw = std::make_unique<uint8_t[]>(8);
    pmPollStaticQuery(&staticQueryElement, processId, gpuMemMaxBw.get());
    staticQueryElement = { .metric = PM_METRIC_GPU_MEM_SIZE, .deviceId = 0, .arrayIndex = 0 };
    auto gpuMemSize = std::make_unique<uint8_t[]>(8);
    pmPollStaticQuery(&staticQueryElement, processId, gpuMemSize.get());

    for (;;)
    {
        auto status = pmPollDynamicQuery(q, processId, pBlob.get(), &numSwapChains);
        if (status == PM_STATUS_SUCCESS)
        {
            ConsolePrintLn("Static Process Name = %s", reinterpret_cast<char*>(processName.get()));
            PrintDeviceVendor("Static CPU Vendor = ", reinterpret_cast<PM_DEVICE_VENDOR&>(cpuVendor[0]));
            ConsolePrintLn("Static CPU Name = %s", reinterpret_cast<char*>(cpuName.get()));
            PrintDeviceVendor("Static GPU Vendor = ", reinterpret_cast<PM_DEVICE_VENDOR&>(gpuVendor[0]));
            ConsolePrintLn("Static GPU Name = %s", reinterpret_cast<char*>(gpuName.get()));
            PrintMetric("Static GPU Memory Max Bandwidth = %f", reinterpret_cast<double&>(gpuMemMaxBw[0]), true);
            PrintMetric("Static GPU Memory Size = %f", reinterpret_cast<double&>(gpuMemSize[0]), true);

            PrintMetric("Presented fps Average = %f", reinterpret_cast<double&>(pBlob[elements[0].dataOffset]), true);
            PrintMetric("Presented fps 90% = %f", reinterpret_cast<double&>(pBlob[elements[1].dataOffset]), true);
            PrintMetric("Presented fps 95% = %f", reinterpret_cast<double&>(pBlob[elements[2].dataOffset]), true);
            PrintMetric("Presented fps 99% = %f", reinterpret_cast<double&>(pBlob[elements[3].dataOffset]), true);
            PrintMetric("Frame time Average = %f", reinterpret_cast<double&>(pBlob[elements[4].dataOffset]), true);
            PrintMetric("Frame time 90% = %f", reinterpret_cast<double&>(pBlob[elements[5].dataOffset]), true);
            PrintMetric("Frame time 95% = %f", reinterpret_cast<double&>(pBlob[elements[6].dataOffset]), true);
            PrintMetric("Frame time 99% = %f", reinterpret_cast<double&>(pBlob[elements[7].dataOffset]), true);
            PrintMetric("Cpu Wait time Average = %f", reinterpret_cast<double&>(pBlob[elements[8].dataOffset]), true);
            PrintMetric("Cpu Wait time 99% = %f", reinterpret_cast<double&>(pBlob[elements[9].dataOffset]), true);
            PrintMetric("Cpu Wait time 95% = %f", reinterpret_cast<double&>(pBlob[elements[10].dataOffset]), true);
            PrintMetric("Cpu Wait time 90% = %f", reinterpret_cast<double&>(pBlob[elements[11].dataOffset]), true);
            PrintMetric("Cpu Wait time Max = %f", reinterpret_cast<double&>(pBlob[elements[12].dataOffset]), true);
            PrintMetric("Cpu Wait time Min = %f", reinterpret_cast<double&>(pBlob[elements[13].dataOffset]), true);
            PrintMetric("Cpu Wait time Raw = %f", reinterpret_cast<double&>(pBlob[elements[14].dataOffset]), true);
            PrintMetric("Cpu Busy time Average = %f", reinterpret_cast<double&>(pBlob[elements[15].dataOffset]), true);
            PrintMetric("Cpu Busy time 99% = %f", reinterpret_cast<double&>(pBlob[elements[16].dataOffset]), true);
            PrintMetric("Cpu Busy time 95% = %f", reinterpret_cast<double&>(pBlob[elements[17].dataOffset]), true);
            PrintMetric("Cpu Busy time 90% = %f", reinterpret_cast<double&>(pBlob[elements[18].dataOffset]), true);
            PrintMetric("Cpu Busy time Max = %f", reinterpret_cast<double&>(pBlob[elements[19].dataOffset]), true);
            PrintMetric("Cpu Busy time Min = %f", reinterpret_cast<double&>(pBlob[elements[20].dataOffset]), true);
            PrintMetric("Cpu Busy time Raw = %f", reinterpret_cast<double&>(pBlob[elements[21].dataOffset]), true);
            PrintMetric("Gpu Busy time Average = %f", reinterpret_cast<double&>(pBlob[elements[22].dataOffset]), true);
            PrintMetric("Gpu Busy time 99% = %f", reinterpret_cast<double&>(pBlob[elements[23].dataOffset]), true);
            PrintMetric("Gpu Busy time 95% = %f", reinterpret_cast<double&>(pBlob[elements[24].dataOffset]), true);
            PrintMetric("Gpu Busy time 90% = %f", reinterpret_cast<double&>(pBlob[elements[25].dataOffset]), true);
            PrintMetric("Gpu Busy time Max = %f", reinterpret_cast<double&>(pBlob[elements[26].dataOffset]), true);
            PrintMetric("Gpu Busy time Min = %f", reinterpret_cast<double&>(pBlob[elements[27].dataOffset]), true);
            PrintMetric("Gpu Busy time Raw = %f", reinterpret_cast<double&>(pBlob[elements[28].dataOffset]), true);
            PrintMetric("GPU Utilization Average = %f", reinterpret_cast<double&>(pBlob[elements[29].dataOffset]), true);
            PrintMetric("GPU Utilization 90% = %f", reinterpret_cast<double&>(pBlob[elements[30].dataOffset]), true);
            PrintMetric("GPU Utilization 95% = %f", reinterpret_cast<double&>(pBlob[elements[31].dataOffset]), true);
            PrintMetric("GPU Utilization 99% = %f", reinterpret_cast<double&>(pBlob[elements[32].dataOffset]), true);
            PrintMetric("GPU Temperature Average = %f", reinterpret_cast<double&>(pBlob[elements[33].dataOffset]), true);
            PrintMetric("CPU Utilization Average = %f", reinterpret_cast<double&>(pBlob[elements[34].dataOffset]), true);
            ConsolePrintLn("Process Name = %s", reinterpret_cast<char*>(&pBlob[elements[35].dataOffset]));
            PrintDeviceVendor("CPU Vendor = ", reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[elements[36].dataOffset]));
            ConsolePrintLn("CPU Name = %s", reinterpret_cast<char*>(&pBlob[elements[37].dataOffset]));
            PrintDeviceVendor("GPU Vendor = ", reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[elements[38].dataOffset]));
            ConsolePrintLn("GPU Name = %s", reinterpret_cast<char*>(&pBlob[elements[39].dataOffset]));
            PrintMetric("GPU Memory Max Bandwidth = %f", reinterpret_cast<double&>(pBlob[elements[40].dataOffset]), true);
            PrintMetric("GPU Memory Size = %f", reinterpret_cast<double&>(pBlob[elements[41].dataOffset]), true);
            /*
            PrintMetric("Presented fps Average = %f", reinterpret_cast<double&>(pBlob[elements[0].dataOffset]), true);
            PrintMetric("Presented fps 99% = %f", reinterpret_cast<double&>(pBlob[elements[1].dataOffset]), true);
            PrintMetric("Presented fps 95% = %f", reinterpret_cast<double&>(pBlob[elements[2].dataOffset]), true);
            PrintMetric("Presented fps 90% = %f", reinterpret_cast<double&>(pBlob[elements[3].dataOffset]), true);
            PrintMetric("Presented fps Max = %f", reinterpret_cast<double&>(pBlob[elements[4].dataOffset]), true);
            PrintMetric("Presented fps Min = %f", reinterpret_cast<double&>(pBlob[elements[5].dataOffset]), true);
            PrintMetric("Presented fps Raw = %f", reinterpret_cast<double&>(pBlob[elements[6].dataOffset]), true);
            PrintMetric("Frame time Average = %f", reinterpret_cast<double&>(pBlob[elements[7].dataOffset]), true);
            PrintMetric("Frame time 99% = %f", reinterpret_cast<double&>(pBlob[elements[8].dataOffset]), true);
            PrintMetric("Frame time 95% = %f", reinterpret_cast<double&>(pBlob[elements[9].dataOffset]), true);
            PrintMetric("Frame time 90% = %f", reinterpret_cast<double&>(pBlob[elements[10].dataOffset]), true);
            PrintMetric("Frame time Max = %f", reinterpret_cast<double&>(pBlob[elements[11].dataOffset]), true);
            PrintMetric("Frame time Min = %f", reinterpret_cast<double&>(pBlob[elements[12].dataOffset]), true);
            PrintMetric("Frame time Raw = %f", reinterpret_cast<double&>(pBlob[elements[13].dataOffset]), true);
            PrintMetric("Displayed fps Average = %f", reinterpret_cast<double&>(pBlob[elements[14].dataOffset]), true);
            PrintMetric("Displayed fps 99% = %f", reinterpret_cast<double&>(pBlob[elements[15].dataOffset]), true);
            PrintMetric("Displayed fps 95% = %f", reinterpret_cast<double&>(pBlob[elements[16].dataOffset]), true);
            PrintMetric("Displayed fps 90% = %f", reinterpret_cast<double&>(pBlob[elements[17].dataOffset]), true);
            PrintMetric("Displayed fps Max = %f", reinterpret_cast<double&>(pBlob[elements[18].dataOffset]), true);
            PrintMetric("Displayed fps Min = %f", reinterpret_cast<double&>(pBlob[elements[19].dataOffset]), true);
            PrintMetric("Displayed fps Raw = %f", reinterpret_cast<double&>(pBlob[elements[20].dataOffset]), true);
            PrintMetric("Cpu Wait time Average = %f", reinterpret_cast<double&>(pBlob[elements[21].dataOffset]), true);
            PrintMetric("Cpu Wait time 99% = %f", reinterpret_cast<double&>(pBlob[elements[22].dataOffset]), true);
            PrintMetric("Cpu Wait time 95% = %f", reinterpret_cast<double&>(pBlob[elements[23].dataOffset]), true);
            PrintMetric("Cpu Wait time 90% = %f", reinterpret_cast<double&>(pBlob[elements[24].dataOffset]), true);
            PrintMetric("Cpu Wait time Max = %f", reinterpret_cast<double&>(pBlob[elements[25].dataOffset]), true);
            PrintMetric("Cpu Wait time Min = %f", reinterpret_cast<double&>(pBlob[elements[26].dataOffset]), true);
            PrintMetric("Cpu Wait time Raw = %f", reinterpret_cast<double&>(pBlob[elements[27].dataOffset]), true);
            PrintMetric("Cpu Busy time Average = %f", reinterpret_cast<double&>(pBlob[elements[28].dataOffset]), true);
            PrintMetric("Cpu Busy time 99% = %f", reinterpret_cast<double&>(pBlob[elements[29].dataOffset]), true);
            PrintMetric("Cpu Busy time 95% = %f", reinterpret_cast<double&>(pBlob[elements[30].dataOffset]), true);
            PrintMetric("Cpu Busy time 90% = %f", reinterpret_cast<double&>(pBlob[elements[31].dataOffset]), true);
            PrintMetric("Cpu Busy time Max = %f", reinterpret_cast<double&>(pBlob[elements[32].dataOffset]), true);
            PrintMetric("Cpu Busy time Min = %f", reinterpret_cast<double&>(pBlob[elements[33].dataOffset]), true);
            PrintMetric("Cpu Busy time Raw = %f", reinterpret_cast<double&>(pBlob[elements[34].dataOffset]), true);
            PrintMetric("Gpu Busy time Average = %f", reinterpret_cast<double&>(pBlob[elements[35].dataOffset]), true);
            PrintMetric("Gpu Busy time 99% = %f", reinterpret_cast<double&>(pBlob[elements[36].dataOffset]), true);
            PrintMetric("Gpu Busy time 95% = %f", reinterpret_cast<double&>(pBlob[elements[37].dataOffset]), true);
            PrintMetric("Gpu Busy time 90% = %f", reinterpret_cast<double&>(pBlob[elements[38].dataOffset]), true);
            PrintMetric("Gpu Busy time Max = %f", reinterpret_cast<double&>(pBlob[elements[39].dataOffset]), true);
            PrintMetric("Gpu Busy time Min = %f", reinterpret_cast<double&>(pBlob[elements[40].dataOffset]), true);
            PrintMetric("Gpu Busy time Raw = %f", reinterpret_cast<double&>(pBlob[elements[41].dataOffset]), true);
            PrintMetric("Display Busy time Average = %f", reinterpret_cast<double&>(pBlob[elements[42].dataOffset]), true);
            PrintMetric("Display Busy time 99% = %f", reinterpret_cast<double&>(pBlob[elements[43].dataOffset]), true);
            PrintMetric("Display Busy time 95% = %f", reinterpret_cast<double&>(pBlob[elements[44].dataOffset]), true);
            PrintMetric("Display Busy time 90% = %f", reinterpret_cast<double&>(pBlob[elements[45].dataOffset]), true);
            PrintMetric("Display Busy time Max = %f", reinterpret_cast<double&>(pBlob[elements[46].dataOffset]), true);
            PrintMetric("Display Busy time Min = %f", reinterpret_cast<double&>(pBlob[elements[47].dataOffset]), true);
            PrintMetric("Display Busy time Raw = %f", reinterpret_cast<double&>(pBlob[elements[48].dataOffset]), true);
            */
            CommitConsole();
        }

        if (gQuit == true) {
            break;
        }

        Sleep(kSleepTime);
    }

    pmFreeDynamicQuery(q);

    return;
}

int main(int argc, char* argv[]) {
    if (auto e = clio::Options::Init(argc, argv)) {
        return *e;
    }
    auto& opt = clio::Options::Get();
    // validate options, better to do this with CLI11 validation but framework needs upgrade...
    if (bool(opt.controlPipe) != bool(opt.introNsm)) {
        OutputString("Must set both control pipe and intro NSM, or neither.\n");
        return -1;
    }

  bool streamingStarted = false;

  // finer granularity sleeps
  if (timeBeginPeriod(kSleepTime) != TIMERR_NOERROR) {
    OutputString("Not able to set the Windows sleep() resolution\n");
  }

  if (InitializeConsole() == false) {
    OutputString("\nFailed to initialize console.\n");
    return -1;
  }

   g_menu_action = DisplayMainMenu();
  if (g_menu_action == kQuit) {
    return 0;
  }

  PM_STATUS pmStatus{};
  try {
      if (opt.controlPipe) {
          pmStatus = pmOpenSession_((*opt.controlPipe).c_str(), (*opt.introNsm).c_str());
      }
      else {
          pmStatus = pmOpenSession();
      }
    if (pmStatus != PM_STATUS::PM_STATUS_SUCCESS) {
      PrintError(pmStatus);
      return -1;
    }
  } catch (const std::bad_array_new_length& e) {
    std::cout
        << "pmOpenSession caused bad array new length exception, with message '"
        << e.what() << "'" << std::endl;
  } catch (const std::runtime_error& e) {
    std::cout
        << "pmOpenSession caused std::runtime exception '"
        << e.what() << "'" << std::endl;
  }

  if (g_menu_action == MenuActions::kProcessETL) {
#ifdef ENABLE_ETL
      ProcessEtl();
#endif // ENABLE_ETL
  } else {
    g_metrics_offset = GetMetricsOffset();
    gQuit = false;
    while (streamingStarted == false) {
      OutputString("Enter Process Name to monitor: \n");
      if (GetUserInput(g_process_name) == false) {
        return 0;
      }
      if (g_process_name.length() == 0) {
          pmCloseSession();
          return 0;
      }
      gCurrentPid = FindProcessId(g_process_name);
      if (gCurrentPid != 0) {
        SetRecordFrames();
        try {
          pmStatus = pmStartStreaming(gCurrentPid);
          if (pmStatus == PM_STATUS::PM_STATUS_SUCCESS) {
            streamingStarted = true;
          } else {
            OutputString("Process Name Not Found.\n");
          }
        } catch (...) {
          pmStatus = PM_STATUS::PM_STATUS_FAILURE;
          OutputString("Unable to start stream\n");
        }
      }
    }

#ifdef ENABLE_STATIC_QUERIES
    uint32_t cpu_name_size = 0;
    pmStatus = pmGetCpuName(nullptr, &cpu_name_size);
    if (cpu_name_size != 0) {
      g_cpu_name.resize(cpu_name_size);
      pmStatus = pmGetCpuName(g_cpu_name.data(), &cpu_name_size);
    }
#endif

    std::string status_string;
    try {
    status_string = std::format("Process Name: {}\n", g_process_name);
    OutputString(status_string.c_str());
    } catch (...) {
    OutputString("Process Name: Unknown\n");
    }
    try {
    status_string = std::format("Monitoring Process Id: {}\n", gCurrentPid);
    OutputString(status_string.c_str());
    } catch (...) {
    OutputString("Process Id: Unknown\n");
    }

    std::cout << "Hit Ctrl-C to exit application." << std::endl;

    // Create an event
    gCloseEvent = CreateEvent(NULL,   // default security attributes
                              TRUE,   // manual reset event
                              FALSE,  // not signaled
                              NULL);  // no name

    if (SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
      // Start metrics capture thread
      std::thread pollMetricsThread(PollMetrics, gCurrentPid, g_metrics_offset);

      // Wait for the metrics capture thread to finish
      pollMetricsThread.join();
    }
  }

  pmStopStreaming(gCurrentPid);
  pmCloseSession();

  try {
    g_csv_file.close();
  } catch (...) {
    std::cout << "Unabled to close csv file" << std::endl;
  }

  return 0;
}
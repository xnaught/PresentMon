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
#include <conio.h>
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include "../PresentMonAPI2/source/Internal.h"
#include "CliOptions.h"

#include "../PresentMonAPIWrapper/source/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapper/source/QueryElement.h"
#include "DynamicQuerySample.h"
#include "FrameQuerySample.h"
#include "IntrospectionSample.h"

using namespace std::chrono;
using namespace std::chrono_literals;

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

bool CaseInsensitiveCompare(std::string str1, std::string str2) {
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
        if ((processName.has_value() && CaseInsensitiveCompare(process_info.szExeFile, processName.value())) ||
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

#define PM_BEGIN_DYNAMIC_QUERY(type) struct type : DynamicQueryContainer { using DynamicQueryContainer::DynamicQueryContainer;
#define PM_BEGIN_FRAME_QUERY(type) struct type : FrameQueryContainer<type> { using FrameQueryContainer<type>::FrameQueryContainer;
#define PM_END_QUERY private: FinalizingElement finalizer{ this }; }

int WrapperTest()
{
    using namespace std::chrono_literals;
    using namespace pmapi;

    try {
        auto& opt = clio::Options::Get();
        if (!opt.processId) {
            throw std::runtime_error{ "You need to specify a pid!" };
        }
        Session session;
        auto proc = session.TrackProcess(*opt.processId);

        if (opt.dynamic) {
            PM_BEGIN_DYNAMIC_QUERY(MyDynamicQuery)
                QueryElement fpsAvg{ this, PM_METRIC_DISPLAYED_FPS, PM_STAT_AVG };
                QueryElement fps99{ this, PM_METRIC_DISPLAYED_FPS, PM_STAT_PERCENTILE_99 };
                QueryElement gpuPower{ this, PM_METRIC_GPU_POWER, PM_STAT_PERCENTILE_99, 1 };
            PM_END_QUERY dq{ session, 1000., 1010., 1, 1 };

            while (!_kbhit()) {
                dq.Poll(proc);
                const double fps = dq.fpsAvg;
                const float pow = dq.gpuPower;
                std::cout << fps << ", " << dq.fpsAvg.As<int>()
                    << ", " << dq.fps99.As<double>()
                    << " | " << pow << std::endl;
                std::this_thread::sleep_for(20ms);
            }
        }
        else {
            PM_BEGIN_FRAME_QUERY(MyFrameQuery)
                QueryElement gpuDuration{ this, PM_METRIC_GPU_DURATION, PM_STAT_AVG };
                QueryElement gpuPower{ this, PM_METRIC_GPU_POWER, PM_STAT_AVG, 1 };
            PM_END_QUERY fq{ session, 20, 1 };

            while (!_kbhit()) {
                std::cout << "Polling...\n";
                const auto nProcessed = fq.ForEachConsume(proc, [](const MyFrameQuery& q) {
                    std::cout << q.gpuDuration.As<float>() << ", " << q.gpuPower.As<double>() << "\n";
                    });
                std::cout << "Processed " << nProcessed << " frames, sleeping..." << std::endl;
                std::this_thread::sleep_for(150ms);
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cout << "Unknown Error" << std::endl;
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

        //if (gQuit){
        //    break;
        //}
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
    if (opt.introspectionSample ^ opt.dynamicQuerySample ^ opt.frameQuerySample) {
        std::unique_ptr<pmapi::Session> pSession;
        if (opt.controlPipe) {
            pSession = std::make_unique<pmapi::Session>(*opt.controlPipe, *opt.introNsm);
        }
        else {
            pSession = std::make_unique<pmapi::Session>();
        }

        if (opt.introspectionSample) {
            return IntrospectionSample(std::move(pSession));
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
                if (opt.dynamicQuerySample) {
                    return DynamicQuerySample(std::move(pSession), processId.value(), *opt.windowSize, *opt.metricOffset);
                }
                else {
                    return FrameQuerySample(std::move(pSession), processName.value(), processId.value());
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
        OutputString("SampleClient supports one action at a time. Select one of:\n");
        OutputString("--view-available-metrics\n");
        OutputString("--poll-metrics [--process-id id | --process-name name.exe]\n");
        OutputString("--record-frames [--process-id id | --process-name name.exe]\n");
        return -1;
    }
}
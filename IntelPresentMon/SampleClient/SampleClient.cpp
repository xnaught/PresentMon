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
#include "Utils.h"
#include "DynamicQuerySample.h"
#include "FrameQuerySample.h"
#include "IntrospectionSample.h"
#include "CheckMetricSample.h"

using namespace std::chrono;
using namespace std::chrono_literals;


#define PM_BEGIN_DYNAMIC_QUERY(type) struct type : DynamicQueryContainer { using DynamicQueryContainer::DynamicQueryContainer;
#define PM_BEGIN_FRAME_QUERY(type) struct type : FrameQueryContainer<type> { using FrameQueryContainer<type>::FrameQueryContainer;
#define PM_END_QUERY private: FinalizingElement finalizer{ this }; }

int WrapperTest()
{
    using namespace std::chrono_literals;
    using namespace pmapi;

    try {
        std::optional<unsigned int> processId;
        std::optional<std::string> processName;

        GetProcessInformation(processName, processId);
        if (!processId.has_value())
        {
            if (!processId.has_value()) {
                throw std::runtime_error{ "You need to specify a pid!" };
            }
        }

        auto& opt = clio::Options::Get();

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

int main(int argc, char* argv[])
{
    try {
        if (auto e = clio::Options::Init(argc, argv)) {
            return *e;
        }
        auto& opt = clio::Options::Get();

        // if wrapper is specified immediately 
        if (opt.wrapper)
        {
            return WrapperTest();
        }

        // validate options, better to do this with CLI11 validation but framework needs upgrade...
        if (bool(opt.controlPipe) != bool(opt.introNsm)) {
            std::cout << "Must set both control pipe and intro NSM, or neither.\n";
            return -1;
        }

        // determine requested activity
        if (opt.introspectionSample ^ opt.dynamicQuerySample ^ opt.frameQuerySample ^ opt.checkMetricSample) {
            std::unique_ptr<pmapi::Session> pSession;
            if (opt.controlPipe) {
                pSession = std::make_unique<pmapi::Session>(*opt.controlPipe, *opt.introNsm);
            }
            else {
                pSession = std::make_unique<pmapi::Session>();
            }

            if (opt.introspectionSample)
            {
                return IntrospectionSample(std::move(pSession));
            }
            else if (opt.checkMetricSample)
            {
                return CheckMetricSample(std::move(pSession));
            }
            else if (opt.dynamicQuerySample)
            {
                return DynamicQuerySample(std::move(pSession), *opt.windowSize, *opt.metricOffset);
            }
            else
            {
                return FrameQuerySample(std::move(pSession));
            }
        }
        else {
            std::cout << "SampleClient supports one action at a time. Select one of:\n";
            std::cout << "--introspection-sample\n";
            std::cout << "--dynamic-query-sample [--process-id id | --process-name name.exe] [--add-gpu-metric]\n";
            std::cout << "--frame-query-sample [--process-id id | --process-name name.exe]  [--gen-csv]\n";
            std::cout << "--check-metric-sample --metric PM_METRIC_*\n";
            return -1;
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

}
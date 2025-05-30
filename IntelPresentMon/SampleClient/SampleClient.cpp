// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#define WIN32_LEAN_AND_MEAN
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
#include <boost/process.hpp>
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPI2/Internal.h"
#include "CliOptions.h"

#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapper/FixedQuery.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "Utils.h"
#include "DynamicQuerySample.h"
#include "FrameQuerySample.h"
#include "IntrospectionSample.h"
#include "CheckMetricSample.h"
#include "WrapperStaticQuery.h"
#include "MetricListSample.h"
#include "LogDemo.h"
#include "DiagnosticDemo.h"

#include "../CommonUtilities/IntervalWaiter.h"


int main(int argc, char* argv[])
{
    try {
        if (auto e = clio::Options::Init(argc, argv)) {
            return *e;
        }
        auto& opt = clio::Options::Get();

        // make sure we're using the debug dev version of the pmapi dll
        pmLoaderSetPathToMiddlewareDll_("PresentMonAPI2.dll");

        // launch the service, getting it to process an ETL gold file (custom object names)
        namespace bp = boost::process;
        using namespace std::literals;
        const auto pipeName = R"(\\.\pipe\pmsvc-ctl-pipe-tt)"s;
        bp::child svc{
            "PresentMonService.exe"s,
            "--timed-stop"s, "10000"s,
            "--control-pipe"s, pipeName,
            "--nsm-prefix"s, "pmon_nsm_tt_"s,
            "--intro-nsm"s, "svc-intro-tt"s,
            "--etw-session-name"s, "svc-sesh-tt"s,
            "--etl-test-file"s, R"(..\..\tests\gold\test_case_0.etl)"s,
        };

        // connect to the service with custom control pipe name
        std::this_thread::sleep_for(250ms);
        auto pApi = std::make_unique<pmapi::Session>(pipeName);

        // track the pid we know to be active in the ETL (10792 for gold1
        auto tracker = pApi->TrackProcess(10792);

        // setup fixed dynamic query for basic metrics (FPS presented)
        PM_BEGIN_FIXED_DYNAMIC_QUERY(MyDynamicQuery)
            pmapi::FixedQueryElement fpsAvg{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_AVG };
        PM_END_FIXED_QUERY query{ *pApi, 200., 50., 1, 1 };

        // output realtime samples to console at a steady interval
        pmon::util::IntervalWaiter waiter{ 0.1 };
        while (true) {
            query.Poll(tracker);
            std::cout << "FPS: " << query.fpsAvg.As<double>() << std::endl;
            waiter.Wait();
        }

        if (opt.logDemo) {
            RunLogDemo(*opt.logDemo);
            return 0;
        }
        if (opt.diagDemo) {
            RunDiagnosticDemo(*opt.diagDemo);
            return 0;
        }

        if (opt.middlewareDllPath) {
            pmLoaderSetPathToMiddlewareDll_(opt.middlewareDllPath->c_str());
        }

        // determine requested activity
        if (opt.introspectionSample ^ opt.dynamicQuerySample ^ opt.frameQuerySample ^ opt.checkMetricSample ^ opt.wrapperStaticQuerySample ^ opt.metricListSample) {
            std::unique_ptr<pmapi::Session> pSession;
            if (opt.controlPipe) {
                pSession = std::make_unique<pmapi::Session>(*opt.controlPipe);
            }
            else {
                pSession = std::make_unique<pmapi::Session>();
            }

            if (opt.introspectionSample) {
                return IntrospectionSample(std::move(pSession));
            }
            else if (opt.checkMetricSample) {
                return CheckMetricSample(std::move(pSession));
            }
            else if (opt.dynamicQuerySample) {
                return DynamicQuerySample(std::move(pSession), *opt.windowSize, *opt.metricOffset);
            }
            else if (opt.wrapperStaticQuerySample) {
                return WrapperStaticQuerySample(std::move(pSession));
            }
            else if (opt.metricListSample) {
                return MetricListSample(*pSession);
            }
            else {
                return FrameQuerySample(std::move(pSession));
            }
        }
        else {
            std::cout << "SampleClient supports one action at a time. For example:\n";
            std::cout << "--introspection-sample\n";
            std::cout << "--wrapper-static-query-sample\n";
            std::cout << "--dynamic-query-sample [--process-id id | --process-name name.exe] [--add-gpu-metric]\n";
            std::cout << "--frame-query-sample [--process-id id | --process-name name.exe]  [--gen-csv]\n";
            std::cout << "--check-metric-sample --metric PM_METRIC_*\n";
            std::cout << "Use --help to see the full list of commands and configuration options available\n";
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
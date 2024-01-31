#pragma once

#define PM_BEGIN_DYNAMIC_QUERY(type) struct type : DynamicQueryContainer { using DynamicQueryContainer::DynamicQueryContainer;
#define PM_BEGIN_FRAME_QUERY(type) struct type : FrameQueryContainer<type> { using FrameQueryContainer<type>::FrameQueryContainer;
#define PM_END_QUERY private: FinalizingElement finalizer{ this }; }

int DynamicQuerySample(std::string controlPipe, std::string introspectionNsm, unsigned int processId, double windowSize, double metricOffset)
{
    using namespace pmapi;

    std::unique_ptr<pmapi::Session> pSession;
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

    ProcessTracker processTracker;
    try {
        processTracker = pSession->TrackProcess(processId);

        PM_BEGIN_DYNAMIC_QUERY(MyDynamicQuery)
            QueryElement appName{ this, PM_METRIC_APPLICATION, PM_STAT_MID_POINT };
            QueryElement fpsAvg{ this, PM_METRIC_DISPLAYED_FPS, PM_STAT_AVG };
            QueryElement fps90{ this, PM_METRIC_DISPLAYED_FPS, PM_STAT_PERCENTILE_90 };
            QueryElement fps95{ this, PM_METRIC_DISPLAYED_FPS, PM_STAT_PERCENTILE_95 };
            QueryElement fps99{ this, PM_METRIC_DISPLAYED_FPS, PM_STAT_PERCENTILE_99 };
            QueryElement fpsMin{ this, PM_METRIC_DISPLAYED_FPS, PM_STAT_MAX };
            QueryElement fpsMax{ this, PM_METRIC_DISPLAYED_FPS, PM_STAT_MIN };
            QueryElement frameDurationAvg{ this, PM_METRIC_FRAME_DURATION, PM_STAT_AVG };
            QueryElement fpStallAvg{ this, PM_METRIC_CPU_FRAME_PACING_STALL, PM_STAT_AVG };
            QueryElement gpuDurationAvg{ this, PM_METRIC_GPU_DURATION, PM_STAT_AVG };
            QueryElement gpuBusyTimeAvg{ this, PM_METRIC_GPU_BUSY_TIME, PM_STAT_AVG };
            QueryElement gpuDisplayLatencyAvg{ this, PM_METRIC_DISPLAY_LATENCY, PM_STAT_AVG };
            QueryElement gpuDisplayDurationAvg{ this, PM_METRIC_DISPLAY_DURATION, PM_STAT_AVG };
            QueryElement gpuInputLatencyAvg{ this, PM_METRIC_INPUT_LATENCY, PM_STAT_AVG };
            QueryElement gpuPower{ this, PM_METRIC_GPU_POWER, PM_STAT_AVG, 1 };
        PM_END_QUERY dq{ *pSession, windowSize, metricOffset, 1, 1};

        if (InitializeConsole() == false) {
            std::cout << "\nFailed to initialize console.\n";
            return -1;
        }

        while (!_kbhit()) {
            dq.Poll(processTracker);
            ConsolePrintLn("Process Name = %s", dq.appName);
            ConsolePrintLn("Presented FPS Average = %f", dq.fpsAvg);
            ConsolePrintLn("Presented FPS 90% = %f", dq.fps90);
            ConsolePrintLn("Presented FPS 95% = %f", dq.fps95);
            ConsolePrintLn("Presented FPS 99% = %f", dq.fps99);
            ConsolePrintLn("Presented FPS Max = %f", dq.fpsMax);
            ConsolePrintLn("Presented FPS Min = %f", dq.fpsMin);
            ConsolePrintLn("Frame Duration Average = %f", dq.frameDurationAvg);
            ConsolePrintLn("Frame Pacing Stall Average = %f", dq.fpStallAvg);
            ConsolePrintLn("GPU Duration Average = %f", dq.gpuDisplayDurationAvg);
            ConsolePrintLn("GPU Busy Time Average = %f", dq.gpuBusyTimeAvg);
            ConsolePrintLn("Display Latency Average = %f", dq.gpuDisplayLatencyAvg);
            ConsolePrintLn("Display Duration Average = %f", dq.gpuDisplayDurationAvg);
            ConsolePrintLn("Input Latency Average = %f", dq.gpuInputLatencyAvg);
            ConsolePrintLn("GPU Power Average = %f", dq.gpuPower);
            CommitConsole();
            Sleep(20);
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

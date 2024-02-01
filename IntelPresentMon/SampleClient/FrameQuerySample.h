#pragma once

#define PM_BEGIN_DYNAMIC_QUERY(type) struct type : DynamicQueryContainer { using DynamicQueryContainer::DynamicQueryContainer;
#define PM_BEGIN_FRAME_QUERY(type) struct type : FrameQueryContainer<type> { using FrameQueryContainer<type>::FrameQueryContainer;
#define PM_END_QUERY private: FinalizingElement finalizer{ this }; }

int FrameQuerySample(std::unique_ptr<pmapi::Session>&& pSession, std::string processName, unsigned int processId)
{
    using namespace std::chrono_literals;
    using namespace pmapi;

    try {
        auto proc = pSession->TrackProcess(processId);

        PM_BEGIN_FRAME_QUERY(MyFrameQuery)
            QueryElement swapChain{ this, PM_METRIC_SWAP_CHAIN_ADDRESS, PM_STAT_NONE };
            QueryElement cpuFrameQpc{ this, PM_METRIC_CPU_FRAME_QPC, PM_STAT_NONE };
            QueryElement cpuDuration{ this, PM_METRIC_CPU_DURATION, PM_STAT_NONE };
            QueryElement cpuFpStall{ this, PM_METRIC_CPU_FRAME_PACING_STALL, PM_STAT_NONE };
            QueryElement gpuLatency{ this, PM_METRIC_GPU_LATENCY, PM_STAT_NONE };
            QueryElement gpuDuration{ this, PM_METRIC_GPU_DURATION, PM_STAT_NONE };
            QueryElement gpuBusyTime{ this, PM_METRIC_GPU_BUSY_TIME, PM_STAT_NONE };
            QueryElement gpuDisplayLatency{ this, PM_METRIC_DISPLAY_LATENCY, PM_STAT_NONE };
            QueryElement gpuDisplayDuration{ this, PM_METRIC_DISPLAY_DURATION, PM_STAT_NONE};
            QueryElement inputLatency{ this, PM_METRIC_DISPLAY_DURATION, PM_STAT_NONE };
            QueryElement gpuPower{ this, PM_METRIC_GPU_POWER, PM_STAT_NONE, 1 };
        PM_END_QUERY fq{ *pSession, 20, 1};

        while (!_kbhit()) {
            std::cout << "Consuming frames...";
            const auto nProcessed = fq.ForEachConsume(proc, [processName, processId](const MyFrameQuery& q) {
                std::cout << processName << ",";
                std::cout << processId << ",";
                std::cout << std::hex << "0x" << q.swapChain.As<uint64_t>() << std::dec << ",";
                std::cout << q.cpuFrameQpc.As<uint64_t>() << ",";
                std::cout << q.cpuDuration.As<double>() << ",";
                std::cout << q.cpuFpStall.As<double>() << ",";
                std::cout << q.gpuLatency.As<double>() << ",";
                std::cout << q.gpuDuration.As<double>() << ",";
                std::cout << q.gpuBusyTime.As<double>() << ",";
                std::cout << q.gpuDisplayLatency.As<double>() << ",";
                std::cout << q.gpuDisplayDuration.As<double>() << ",";
                std::cout << q.inputLatency.As<double>() << ",";
                std::cout << q.gpuPower.As<double>() << "\n";
                });
            std::cout << "Processed " << nProcessed << " frames, sleeping..." << std::endl;
            std::this_thread::sleep_for(150ms);
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

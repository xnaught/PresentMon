#include "ConcreteMiddleware.h"
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdlib>
#include "../../PresentMonUtils/NamedPipeHelper.h"
#include "../../PresentMonUtils/QPCUtils.h"
#include "../../PresentMonAPI2/source/Internal.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
// TODO: don't need transfer if we can somehow get the PM_ struct generation working without inheritance
// needed right now because even if we forward declare, we don't have the inheritance info
#include "../../Interprocess/source/IntrospectionTransfer.h"
#include "../../Interprocess/source/IntrospectionHelpers.h"
#include "../../Interprocess/source/IntrospectionCloneAllocators.h"
//#include "MockCommon.h"
#include "DynamicQuery.h"
#include "../../ControlLib/PresentMonPowerTelemetry.h"
#include "../../ControlLib/CpuTelemetryInfo.h"
#include "../../PresentMonService/GlobalIdentifiers.h"

namespace pmon::mid
{
    using namespace ipc::intro;

    static const uint32_t kMaxRespBufferSize = 4096;
	static const uint64_t kClientFrameDeltaQPCThreshold = 50000000;
	ConcreteMiddleware::ConcreteMiddleware(std::optional<std::string> pipeNameOverride, std::optional<std::string> introNsmOverride)
	{
        const auto pipeName = pipeNameOverride.transform(&std::string::c_str)
            .value_or(pmon::gid::defaultControlPipeName);

        HANDLE namedPipeHandle;
        // Try to open a named pipe; wait for it, if necessary.
        while (1) {
            namedPipeHandle = CreateFileA(
                pipeName,
                GENERIC_READ | GENERIC_WRITE,
                0,              
                NULL,           
                OPEN_EXISTING,  
                0,              
                NULL);          

            // Break if the pipe handle is valid.
            if (namedPipeHandle != INVALID_HANDLE_VALUE) {
                break;
            }

            // Exit if an error other than ERROR_PIPE_BUSY occurs.
            if (const auto hr = GetLastError(); hr != ERROR_PIPE_BUSY) {
                throw std::runtime_error{ "Service not found" };
            }

            // All pipe instances are busy, so wait for 20 seconds.
            if (!WaitNamedPipeA(pipeName, 20000)) {
                throw std::runtime_error{ "Pipe sessions full" };
            }
        }
        // The pipe connected; change to message-read mode.
        DWORD mode = PIPE_READMODE_MESSAGE;
        BOOL success = SetNamedPipeHandleState(namedPipeHandle,
            &mode,
            NULL,
            NULL);
        if (!success) {
            throw std::runtime_error{ "Pipe error" };
        }
        pNamedPipeHandle.reset(namedPipeHandle);
        clientProcessId = GetCurrentProcessId();
        // connect to the introspection nsm
        pComms = ipc::MakeMiddlewareComms(std::move(introNsmOverride));

        // Get the introspection data
        pmapi::intro::Dataset ispec{ GetIntrospectionData(), [this](auto p) {FreeIntrospectionData(p); } };
        
        uint32_t gpuAdapterId = 0;
        auto deviceView = ispec.GetDevices();
        for (auto dev : deviceView)
        {
            if (dev.GetBasePtr()->type == PM_DEVICE_TYPE_GRAPHICS_ADAPTER)
            {
                cachedGpuInfo.push_back({ dev.GetBasePtr()->vendor, dev.GetName(), dev.GetId(), gpuAdapterId });
                gpuAdapterId++;
            }
        }

        GetCpuInfo();
	}
    
    const PM_INTROSPECTION_ROOT* ConcreteMiddleware::GetIntrospectionData()
    {
        return pComms->GetIntrospectionRoot();
    }

    void ConcreteMiddleware::FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot)
    {
        free(const_cast<PM_INTROSPECTION_ROOT*>(pRoot));
    }

	void ConcreteMiddleware::Speak(char* buffer) const
	{
		strcpy_s(buffer, 256, "concrete-middle");
	}

    PM_STATUS ConcreteMiddleware::SendRequest(MemBuffer* requestBuffer) {
        DWORD bytesWritten;
        BOOL success = WriteFile(
            pNamedPipeHandle.get(),
            requestBuffer->AccessMem(),
            static_cast<DWORD>(requestBuffer->GetCurrentSize()),
            &bytesWritten,
            NULL);

        if (success && requestBuffer->GetCurrentSize() == bytesWritten) {
            return PM_STATUS::PM_STATUS_SUCCESS;
        }
        else {
            return PM_STATUS::PM_STATUS_FAILURE;
        }
    }

    PM_STATUS ConcreteMiddleware::ReadResponse(MemBuffer* responseBuffer) {
        BOOL success;
        DWORD bytesRead;
        BYTE inBuffer[kMaxRespBufferSize];
        ZeroMemory(&inBuffer, sizeof(inBuffer));

        do {
            // Read from the pipe using a nonoverlapped read
            success = ReadFile(pNamedPipeHandle.get(),
                inBuffer,
                sizeof(inBuffer),
                &bytesRead,
                NULL);

            // If the call was not successful AND there was
            // no more data to read bail out
            if (!success && GetLastError() != ERROR_MORE_DATA) {
                break;
            }

            // Either the call was successful or there was more
            // data in the pipe. In both cases add the response data
            // to the memory buffer
            responseBuffer->AddItem(inBuffer, bytesRead);
        } while (!success);  // repeat loop if ERROR_MORE_DATA

        if (success) {
            return PM_STATUS::PM_STATUS_SUCCESS;
        }
        else {
            return PM_STATUS::PM_STATUS_FAILURE;
        }
    }

    PM_STATUS ConcreteMiddleware::CallPmService(MemBuffer* requestBuffer, MemBuffer* responseBuffer)
    {
        PM_STATUS status;

        status = SendRequest(requestBuffer);
        if (status != PM_STATUS::PM_STATUS_SUCCESS) {
            return status;
        }

        status = ReadResponse(responseBuffer);
        if (status != PM_STATUS::PM_STATUS_SUCCESS) {
            return status;
        }

        return status;
    }

    PM_STATUS ConcreteMiddleware::StartStreaming(uint32_t processId)
    {
        MemBuffer requestBuffer;
        MemBuffer responseBuffer;

        NamedPipeHelper::EncodeStartStreamingRequest(&requestBuffer, clientProcessId,
            processId, nullptr);

        PM_STATUS status = CallPmService(&requestBuffer, &responseBuffer);
        if (status != PM_STATUS::PM_STATUS_SUCCESS) {
            return status;
        }

        IPMSMStartStreamResponse startStreamResponse{};

        status = NamedPipeHelper::DecodeStartStreamingResponse(
            &responseBuffer, &startStreamResponse);
        if (status != PM_STATUS::PM_STATUS_SUCCESS) {
            return status;
        }

        // Get the NSM file name from 
        std::string mapFileName(startStreamResponse.fileName);

        // Initialize client with returned mapfile name
        auto iter = presentMonStreamClients.find(processId);
        if (iter == presentMonStreamClients.end()) {
            try {
                std::unique_ptr<StreamClient> client =
                    std::make_unique<StreamClient>(std::move(mapFileName), false);
                presentMonStreamClients.emplace(processId, std::move(client));
            }
            catch (...) {
                return PM_STATUS::PM_STATUS_FAILURE;
            }
        }

        return PM_STATUS_SUCCESS;
    }
    
    PM_STATUS ConcreteMiddleware::StopStreaming(uint32_t processId)
    {
        MemBuffer requestBuffer;
        MemBuffer responseBuffer;

        NamedPipeHelper::EncodeStopStreamingRequest(&requestBuffer,
            clientProcessId,
            processId);

        PM_STATUS status = CallPmService(&requestBuffer, &responseBuffer);
        if (status != PM_STATUS::PM_STATUS_SUCCESS) {
            return status;
        }

        status = NamedPipeHelper::DecodeStopStreamingResponse(&responseBuffer);
        if (status != PM_STATUS::PM_STATUS_SUCCESS) {
            return status;
        }

        // Remove client
        auto iter = presentMonStreamClients.find(processId);
        if (iter != presentMonStreamClients.end()) {
            presentMonStreamClients.erase(std::move(iter));
        }

        return status;
    }

    void ConcreteMiddleware::GetCpuInfo()
    {
        MemBuffer requestBuffer;
        MemBuffer responseBuffer;

        NamedPipeHelper::EncodeRequestHeader(&requestBuffer, PM_ACTION::GET_CPU_NAME);

        PM_STATUS status = CallPmService(&requestBuffer, &responseBuffer);
        if (status != PM_STATUS::PM_STATUS_SUCCESS) {
            return;
        }

        IPMCpuNameResponse cpu_name{};
        status = NamedPipeHelper::DecodeCpuNameResponse(&responseBuffer, &cpu_name);
        if (status != PM_STATUS::PM_STATUS_SUCCESS ||
            cpu_name.cpu_name_length > MAX_PM_CPU_NAME) {
            return;
        }

        auto ContainsString = [](std::string str, std::string subStr)
            {
                return std::search(str.begin(), str.end(), subStr.begin(), subStr.end(),
                    [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); }) != str.end();
            };

        std::string cpuName = cpu_name.cpu_name;
        PM_DEVICE_VENDOR deviceVendor;
        if (ContainsString(cpuName, "intel"))
        {
            deviceVendor = PM_DEVICE_VENDOR_INTEL;
        }
        else if (ContainsString(cpuName, "amd"))
        {
            deviceVendor = PM_DEVICE_VENDOR_AMD;
        }
        else
        {
            deviceVendor = PM_DEVICE_VENDOR_UNKNOWN;
        }
        cachedCpuInfo.push_back({ deviceVendor, cpuName });
    }

    PM_DYNAMIC_QUERY* ConcreteMiddleware::RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, double windowSizeMs, double metricOffsetMs)
    { 
        // get introspection data for reference
        // TODO: cache this data so it's not required to be generated every time
        pmapi::intro::Dataset ispec{ GetIntrospectionData(), [this](auto p) {FreeIntrospectionData(p); } };

        // make the query object that will be managed by the handle
        auto pQuery = std::make_unique<PM_DYNAMIC_QUERY>();
        std::optional<uint32_t> cachedGpuInfoIndex;

        uint64_t offset = 0u;
        for (auto& qe : queryElements)
        {
            // A device of zero is NOT a graphics adapter.
            if (qe.deviceId != 0)
            {
                // If we have already set a device id in this query, check to
                // see if it's the same device id as previously set. Currently
                // we don't support querying multiple gpu devices in the one
                // query
                if (cachedGpuInfoIndex.has_value())
                {
                    if (cachedGpuInfo[cachedGpuInfoIndex.value()].deviceId != qe.deviceId)
                    {
                        throw std::runtime_error{ "Multiple GPU devices not allowed in single query" };
                    }
                }
                else
                {
                    // Go through the cached Gpus and see which device the client
                    // wants
                    for (int i = 0; i < cachedGpuInfo.size(); i++)
                    {
                        if (qe.deviceId == cachedGpuInfo[i].deviceId)
                        {
                            cachedGpuInfoIndex = i;
                            break;
                        }
                    }
                }
            }

            auto metricView = ispec.FindMetric(qe.metric);
            switch (qe.metric) {
            case PM_METRIC_PRESENTED_FPS:
            case PM_METRIC_DISPLAYED_FPS:
            case PM_METRIC_FRAME_TIME:
            case PM_METRIC_GPU_BUSY_TIME:
            case PM_METRIC_CPU_BUSY_TIME:
            case PM_METRIC_CPU_WAIT_TIME:
            case PM_METRIC_DISPLAY_BUSY_TIME:
            case PM_METRIC_PROCESS_NAME:
                pQuery->accumFpsData = true;
                break;
            case PM_METRIC_GPU_POWER:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_power));
                break;
            case PM_METRIC_GPU_SUSTAINED_POWER_LIMIT:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_sustained_power_limit));
                break;
            case PM_METRIC_GPU_VOLTAGE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_voltage));
                break;
            case PM_METRIC_GPU_FREQUENCY:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_frequency));
                break;
            case PM_METRIC_GPU_TEMPERATURE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_temperature));
                break;
            case PM_METRIC_GPU_UTILIZATION:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_utilization));
                break;
            case PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_render_compute_utilization));
                break;
            case PM_METRIC_GPU_MEDIA_UTILIZATION:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_media_utilization));
                break;
            case PM_METRIC_VRAM_POWER:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_power));
                break;
            case PM_METRIC_VRAM_VOLTAGE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_voltage));
                break;
            case PM_METRIC_VRAM_FREQUENCY:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_frequency));
                break;
            case PM_METRIC_VRAM_EFFECTIVE_FREQUENCY:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_effective_frequency));
                break;
            case PM_METRIC_VRAM_TEMPERATURE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_temperature));
                break;
            case PM_METRIC_GPU_MEM_SIZE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_size));
                break;
            case PM_METRIC_GPU_MEM_USED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_used));
                break;
            case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_max_bandwidth));
                break;
            case PM_METRIC_GPU_MEM_WRITE_BANDWIDTH:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_write_bandwidth));
                break;
            case PM_METRIC_GPU_MEM_READ_BANDWIDTH:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_read_bandwidth));
                break;
            case PM_METRIC_GPU_POWER_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_power_limited));
                break;
            case PM_METRIC_GPU_TEMPERATURE_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_temperature_limited));
                break;
            case PM_METRIC_GPU_CURRENT_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_current_limited));
                break;
            case PM_METRIC_GPU_VOLTAGE_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_voltage_limited));
                break;
            case PM_METRIC_GPU_UTILIZATION_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_utilization_limited));
                break;
            case PM_METRIC_VRAM_POWER_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_power_limited));
                break;
            case PM_METRIC_VRAM_TEMPERATURE_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_temperature_limited));
                break;
            case PM_METRIC_VRAM_CURRENT_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_current_limited));
                break;
            case PM_METRIC_VRAM_VOLTAGE_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_voltage_limited));
                break;
            case PM_METRIC_VRAM_UTILIZATION_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_utilization_limited));
                break;
            case PM_METRIC_GPU_FAN_SPEED:
                switch (qe.arrayIndex)
                {
                case 0:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_0));
                    break;
                case 1:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_1));
                    break;
                case 2:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_2));
                    break;
                case 3:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_3));
                    break;
                case 4:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_4));
                    break;
                }
                break;
            case PM_METRIC_CPU_UTILIZATION:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_utilization));
                break;
            case PM_METRIC_CPU_POWER:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_power));
                break;
            case PM_METRIC_CPU_POWER_LIMIT:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_power_limit));
                break;
            case PM_METRIC_CPU_TEMPERATURE:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_temperature));
                break;
            case PM_METRIC_CPU_FREQUENCY:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_frequency));
                break;
            case PM_METRIC_CPU_CORE_UTILITY:
                //pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_power));
                break;
            default:
                break;
            }

            auto result = pQuery->compiledMetrics.emplace(qe.metric, CompiledStats());
            auto stats = &result.first->second;
            switch (qe.stat)
            {
            case PM_STAT_AVG:
                stats->calcAvg = true;
                break;
            case PM_STAT_PERCENTILE_99:
                stats->calcPercentile99 = true;
                break;
            case PM_STAT_PERCENTILE_95:
                stats->calcPercentile95 = true;
                break;
            case PM_STAT_PERCENTILE_90:
                stats->calcPercentile90 = true;
                break;
            case PM_STAT_MAX:
                stats->calcMax = true;
                break;
            case PM_STAT_MIN:
                stats->calcMin = true;
                break;
            case PM_STAT_RAW:
                stats->calcRaw = true;
                break;
            default:
                // Invalid stat enum
                throw std::runtime_error{ "Invalid stat enum" };
            }
            qe.dataOffset = offset;
            qe.dataSize = GetDataTypeSize(metricView.GetDataTypeInfo().GetBasePtr()->type);
            offset += qe.dataSize;
        }

        pQuery->dynamicQueryHandle = pQuery.get();
        pQuery->metricOffsetMs = metricOffsetMs;
        pQuery->windowSizeMs = windowSizeMs;
        pQuery->elements = std::vector<PM_QUERY_ELEMENT>{ queryElements.begin(), queryElements.end() };
        pQuery->queryCacheSize = pQuery->elements[std::size(pQuery->elements) - 1].dataOffset + pQuery->elements[std::size(pQuery->elements) - 1].dataSize;
        if (cachedGpuInfoIndex.has_value())
        {
            pQuery->cachedGpuInfoIndex = cachedGpuInfoIndex.value();
        }

        return pQuery.release();
    }

    void ConcreteMiddleware::PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains)
    {
        std::unordered_map<uint64_t, fpsSwapChainData> swapChainData;
        std::unordered_map<PM_METRIC, std::vector<double>> gpucpuMetricData;
        bool allMetricsCalculated = false;
        bool fpsMetricsCalculated = false;

        if (*numSwapChains == 0) {
            return;
        }

        if (pQuery->cachedGpuInfoIndex.has_value())
        {
            if (pQuery->cachedGpuInfoIndex.value() != currentGpuInfoIndex)
            {
                // Set the adapter id 
                SetActiveGraphicsAdapter(cachedGpuInfo[pQuery->cachedGpuInfoIndex.value()].adapterId.value());
                // Set the current index to the queried one
                currentGpuInfoIndex = pQuery->cachedGpuInfoIndex.value();
            }
        }

        auto iter = presentMonStreamClients.find(processId);
        if (iter == presentMonStreamClients.end()) {
            return;
        }

        // Get the named shared memory associated with the stream client
        StreamClient* client = iter->second.get();
        auto nsm_view = client->GetNamedSharedMemView();
        auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            // TODO: Do we want to inform the client if the server has destroyed the
            // named shared memory?
            // Server destroyed the named shared memory due to process exit. Destroy the
            // mapped view from client side.
            //StopStreamProcess(process_id);
            //return PM_STATUS::PM_STATUS_PROCESS_NOT_EXIST;
            return;
        }

        uint64_t index = 0;
        double adjusted_window_size_in_ms = pQuery->windowSizeMs;
        auto result = queryFrameDataDeltas.emplace(std::pair(std::pair(pQuery->dynamicQueryHandle, processId), uint64_t()));
        auto queryToFrameDataDelta = &result.first->second;
        
        PmNsmFrameData* frame_data = GetFrameDataStart(client, index, SecondsDeltaToQpc(pQuery->metricOffsetMs/1000., client->GetQpcFrequency()), *queryToFrameDataDelta, adjusted_window_size_in_ms);
        if (frame_data == nullptr) {
            CopyMetricCacheToBlob(pQuery, processId, pBlob);
            return;
        }

        // Calculate the end qpc based on the current frame's qpc and
        // requested window size coverted to a qpc
        uint64_t end_qpc =
            frame_data->present_event.PresentStartTime -
            SecondsDeltaToQpc(adjusted_window_size_in_ms/1000., client->GetQpcFrequency());

        // These are only used for logging:
        uint64_t last_checked_qpc = frame_data->present_event.PresentStartTime;
        bool decrement_failed = false;
        bool read_frame_failed = false;

        // Loop from the most recent frame data until we either run out of data or
        // we meet the window size requirements sent in by the client
        while (frame_data->present_event.PresentStartTime > end_qpc) {
            if (pQuery->accumFpsData)
            {
                auto result = swapChainData.emplace(
                    frame_data->present_event.SwapChainAddress, fpsSwapChainData());
                auto swap_chain = &result.first->second;

                // Save off the application name
                swap_chain->applicationName = frame_data->present_event.application;
                // Copy swap_chain data for the previous first frame needed for calculations below
                auto nextFramePresentStartTime = swap_chain->present_start_0;
                auto nextFramePresentStopTime = swap_chain->present_stop_0;
                auto nextFrameGPUDuration = swap_chain->gpu_duration_0;

                // Save current frame's properties into swap_chain (the new first frame)
                swap_chain->displayed_0 = frame_data->present_event.FinalState == PresentResult::Presented;
                swap_chain->present_start_0 = frame_data->present_event.PresentStartTime;
                swap_chain->present_stop_0 = frame_data->present_event.PresentStopTime;
                swap_chain->gpu_duration_0 = frame_data->present_event.GPUDuration;
                swap_chain->num_presents += 1;

                if (swap_chain->displayed_0) {
                    swap_chain->display_1_screen_time = swap_chain->display_0_screen_time;
                    swap_chain->display_0_screen_time = frame_data->present_event.ScreenTime;
                    swap_chain->display_count += 1;
                    if (swap_chain->display_count == 1) {
                        swap_chain->display_n_screen_time = frame_data->present_event.ScreenTime;
                    }
                }

                // These are only saved for the last frame:
                if (swap_chain->num_presents == 1) {
                    swap_chain->present_start_n = frame_data->present_event.PresentStartTime;
                    swap_chain->sync_interval = frame_data->present_event.SyncInterval;
                    //swap_chain->present_mode = TranslatePresentMode(frame_data->present_event.PresentMode);
                    swap_chain->allows_tearing = static_cast<int32_t>(frame_data->present_event.SupportsTearing);
                }

                // Compute metrics for this frame if we've seen enough subsequent frames to have all the
                // required data
                //
                // frame_data:      PresentStart--PresentStop--GPUDuration--ScreenTime
                // nextFrame:                                   PresentStart--PresentStop--GPUDuration--ScreenTime
                // nextNextFrame:                                                           PresentStart--PresentStop--GPUDuration--ScreenTime
                //                                CPUStart
                //                                CPUBusy------>CPUWait----------------->  GPUBusy--->  DisplayBusy---------------->
                if (swap_chain->num_presents > 1) {
                    auto cpuStart = frame_data->present_event.PresentStopTime;
                    auto cpuBusy = nextFramePresentStartTime - cpuStart;
                    auto cpuWait = nextFramePresentStopTime - nextFramePresentStartTime;
                    auto gpuBusy = nextFrameGPUDuration;
                    auto displayBusy = swap_chain->display_1_screen_time - swap_chain->display_0_screen_time;

                    auto frameTime_ms = QpcDeltaToMs(cpuBusy + cpuWait, client->GetQpcFrequency());
                    auto gpuBusy_ms = QpcDeltaToMs(gpuBusy, client->GetQpcFrequency());
                    auto displayBusy_ms = QpcDeltaToMs(displayBusy, client->GetQpcFrequency());
                    auto cpuBusy_ms = QpcDeltaToMs(cpuBusy, client->GetQpcFrequency());
                    auto cpuWait_ms = QpcDeltaToMs(cpuWait, client->GetQpcFrequency());

                    swap_chain->frame_times_ms.push_back(frameTime_ms);
                    swap_chain->gpu_sum_ms.push_back(gpuBusy_ms);
                    swap_chain->cpu_busy_ms.push_back(cpuBusy_ms);
                    swap_chain->cpu_wait_ms.push_back(cpuWait_ms);
                    swap_chain->display_busy_ms.push_back(displayBusy_ms);
                    swap_chain->dropped.push_back(swap_chain->displayed_0 ? 0. : 1.);

                    if (swap_chain->displayed_0 && swap_chain->display_count >= 2 && displayBusy > 0) {
                        swap_chain->displayed_fps.push_back(1000. / displayBusy_ms);
                    }
                }
            }

            for (size_t i = 0; i < pQuery->accumGpuBits.size(); ++i) {
                if (pQuery->accumGpuBits[i])
                {
                    PM_METRIC gpuMetric;
                    double gpuMetricValue;
                    if (GetGpuMetricData(i, frame_data->power_telemetry, gpuMetric, gpuMetricValue))
                    {
                        if (gpuMetric == PM_METRIC_GPU_MEM_MAX_BANDWIDTH)
                        {
                            cachedGpuMemMaxBandwidth = gpuMetricValue;
                        }
                        else if (gpuMetric == PM_METRIC_GPU_MEM_SIZE)
                        {
                            cachedGpuMemSize = gpuMetricValue;
                        }
                        else
                        {
                            auto result = gpucpuMetricData.emplace(gpuMetric, std::vector<double>());
                            auto data = &result.first->second;
                            data->push_back(gpuMetricValue);
                        }
                    }
                }
            }

            for (size_t i = 0; i < pQuery->accumCpuBits.size(); ++i) {
                if (pQuery->accumCpuBits[i])
                {
                    PM_METRIC cpuMetric;
                    double cpuMetricValue;
                    if (GetCpuMetricData(i, frame_data->cpu_telemetry, cpuMetric, cpuMetricValue))
                    {
                        auto result = gpucpuMetricData.emplace(cpuMetric, std::vector<double>());
                        auto data = &result.first->second;
                        data->push_back(cpuMetricValue);
                    }
                }
            }

            // Get the index of the next frame
            if (DecrementIndex(nsm_view, index) == false) {
                // We have run out of data to process, time to go
                decrement_failed = true;
                break;
            }
            frame_data = client->ReadFrameByIdx(index);
            if (frame_data == nullptr) {
                read_frame_failed = true;
                break;
            }
        }

        CalculateMetrics(pQuery, processId, pBlob, numSwapChains, client->GetQpcFrequency(), swapChainData, gpucpuMetricData);
    }

    void ConcreteMiddleware::PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob)
    {
        pmapi::intro::Dataset ispec{ GetIntrospectionData(), [this](auto p) {FreeIntrospectionData(p); } };
        auto metricView = ispec.FindMetric(element.metric);
        if (metricView.GetType().GetValue() != int(PM_METRIC_TYPE_STATIC)) {
            throw std::runtime_error{ "dynamic metric in static query poll" };
        }

        auto elementSize = GetDataTypeSize(metricView.GetDataTypeInfo().GetBasePtr()->type);

        switch (element.metric)
        {
        case PM_METRIC_CPU_NAME:
            strcpy_s(reinterpret_cast<char*>(pBlob), elementSize, cachedCpuInfo[0].deviceName.c_str());
            break;
        case PM_METRIC_GPU_NAME:
            strcpy_s(reinterpret_cast<char*>(pBlob), elementSize, cachedGpuInfo[element.deviceId].deviceName.c_str());
            break;
        case PM_METRIC_CPU_VENDOR:
        {
            auto& output = reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[0]);
            output = cachedCpuInfo[0].deviceVendor;
        }
            break;
        case PM_METRIC_GPU_VENDOR:
        {
            auto& output = reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[0]);
            output = cachedGpuInfo[element.deviceId].deviceVendor;
        }
            break;
        case PM_METRIC_PROCESS_NAME:
        case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
        case PM_METRIC_GPU_MEM_SIZE:
        {
            // Check to stream client associated with the process id saved in the dynamic query
            auto iter = presentMonStreamClients.find(processId);
            if (iter == presentMonStreamClients.end()) {
                return;
            }

            // Get the named shared memory associated with the stream client
            StreamClient* client = iter->second.get();
            auto nsm_view = client->GetNamedSharedMemView();
            auto nsm_hdr = nsm_view->GetHeader();
            if (!nsm_hdr->process_active) {
                return;
            }

            PmNsmFrameData* frameData = client->ReadFrameByIdx(client->GetLatestFrameIndex());
            if (frameData == nullptr) {
                return;
            }
            if (element.metric == PM_METRIC_PROCESS_NAME)
            {
                strcpy_s(reinterpret_cast<char*>(pBlob), elementSize, frameData->present_event.application);
            }
            else if (element.metric == PM_METRIC_GPU_MEM_MAX_BANDWIDTH)
            {
                auto& output = reinterpret_cast<double&>(pBlob[0]);
                output = static_cast<double>(frameData->power_telemetry.gpu_mem_max_bandwidth_bps);
            }
            else if (element.metric == PM_METRIC_GPU_MEM_SIZE)
            {
                auto& output = reinterpret_cast<double&>(pBlob[0]);
                output = static_cast<double>(frameData->power_telemetry.gpu_mem_total_size_b);
                int i = 0;
                i++;
            }
        }
            break;
        default:
            throw std::runtime_error{ "unknown metric in static poll" };
        }
        return;
    }

    void ConcreteMiddleware::CalculateFpsMetric(fpsSwapChainData& swapChain, const PM_QUERY_ELEMENT& element, uint8_t* pBlob, LARGE_INTEGER qpcFrequency)
    {
        auto MillisecondsToFPS = [](double ms) { return ms == 0. ? 0. : 1000. / ms; };
        auto& output = reinterpret_cast<double&>(pBlob[element.dataOffset]);

        if (element.stat == PM_STAT_AVG) {
            // We handle the averages for presented fps, frame times and displayed fps metrics
            // by using the first and last frame data for the specified window. If not this combination
            // metric and stat fall through
            if (element.metric == PM_METRIC_PRESENTED_FPS || element.metric == PM_METRIC_FRAME_TIME)
            {
                if (swapChain.num_presents > 1)
                {
                    output = QpcDeltaToMs(swapChain.present_start_n - swapChain.present_start_0, qpcFrequency);
                    output = output / (swapChain.num_presents - 1);
                    if (element.metric == PM_METRIC_PRESENTED_FPS)
                    {
                        output = MillisecondsToFPS(output);
                    }
                }
                else
                {
                    output = 0.;
                }
                return;
            }
            else if (element.metric == PM_METRIC_DISPLAYED_FPS)
            {
                if (swapChain.display_count > 1)
                {
                    output = QpcDeltaToMs(swapChain.display_n_screen_time - swapChain.display_0_screen_time, qpcFrequency);
                    output = MillisecondsToFPS(output / (swapChain.display_count - 1));
                }
                else
                {
                    output = 0.;
                }
                return;
            }
        }

        switch (element.metric)
        {
        case PM_METRIC_PRESENTED_FPS:
        case PM_METRIC_FRAME_TIME:
            CalculateMetric(output, swapChain.frame_times_ms, element.stat, false);
            if (element.metric == PM_METRIC_PRESENTED_FPS) {
                output = MillisecondsToFPS(output);
            }
            break;
        case PM_METRIC_DISPLAYED_FPS:
            CalculateMetric(output, swapChain.displayed_fps, element.stat);
            break;
        case PM_METRIC_GPU_BUSY_TIME:
            CalculateMetric(output, swapChain.gpu_sum_ms, element.stat);
            break;
        case PM_METRIC_CPU_BUSY_TIME:
            CalculateMetric(output, swapChain.cpu_busy_ms, element.stat);
            break;
        case PM_METRIC_CPU_WAIT_TIME:
            CalculateMetric(output, swapChain.cpu_wait_ms, element.stat);
            break;
        case PM_METRIC_DISPLAY_BUSY_TIME:
            CalculateMetric(output, swapChain.display_busy_ms, element.stat);
            break;
        default:
            output = 0.;
            break;
        }
        return;
    }

    void ConcreteMiddleware::CalculateGpuCpuMetric(std::unordered_map<PM_METRIC, std::vector<double>>& metricData, const PM_QUERY_ELEMENT& element, uint8_t* pBlob)
    {
        auto& output = reinterpret_cast<double&>(pBlob[element.dataOffset]);
        output = 0.;

        auto it = metricData.find(element.metric);
        if (it != metricData.end())
        {
            CalculateMetric(output, it->second, element.stat);
        }
        return;
    }

    void ConcreteMiddleware::CalculateMetric(double& pBlob, std::vector<double>& inData, PM_STAT stat, bool ascending)
    {
        auto& output = reinterpret_cast<double&>(pBlob);
        output = 0.;
        if (inData.size() > 1) {
            if (stat == PM_STAT_AVG)
            {
                for (auto& element : inData) {
                    output += element;
                }
                output /= inData.size();
                return;
            }
            if (stat == PM_STAT_RAW)
            {
                size_t middle_index = inData.size() / 2;
                output = inData[middle_index];
                return;
            }
            if (ascending) {
                std::sort(inData.begin(), inData.end());
                switch (stat)
                {
                case PM_STAT_MIN:
                    output = inData[0];
                    break;
                case PM_STAT_MAX:
                    output = inData[inData.size() - 1];
                    break;
                case PM_STAT_PERCENTILE_99:
                    output = GetPercentile(inData, 0.01);
                    break;
                case PM_STAT_PERCENTILE_95:
                    output = GetPercentile(inData, 0.05);
                    break;
                case PM_STAT_PERCENTILE_90:
                    output = GetPercentile(inData, 0.10);
                    break;
                default:
                    break;
                }
            }
            else {
                std::sort(inData.begin(), inData.end(), std::greater<>());
                switch (stat)
                {
                case PM_STAT_MIN:
                    output = inData[0];
                    break;
                case PM_STAT_MAX:
                    output = inData[inData.size() - 1];
                    break;
                case PM_STAT_PERCENTILE_99:
                    output = GetPercentile(inData, 0.01);
                    break;
                case PM_STAT_PERCENTILE_95:
                    output = GetPercentile(inData, 0.05);
                    break;
                case PM_STAT_PERCENTILE_90:
                    output = GetPercentile(inData, 0.10);
                    break;
                default:
                    break;
                }
            }
        }
        else if (inData.size() == 1) {
            output = inData[0];
        }
    }

    // Calculate percentile using linear interpolation between the closet ranks
    // data must be pre-sorted
    double ConcreteMiddleware::GetPercentile(std::vector<double>& data, double percentile)
    {
        percentile = max(percentile, 0.);

        double integral_part_as_double;
        double fractpart =
            modf(percentile * static_cast<double>(data.size()),
                &integral_part_as_double);

        uint32_t idx = static_cast<uint32_t>(integral_part_as_double);
        if (idx >= data.size() - 1) {
            return data[data.size() - 1];
        }

        return data[idx] + (fractpart * (data[idx + 1] - data[idx]));
    }

    PmNsmFrameData* ConcreteMiddleware::GetFrameDataStart(StreamClient* client, uint64_t& index, uint64_t queryMetricsDataOffset, uint64_t& queryFrameDataDelta, double& window_sample_size_in_ms)
    {

        PmNsmFrameData* frame_data = nullptr;
        index = 0;
        if (client == nullptr) {
            return nullptr;
        }

        auto nsm_view = client->GetNamedSharedMemView();
        auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            return nullptr;
        }

        index = client->GetLatestFrameIndex();
        frame_data = client->ReadFrameByIdx(index);
        if (frame_data == nullptr) {
            index = 0;
            return nullptr;
        }

        if (queryMetricsDataOffset == 0) {
            // Client has not specified a metric offset. Return back the most
            // most recent frame data
            return frame_data;
        }

        LARGE_INTEGER client_qpc = {};
        QueryPerformanceCounter(&client_qpc);
        uint64_t adjusted_qpc = GetAdjustedQpc(
            client_qpc.QuadPart, frame_data->present_event.PresentStartTime,
            queryMetricsDataOffset, client->GetQpcFrequency(), queryFrameDataDelta);

        if (adjusted_qpc > frame_data->present_event.PresentStartTime) {
            // Need to adjust the size of the window sample size
            double ms_adjustment =
                QpcDeltaToMs(adjusted_qpc - frame_data->present_event.PresentStartTime,
                    client->GetQpcFrequency());
            window_sample_size_in_ms = window_sample_size_in_ms - ms_adjustment;
            if (window_sample_size_in_ms <= 0.0) {
                return nullptr;
            }
        }
        else {
            // Find the frame with the appropriate time based on the adjusted
            // qpc
            for (;;) {

                if (DecrementIndex(nsm_view, index) == false) {
                    // Increment index to match up with the frame_data read below
                    index++;
                    break;
                }
                frame_data = client->ReadFrameByIdx(index);
                if (frame_data == nullptr) {
                    return nullptr;
                }
                if (adjusted_qpc >= frame_data->present_event.PresentStartTime) {
                    break;
                }
            }
        }

        return frame_data;
    }

    uint64_t ConcreteMiddleware::GetAdjustedQpc(uint64_t current_qpc, uint64_t frame_data_qpc, uint64_t queryMetricsOffset, LARGE_INTEGER frequency, uint64_t& queryFrameDataDelta) {
        // Calculate how far behind the frame data qpc is compared
        // to the client qpc
        uint64_t current_qpc_delta = current_qpc - frame_data_qpc;
        if (queryFrameDataDelta == 0) {
            queryFrameDataDelta = current_qpc_delta;
        }
        else {
            if (_abs64(queryFrameDataDelta - current_qpc_delta) >
                kClientFrameDeltaQPCThreshold) {
                queryFrameDataDelta = current_qpc_delta;
            }
        }

        // Add in the client set metric offset in qpc ticks
        return current_qpc -
            (queryFrameDataDelta + queryMetricsOffset);
    }

    bool ConcreteMiddleware::DecrementIndex(NamedSharedMem* nsm_view, uint64_t& index) {

        if (nsm_view == nullptr) {
            return false;
        }

        auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            return false;
        }

        uint64_t current_max_entries =
            (nsm_view->IsFull()) ? nsm_hdr->max_entries - 1 : nsm_hdr->tail_idx;
        index = (index == 0) ? current_max_entries : index - 1;
        if (index == nsm_hdr->head_idx) {
            return false;
        }

        return true;
    }

    bool ConcreteMiddleware::GetGpuMetricData(size_t telemetry_item_bit, PresentMonPowerTelemetryInfo& power_telemetry_info, PM_METRIC& gpuMetric, double& gpuMetricValue)
    {
        bool validGpuMetric = true;
        GpuTelemetryCapBits bit =
            static_cast<GpuTelemetryCapBits>(telemetry_item_bit);
        switch (bit) {
        case GpuTelemetryCapBits::time_stamp:
            // This is a valid telemetry cap bit but we do not produce metrics for
            // it.
            validGpuMetric = false;
            break;
        case GpuTelemetryCapBits::gpu_power:
            gpuMetric = PM_METRIC_GPU_POWER;
            gpuMetricValue = power_telemetry_info.gpu_power_w;
            break;
        case GpuTelemetryCapBits::gpu_sustained_power_limit:
            gpuMetric = PM_METRIC_GPU_SUSTAINED_POWER_LIMIT;
            gpuMetricValue = power_telemetry_info.gpu_sustained_power_limit_w;
            break;
        case GpuTelemetryCapBits::gpu_voltage:
            gpuMetric = PM_METRIC_GPU_VOLTAGE;
            gpuMetricValue = power_telemetry_info.gpu_voltage_v;
            break;
        case GpuTelemetryCapBits::gpu_frequency:
            gpuMetric = PM_METRIC_GPU_FREQUENCY;
            gpuMetricValue = power_telemetry_info.gpu_frequency_mhz;
            break;
        case GpuTelemetryCapBits::gpu_temperature:
            gpuMetric = PM_METRIC_GPU_TEMPERATURE;
            gpuMetricValue = power_telemetry_info.gpu_temperature_c;
            break;
        case GpuTelemetryCapBits::gpu_utilization:
            gpuMetric = PM_METRIC_GPU_UTILIZATION;
            gpuMetricValue = power_telemetry_info.gpu_utilization;
            break;
        case GpuTelemetryCapBits::gpu_render_compute_utilization:
            gpuMetric = PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION;
            gpuMetricValue = power_telemetry_info.gpu_render_compute_utilization;
            break;
        case GpuTelemetryCapBits::gpu_media_utilization:
            gpuMetric = PM_METRIC_GPU_MEDIA_UTILIZATION;
            gpuMetricValue = power_telemetry_info.gpu_media_utilization;
            break;
        case GpuTelemetryCapBits::vram_power:
            gpuMetric = PM_METRIC_VRAM_POWER;
            gpuMetricValue = power_telemetry_info.vram_power_w;
            break;
        case GpuTelemetryCapBits::vram_voltage:
            gpuMetric = PM_METRIC_VRAM_VOLTAGE;
            gpuMetricValue = power_telemetry_info.vram_voltage_v;
            break;
        case GpuTelemetryCapBits::vram_frequency:
            gpuMetric = PM_METRIC_VRAM_FREQUENCY;
            gpuMetricValue = power_telemetry_info.vram_frequency_mhz;
            break;
        case GpuTelemetryCapBits::vram_effective_frequency:
            gpuMetric = PM_METRIC_VRAM_EFFECTIVE_FREQUENCY;
            gpuMetricValue = power_telemetry_info.vram_effective_frequency_gbps;
            break;
        case GpuTelemetryCapBits::vram_temperature:
            gpuMetric = PM_METRIC_VRAM_TEMPERATURE;
            gpuMetricValue = power_telemetry_info.vram_temperature_c;
            break;
        case GpuTelemetryCapBits::fan_speed_0:
            gpuMetric = PM_METRIC_GPU_FAN_SPEED;
            gpuMetricValue = power_telemetry_info.fan_speed_rpm[0];
            break;
        case GpuTelemetryCapBits::fan_speed_1:
            gpuMetric = PM_METRIC_GPU_FAN_SPEED;
            gpuMetricValue = power_telemetry_info.fan_speed_rpm[1];
            break;
        case GpuTelemetryCapBits::fan_speed_2:
            gpuMetric = PM_METRIC_GPU_FAN_SPEED;
            gpuMetricValue = power_telemetry_info.fan_speed_rpm[2];
            break;
        case GpuTelemetryCapBits::fan_speed_3:
            gpuMetric = PM_METRIC_GPU_FAN_SPEED;
            gpuMetricValue = power_telemetry_info.fan_speed_rpm[3];
            break;
        case GpuTelemetryCapBits::fan_speed_4:
            gpuMetric = PM_METRIC_GPU_FAN_SPEED;
            gpuMetricValue = power_telemetry_info.fan_speed_rpm[4];
            break;
        case GpuTelemetryCapBits::gpu_mem_size:
            gpuMetric = PM_METRIC_GPU_MEM_SIZE;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_mem_total_size_b);
            break;
        case GpuTelemetryCapBits::gpu_mem_used:
            gpuMetric = PM_METRIC_GPU_MEM_USED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_mem_used_b);
            break;
        case GpuTelemetryCapBits::gpu_mem_max_bandwidth:
            gpuMetric = PM_METRIC_GPU_MEM_MAX_BANDWIDTH;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_mem_max_bandwidth_bps);
            break;
        case GpuTelemetryCapBits::gpu_mem_write_bandwidth:
            gpuMetric = PM_METRIC_GPU_MEM_WRITE_BANDWIDTH;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_mem_write_bandwidth_bps);
            break;
        case GpuTelemetryCapBits::gpu_mem_read_bandwidth:
            gpuMetric = PM_METRIC_GPU_MEM_READ_BANDWIDTH;
            gpuMetricValue = power_telemetry_info.gpu_mem_read_bandwidth_bps;
            break;
        case GpuTelemetryCapBits::gpu_power_limited:
            gpuMetric = PM_METRIC_GPU_POWER_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_power_limited);
            break;
        case GpuTelemetryCapBits::gpu_temperature_limited:
            gpuMetric = PM_METRIC_GPU_TEMPERATURE_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_temperature_limited);
            break;
        case GpuTelemetryCapBits::gpu_current_limited:
            gpuMetric = PM_METRIC_GPU_CURRENT_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_current_limited);
            break;
        case GpuTelemetryCapBits::gpu_voltage_limited:
            gpuMetric = PM_METRIC_GPU_VOLTAGE_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_voltage_limited);
            break;
        case GpuTelemetryCapBits::gpu_utilization_limited:
            gpuMetric = PM_METRIC_GPU_UTILIZATION_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.gpu_utilization_limited);
            break;
        case GpuTelemetryCapBits::vram_power_limited:
            gpuMetric = PM_METRIC_VRAM_POWER_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.vram_power_limited);
            break;
        case GpuTelemetryCapBits::vram_temperature_limited:
            gpuMetric = PM_METRIC_VRAM_TEMPERATURE_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.vram_temperature_limited);
            break;
        case GpuTelemetryCapBits::vram_current_limited:
            gpuMetric = PM_METRIC_VRAM_CURRENT_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.vram_current_limited);
            break;
        case GpuTelemetryCapBits::vram_voltage_limited:
            gpuMetric = PM_METRIC_VRAM_VOLTAGE_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.vram_voltage_limited);
            break;
        case GpuTelemetryCapBits::vram_utilization_limited:
            gpuMetric = PM_METRIC_VRAM_UTILIZATION_LIMITED;
            gpuMetricValue = static_cast<double>(power_telemetry_info.vram_utilization_limited);
            break;
        default:
            validGpuMetric = false;
            break;
        }
        return validGpuMetric;
    }

    bool ConcreteMiddleware::GetCpuMetricData(size_t telemetryBit, CpuTelemetryInfo& cpuTelemetry, PM_METRIC& cpuMetric, double& cpuMetricValue)
    {
        bool validCpuMetric = true;
        CpuTelemetryCapBits bit =
            static_cast<CpuTelemetryCapBits>(telemetryBit);
        switch (bit) {
        case CpuTelemetryCapBits::cpu_utilization:
            cpuMetric = PM_METRIC_CPU_UTILIZATION;
            cpuMetricValue = cpuTelemetry.cpu_utilization;
            break;
        case CpuTelemetryCapBits::cpu_power:
            cpuMetric = PM_METRIC_CPU_POWER;
            cpuMetricValue = cpuTelemetry.cpu_power_w;
            break;
        case CpuTelemetryCapBits::cpu_power_limit:
            cpuMetric = PM_METRIC_CPU_POWER_LIMIT;
            cpuMetricValue = cpuTelemetry.cpu_power_limit_w;
            break;
        case CpuTelemetryCapBits::cpu_temperature:
            cpuMetric = PM_METRIC_CPU_TEMPERATURE;
            cpuMetricValue = cpuTelemetry.cpu_temperature;
            break;
        case CpuTelemetryCapBits::cpu_frequency:
            cpuMetric = PM_METRIC_CPU_FREQUENCY;
            cpuMetricValue = cpuTelemetry.cpu_frequency;
            break;
        default:
            validCpuMetric = false;
            break;
        }

        return validCpuMetric;
    }

    void ConcreteMiddleware::SaveMetricCache(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob)
    {
        auto it = cachedMetricDatas.find(std::pair(pQuery->dynamicQueryHandle, processId));
        if (it != cachedMetricDatas.end())
        {
            auto& uniquePtr = it->second;
            std::copy(pBlob, pBlob + pQuery->queryCacheSize, uniquePtr.get());
        }
        else
        {
            auto dataArray = std::make_unique<uint8_t[]>(pQuery->queryCacheSize);
            std::copy(pBlob, pBlob + pQuery->queryCacheSize, dataArray.get());
            cachedMetricDatas.emplace(std::pair(pQuery->dynamicQueryHandle, processId), std::move(dataArray));
        }
    }

    void ConcreteMiddleware::CopyMetricCacheToBlob(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob)
    {
        auto it = cachedMetricDatas.find(std::pair(pQuery->dynamicQueryHandle, processId));
        if (it != cachedMetricDatas.end())
        {
            auto& uniquePtr = it->second;
            std::copy(uniquePtr.get(), uniquePtr.get() + pQuery->queryCacheSize, pBlob);
        }
    }

    // This code currently doesn't support the copying of multiple swap chains. If a second swap chain
    // is encountered it will update the numSwapChains to the correct number and then copy the swap
    // chain frame information with the most presents. If the client does happen to specify two swap
    // chains this code will incorrectly copy the data. WIP.
    void ConcreteMiddleware::CalculateMetrics(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains, LARGE_INTEGER qpcFrequency, std::unordered_map<uint64_t, fpsSwapChainData>& swapChainData, std::unordered_map<PM_METRIC, std::vector<double>>& gpucpuMetricData)
    {
        auto GetSwapChainIndex = [swapChainData]()
            { 
                uint32_t maxSwapChainPresents = 0;
                uint32_t maxSwapChainPresentsIndex = 0;
                uint32_t currentSwapChainIndex = 0;
                for (auto& pair : swapChainData) {
                    auto& swapChain = pair.second;
                    if (swapChain.num_presents > maxSwapChainPresents)
                    {
                        maxSwapChainPresents = swapChain.num_presents;
                        maxSwapChainPresentsIndex = currentSwapChainIndex;
                    }
                    currentSwapChainIndex++;
                }
                return maxSwapChainPresentsIndex;
            };

        uint32_t maxSwapChainPresentsIndex = GetSwapChainIndex();
        uint32_t currentSwapChainIndex = 0;
        bool copyAllMetrics = true;
        bool useCache = false;
        bool allMetricsCalculated = false;

        // If the number of swap chains found in the frame data is greater than the number passed
        // in update the passed in number to notify the client there is more data present than
        // can be returned
        if (swapChainData.size() > *numSwapChains)
        {
            *numSwapChains = static_cast<uint32_t>(swapChainData.size());
            copyAllMetrics = false;
        }

        // If the client choose to monitor frame information then this loop
        // will calculate all store all metrics.
        for (auto& pair : swapChainData) {
            auto& swapChain = pair.second;

            // There are couple reasons where we will not be able to produce
            // fps metric data. The first is if all of the frames are dropped.
            // The second is if in the requested sample window there are
            // no presents.
            if ((swapChain.display_count <= 1) && (swapChain.num_presents <= 1)) {
                useCache = true;
                break;
            }

            // If we are unable to copy all of the metrics to the blob and the current swap
            // chain isn't the one with the most presents, skip by it
            if ((copyAllMetrics == false) && (currentSwapChainIndex != maxSwapChainPresentsIndex))
            {
                continue;
            }
            for (auto& qe : pQuery->elements) {
                switch (qe.metric)
                {
                case PM_METRIC_PRESENTED_FPS:
                case PM_METRIC_DISPLAYED_FPS:
                case PM_METRIC_FRAME_TIME:
                case PM_METRIC_GPU_BUSY_TIME:
                case PM_METRIC_CPU_BUSY_TIME:
                case PM_METRIC_CPU_WAIT_TIME:
                case PM_METRIC_DISPLAY_BUSY_TIME:
                    CalculateFpsMetric(swapChain, qe, pBlob, qpcFrequency);
                    break;
                case PM_METRIC_PROCESS_NAME:
                    strcpy_s(reinterpret_cast<char*>(&pBlob[qe.dataOffset]), 260, swapChain.applicationName.c_str());
                    break;
                case PM_METRIC_CPU_VENDOR:
                {
                    auto& output = reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[qe.dataOffset]);
                    output = cachedCpuInfo[0].deviceVendor;
                }
                    break;
                case PM_METRIC_GPU_VENDOR:
                {
                    auto& output = reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[qe.dataOffset]);
                    output = cachedGpuInfo[currentGpuInfoIndex].deviceVendor;
                }
                    break;
                case PM_METRIC_CPU_NAME:
                    strcpy_s(reinterpret_cast<char*>(&pBlob[qe.dataOffset]), 260, cachedCpuInfo[0].deviceName.c_str());
                    break;
                case PM_METRIC_GPU_NAME:
                    strcpy_s(reinterpret_cast<char*>(&pBlob[qe.dataOffset]), 260, cachedGpuInfo[currentGpuInfoIndex].deviceName.c_str());
                    break;
                case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
                {
                    auto& output = reinterpret_cast<double&>(pBlob[qe.dataOffset]);
                    output = cachedGpuMemMaxBandwidth;
                }
                    break;
                case PM_METRIC_GPU_MEM_SIZE:
                {
                    auto& output = reinterpret_cast<double&>(pBlob[qe.dataOffset]);
                    output = cachedGpuMemSize;
                }
                    break;
                default:
                    CalculateGpuCpuMetric(gpucpuMetricData, qe, pBlob);
                    break;
                }
            }

            allMetricsCalculated = true;
            currentSwapChainIndex++;
        }

        if (useCache == true) {
            CopyMetricCacheToBlob(pQuery, processId, pBlob);
            return;
        }

        if (allMetricsCalculated == false)
        {
            for (auto& qe : pQuery->elements)
            {
                switch (qe.metric)
                {
                case PM_METRIC_GPU_POWER:
                case PM_METRIC_GPU_FAN_SPEED:
                case PM_METRIC_GPU_SUSTAINED_POWER_LIMIT:
                case PM_METRIC_GPU_VOLTAGE:
                case PM_METRIC_GPU_FREQUENCY:
                case PM_METRIC_GPU_TEMPERATURE:
                case PM_METRIC_GPU_UTILIZATION:
                case PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION:
                case PM_METRIC_GPU_MEDIA_UTILIZATION:
                case PM_METRIC_VRAM_POWER:
                case PM_METRIC_VRAM_VOLTAGE:
                case PM_METRIC_VRAM_FREQUENCY:
                case PM_METRIC_VRAM_EFFECTIVE_FREQUENCY:
                case PM_METRIC_VRAM_TEMPERATURE:
                case PM_METRIC_GPU_MEM_USED:
                case PM_METRIC_GPU_MEM_WRITE_BANDWIDTH:
                case PM_METRIC_GPU_MEM_READ_BANDWIDTH:
                case PM_METRIC_GPU_POWER_LIMITED:
                case PM_METRIC_GPU_TEMPERATURE_LIMITED:
                case PM_METRIC_GPU_CURRENT_LIMITED:
                case PM_METRIC_GPU_VOLTAGE_LIMITED:
                case PM_METRIC_GPU_UTILIZATION_LIMITED:
                case PM_METRIC_VRAM_POWER_LIMITED:
                case PM_METRIC_VRAM_TEMPERATURE_LIMITED:
                case PM_METRIC_VRAM_CURRENT_LIMITED:
                case PM_METRIC_VRAM_VOLTAGE_LIMITED:
                case PM_METRIC_VRAM_UTILIZATION_LIMITED:
                case PM_METRIC_CPU_UTILIZATION:
                case PM_METRIC_CPU_POWER:
                case PM_METRIC_CPU_POWER_LIMIT:
                case PM_METRIC_CPU_TEMPERATURE:
                case PM_METRIC_CPU_FREQUENCY:
                case PM_METRIC_CPU_CORE_UTILITY:
                    CalculateGpuCpuMetric(gpucpuMetricData, qe, pBlob);
                    break;
                case PM_METRIC_CPU_VENDOR:
                {
                    auto& output = reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[qe.dataOffset]);
                    output = cachedCpuInfo[0].deviceVendor;
                }
                break;
                case PM_METRIC_GPU_VENDOR:
                {
                    auto& output = reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[qe.dataOffset]);
                    output = cachedGpuInfo[currentGpuInfoIndex].deviceVendor;
                }
                break;
                case PM_METRIC_CPU_NAME:
                    strcpy_s(reinterpret_cast<char*>(pBlob[qe.dataOffset]), 260, cachedCpuInfo[0].deviceName.c_str());
                    break;
                case PM_METRIC_GPU_NAME:
                    strcpy_s(reinterpret_cast<char*>(pBlob[qe.dataOffset]), 260, cachedGpuInfo[currentGpuInfoIndex].deviceName.c_str());
                    break;
                case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
                {
                    auto& output = reinterpret_cast<double&>(pBlob[qe.dataOffset]);
                    output = cachedGpuMemMaxBandwidth;
                }
                break;
                case PM_METRIC_GPU_MEM_SIZE:
                {
                    auto& output = reinterpret_cast<double&>(pBlob[qe.dataOffset]);
                    output = cachedGpuMemSize;
                }
                break;
                default:
                    break;
                }
            }
        }

        // Save calculated metrics blob to cache
        SaveMetricCache(pQuery, processId, pBlob);
    }

    PM_STATUS ConcreteMiddleware::SetActiveGraphicsAdapter(uint32_t adapterId) {
        MemBuffer requestBuf;
        MemBuffer responseBuf;

        NamedPipeHelper::EncodeGeneralSetActionRequest(PM_ACTION::SELECT_ADAPTER,
            &requestBuf, adapterId);

        PM_STATUS status = CallPmService(&requestBuf, &responseBuf);
        if (status != PM_STATUS::PM_STATUS_SUCCESS) {
            return status;
        }

        status = NamedPipeHelper::DecodeGeneralSetActionResponse(
            PM_ACTION::SELECT_ADAPTER, &responseBuf);

        return status;
    }

}
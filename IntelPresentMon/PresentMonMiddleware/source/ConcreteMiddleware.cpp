#include "ConcreteMiddleware.h"
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdlib>
#include "../../PresentMonUtils/NamedPipeHelper.h"
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

namespace pmon::mid
{
    using namespace ipc::intro;

    static const uint32_t kMaxRespBufferSize = 4096;
	ConcreteMiddleware::ConcreteMiddleware()
	{
        LPCTSTR pipe_name = TEXT("\\\\.\\pipe\\presentmonsvcnamedpipe");

        HANDLE namedPipeHandle;
        // Try to open a named pipe; wait for it, if necessary.
        while (1) {
            namedPipeHandle = CreateFile(pipe_name,
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
            if (GetLastError() != ERROR_PIPE_BUSY) {
                throw std::runtime_error{ "Service not found" };
            }

            // All pipe instances are busy, so wait for 20 seconds.
            if (!WaitNamedPipe(pipe_name, 20000)) {
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

        // TODO: Where will the client caches reside? As part of the dynamic query?
        //if (!SetupClientCaches(process_id)) {
        //    return PM_STATUS::PM_STATUS_FAILURE;
        //}

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
        //auto iter = presentMonStreamClients.find(processId);
        //if (iter != presentMonStreamClients.end()) {
        //    presentMonStreamClients.erase(std::move(iter));
        //}

        // TODO: If cached data is part of query maybe we can
        // remove this code
        //RemoveClientCaches(process_id);

        return status;
    }

    PM_DYNAMIC_QUERY* ConcreteMiddleware::RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t processId, double windowSizeMs, double metricOffsetMs)
    { 
        // get introspection data for reference
        // TODO: cache this data so it's not required to be generated every time
        pmapi::intro::Dataset ispec{ GetIntrospectionData(), [this](auto p) {FreeIntrospectionData(p); } };

        // make the query object that will be managed by the handle
        auto pQuery = std::make_unique<PM_DYNAMIC_QUERY>();

        uint64_t offset = 0u;
        for (auto& qe : queryElements) {
            auto metricView = ispec.FindMetric(qe.metric);
            if (metricView.GetType().GetValue() != int(PM_METRIC_TYPE_DYNAMIC)) {
                // TODO: specific exception here
                throw std::runtime_error{ "Static metric in dynamic metric query specification" };
            }
            switch (qe.metric) {
            case PM_METRIC_PRESENTED_FPS:
            case PM_METRIC_DISPLAYED_FPS:
            case PM_METRIC_FRAME_TIME:
            case PM_METRIC_GPU_BUSY_TIME:
            case PM_METRIC_CPU_BUSY_TIME:
            case PM_METRIC_CPU_WAIT_TIME:
            case PM_METRIC_DISPLAY_BUSY_TIME:
                pQuery->accumFpsData = true;
                break;
            case PM_METRIC_GPU_POWER:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_power));
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
                default:
                    // Unknown fan speed index
                    throw std::runtime_error{ "Invalid fan speed index" };
                }
                break;
            default:
                break;
            }

            // TODO: Add or update requested stats for the metric

            // TODO: validate device id
            // TODO: validate array index
            qe.dataOffset = offset;
            qe.dataSize = GetDataTypeSize(metricView.GetDataTypeInfo().GetBasePtr()->type);
            offset += qe.dataSize;
        }

        pQuery->metricOffsetMs = metricOffsetMs;
        pQuery->windowSizeMs = windowSizeMs;
        pQuery->processId = processId;
        pQuery->elements = std::vector<PM_QUERY_ELEMENT>{ queryElements.begin(), queryElements.end() };

        return pQuery.release();
    }
}
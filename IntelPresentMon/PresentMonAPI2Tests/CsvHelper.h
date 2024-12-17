// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../CommonUtilities/str/String.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <map>
#include <optional>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

enum Header {
    Header_Application,
    Header_ProcessID,
    Header_SwapChainAddress,
    Header_PresentRuntime,
    Header_SyncInterval,
    Header_PresentFlags,
    Header_AllowsTearing,
    Header_PresentMode,
    Header_FrameType,
    Header_CPUStartTime,
    Header_CPUStartQPC,
    Header_CPUStartQPCTime,
    Header_CPUStartDateTime,
    Header_FrameTime,
    Header_CPUBusy,
    Header_CPUWait,
    Header_GPULatency,
    Header_GPUTime,
    Header_GPUBusy,
    Header_GPUWait,
    Header_VideoBusy,
    Header_DisplayLatency,
    Header_DisplayedTime,
    Header_AnimationError,
    Header_AnimationTime,
    Header_ClickToPhotonLatency,
    Header_AllInputToPhotonLatency,

    // App Provided Metrics
    Header_InstrumentedLatency,

    // --v1_metrics
    Header_Runtime,
    Header_Dropped,
    Header_TimeInSeconds,
    Header_msBetweenPresents,
    Header_msInPresentAPI,
    Header_msBetweenDisplayChange,
    Header_msUntilRenderComplete,
    Header_msUntilDisplayed,
    Header_msUntilRenderStart,
    Header_msGPUActive,
    Header_msGPUVideoActive,
    Header_msSinceInput,
    Header_QPCTime,

    // Deprecated
    Header_WasBatched,
    Header_DwmNotified,

    // Special values:
    KnownHeaderCount,
    UnknownHeader,
};

struct v2Metrics {
    std::string appName;
    uint32_t processId = 0;
    uint64_t swapChain = 0;
    PM_GRAPHICS_RUNTIME runtime = PM_GRAPHICS_RUNTIME_UNKNOWN;
    int32_t syncInterval = 0;
    uint32_t presentFlags = 0;
    uint32_t allowsTearing = 0;
    PM_PRESENT_MODE presentMode = PM_PRESENT_MODE_UNKNOWN;
    PM_FRAME_TYPE frameType = PM_FRAME_TYPE_NOT_SET;
    uint64_t cpuFrameQpc = 0;
    double cpuFrameTime = 0.;
    double cpuBusy = 0.;
    double cpuWait = 0.;
    double gpuLatency = 0.;
    double gpuTime = 0.;
    double gpuBusy = 0.;
    double gpuWait = 0.;
    double videoBusy = 0.;
    std::optional<double> displayLatency;
    std::optional<double> displayedTime;
    std::optional<double> animationError;
    std::optional<double> animationTime;
    std::optional<double> clickToPhotonLatency;
    std::optional<double> AllInputToPhotonLatency;
    std::optional<double> InstrumentedLatency;
};

constexpr char const* GetHeaderString(Header h)
{
    switch (h) {
    case Header_Application:                return "Application";
    case Header_ProcessID:                  return "ProcessID";
    case Header_SwapChainAddress:           return "SwapChainAddress";
    case Header_PresentRuntime:             return "PresentRuntime";
    case Header_SyncInterval:               return "SyncInterval";
    case Header_PresentFlags:               return "PresentFlags";
    case Header_AllowsTearing:              return "AllowsTearing";
    case Header_PresentMode:                return "PresentMode";
    case Header_FrameType:                  return "FrameType";
    case Header_CPUStartTime:               return "CPUStartTime";
    case Header_CPUStartQPC:                return "CPUStartQPC";
    case Header_CPUStartQPCTime:            return "CPUStartQPCTime";
    case Header_CPUStartDateTime:           return "CPUStartDateTime";
    case Header_FrameTime:                  return "FrameTime";
    case Header_CPUBusy:                    return "CPUBusy";
    case Header_CPUWait:                    return "CPUWait";
    case Header_GPULatency:                 return "GPULatency";
    case Header_GPUTime:                    return "GPUTime";
    case Header_GPUBusy:                    return "GPUBusy";
    case Header_VideoBusy:                  return "VideoBusy";
    case Header_GPUWait:                    return "GPUWait";
    case Header_DisplayLatency:             return "DisplayLatency";
    case Header_DisplayedTime:              return "DisplayedTime";
    case Header_AnimationError:             return "AnimationError";
    case Header_AnimationTime:              return "AnimationTime";
    case Header_ClickToPhotonLatency:       return "ClickToPhotonLatency";
    case Header_AllInputToPhotonLatency:    return "AllInputToPhotonLatency";

    case Header_Runtime:                    return "Runtime";
    case Header_Dropped:                    return "Dropped";
    case Header_TimeInSeconds:              return "TimeInSeconds";
    case Header_msBetweenPresents:          return "msBetweenPresents";
    case Header_msInPresentAPI:             return "msInPresentAPI";
    case Header_msBetweenDisplayChange:     return "msBetweenDisplayChange";
    case Header_msUntilRenderComplete:      return "msUntilRenderComplete";
    case Header_msUntilDisplayed:           return "msUntilDisplayed";
    case Header_msUntilRenderStart:         return "msUntilRenderStart";
    case Header_msGPUActive:                return "msGPUActive";
    case Header_msGPUVideoActive:           return "msGPUVideoActive";
    case Header_msSinceInput:               return "msSinceInput";
    case Header_QPCTime:                    return "QPCTime";

    case Header_WasBatched:                 return "WasBatched";
    case Header_DwmNotified:                return "DwmNotified";

    case Header_InstrumentedLatency:        return "InstrumentedLatency";

    default:                                return "<unknown>";
    }
}

std::wstring CreateErrorString(Header columnId, size_t line)
{
    std::wstring errorMessage = L"Invalid ";
    errorMessage += pmon::util::str::ToWide(GetHeaderString(columnId));
    errorMessage += L" at line: ";
    errorMessage += std::to_wstring(line);

    return errorMessage;
}

template <typename T>
class CharConvert {
public:
    void Convert(const std::string data, T& convertedData, Header columnId, size_t line);
};

template <typename T>
void CharConvert<T>::Convert(const std::string data, T& convertedData, Header columnId, size_t line) {
    if constexpr (std::is_same<T, double>::value) {
        try
        {
            convertedData = std::stod(data);
        }
        catch (...) {
            Assert::Fail(CreateErrorString(columnId, line).c_str());
        }
    }
    else if constexpr (std::is_same<T, uint32_t>::value) {
        try
        {
            convertedData = std::stoul(data);
        }
        catch (...) {
            Assert::Fail(CreateErrorString(columnId, line).c_str());
        }
    }
    else if constexpr (std::is_same<T, int32_t>::value) {
        try
        {
            convertedData = std::stoi(data);
        }
        catch (...) {
            Assert::Fail(CreateErrorString(columnId, line).c_str());
        }
    }
    else if constexpr (std::is_same<T, uint64_t>::value) {
        try
        {
            char* endptr;
            unsigned long long result1;

            if (data.size() > 2 && data[0] == '0' && (data[1] == 'x' || data[1] == 'X')) {
                // Convert hexadecimal
                result1 = std::strtoull(data.c_str(), &endptr, 16);
            }
            else {
                // Convert decimal
                result1 = std::strtoull(data.c_str(), &endptr, 10);
            }
            convertedData = static_cast<uint64_t>(result1);
        }
        catch (...) {
            Assert::Fail(CreateErrorString(columnId, line).c_str());

        }
    }
    else if constexpr (std::is_same<T, PM_GRAPHICS_RUNTIME>::value) {
        if (data == "DXGI") {
            convertedData = PM_GRAPHICS_RUNTIME_DXGI;
        }
        else if (data == "D3D9") {
            convertedData = PM_GRAPHICS_RUNTIME_D3D9;
        }
        else if (data == "Other") {
            convertedData = PM_GRAPHICS_RUNTIME_UNKNOWN;
        }
        else {
            Assert::Fail(CreateErrorString(columnId, line).c_str());
        }

    }
    else if constexpr (std::is_same<T, PM_PRESENT_MODE>::value) {
        if (data == "Hardware: Legacy Flip") {
            convertedData = PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP;
        }
        else if (data == "Hardware: Legacy Copy to Front Buffer") {
            convertedData = PM_PRESENT_MODE_HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER;
        }
        else if (data == "Hardware: Independent Flip") {
            convertedData = PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP;
        }
        else if (data == "Composed: Flip") {
            convertedData = PM_PRESENT_MODE_COMPOSED_FLIP;
        }
        else if (data == "Composed: Copy with GPU GDI") {
            convertedData = PM_PRESENT_MODE_COMPOSED_COPY_WITH_GPU_GDI;
        }
        else if (data == "Composed: Copy with CPU GDI") {
            convertedData = PM_PRESENT_MODE_COMPOSED_COPY_WITH_CPU_GDI;
        }
        else if (data == "Hardware Composed: Independent Flip") {
            convertedData = PM_PRESENT_MODE_HARDWARE_COMPOSED_INDEPENDENT_FLIP;
        }
        else if (data == "Other") {
            convertedData = PM_PRESENT_MODE_UNKNOWN;
        }
        else {
            Assert::Fail(CreateErrorString(Header_PresentMode, line).c_str());
        }
    }
    else if constexpr (std::is_same<T, PM_FRAME_TYPE>::value) {
        if (data == "NotSet") {
            convertedData = PM_FRAME_TYPE_NOT_SET;
        }
        else if (data == "Unspecified") {
            convertedData = PM_FRAME_TYPE_UNSPECIFIED;
        }
        else if (data == "Application") {
            convertedData = PM_FRAME_TYPE_APPLICATION;
        }
        else if (data == "Repeated") {
            convertedData = PM_FRAME_TYPE_REPEATED;
        }
        else if (data == "AMD_AFMF") {
            convertedData = PM_FRAME_TYPE_AMD_AFMF;
        }
        else {
            Assert::Fail(CreateErrorString(Header_FrameType, line).c_str());
        }
    }
    else
    {
        Assert::Fail(CreateErrorString(UnknownHeader, line).c_str());
    }
}

size_t countDecimalPlaces(double value) {
    // Convert double to string
    std::string str = std::to_string(value);

    char const* a = str.c_str();

    double testNumber = 0.0;
    double goldNumber = 0.0;
    int testSucceededCount = sscanf_s(a, "%lf", &testNumber);

    const char* testDecimalAddr = strchr(a, '.');
    size_t testDecimalNumbersCount = testDecimalAddr == nullptr ? 0 : ((a + strlen(a)) - testDecimalAddr - 1);
    return testDecimalNumbersCount;
}

template<typename T>
bool Validate(const T& param1, const T& param2) {
    if constexpr (std::is_same<T, double>::value) {
        auto min = std::min(countDecimalPlaces(param1), countDecimalPlaces(param2));
        double threshold = pow(0.1, min);
        double difference = param1 - param2;
        if (difference > -threshold && difference < threshold) {
            return true;
        }
        else
        {
            return false;
        }
    }
    else {
        return param1 == param2;
    }

}

class CsvParser {
public:
    CsvParser();

    bool Open(std::wstring const& path, uint32_t processId);
    void Close();
    bool VerifyBlobAgainstCsv(const std::string& processName, const unsigned int& processId,
        PM_QUERY_ELEMENT(&queryElements)[22], pmapi::BlobContainer& blobs);
    bool ResetCsv();

private:
    bool FindFirstRowWithPid(const unsigned int& processId);
    bool ReadRow(bool gatherMetrics = false);
    size_t GetColumnIndex(char const* header);

    Header FindHeader(char const* header);
    void CheckAll(size_t const* columnIndex, bool* ok, std::initializer_list<Header> const& headers);

    void ConvertToMetricDataType(const char* data, Header columnId);

    FILE* fp_ = nullptr;

    size_t headerColumnIndex_[KnownHeaderCount];

    char row_[1024];
    size_t line_ = 0;
    std::vector<char const*> cols_;
    v2Metrics v2MetricRow_;
    uint32_t processId_ = 0;
    std::map<size_t, Header> activeColHeadersMap_;
};

CsvParser::CsvParser()
{}

bool CsvParser::VerifyBlobAgainstCsv(const std::string& processName, const unsigned int& processId,
    PM_QUERY_ELEMENT(&queryElements)[22], pmapi::BlobContainer& blobs)
{

    for (auto pBlob : blobs) {
        // Read a row of blob data
        //const auto appName = *reinterpret_cast<const char*>(&pBlob[queryElements[0].dataOffset]);
        const auto swapChain = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[0].dataOffset]);
        const auto graphicsRuntime = *reinterpret_cast<const PM_GRAPHICS_RUNTIME*>(&pBlob[queryElements[1].dataOffset]);
        const auto syncInterval = *reinterpret_cast<const int32_t*>(&pBlob[queryElements[2].dataOffset]);
        const auto presentFlags = *reinterpret_cast<const uint32_t*>(&pBlob[queryElements[3].dataOffset]);
        const auto allowsTearing = *reinterpret_cast<const bool*>(&pBlob[queryElements[4].dataOffset]);
        const auto presentMode = *reinterpret_cast<const PM_PRESENT_MODE*>(&pBlob[queryElements[5].dataOffset]);
        const auto frameType = *reinterpret_cast<const PM_FRAME_TYPE*>(&pBlob[queryElements[6].dataOffset]);
        const auto cpuFrameQpc = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[7].dataOffset]);
        const auto cpuFrameTime = *reinterpret_cast<const double*>(&pBlob[queryElements[8].dataOffset]);
        const auto cpuBusy = *reinterpret_cast<const double*>(&pBlob[queryElements[9].dataOffset]);
        const auto cpuWait = *reinterpret_cast<const double*>(&pBlob[queryElements[10].dataOffset]);
        const auto gpuLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[11].dataOffset]);
        const auto gpuTime = *reinterpret_cast<const double*>(&pBlob[queryElements[12].dataOffset]);
        const auto gpuBusy = *reinterpret_cast<const double*>(&pBlob[queryElements[13].dataOffset]);
        const auto gpuWait = *reinterpret_cast<const double*>(&pBlob[queryElements[14].dataOffset]);
        const auto displayLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[15].dataOffset]);
        const auto displayedTime = *reinterpret_cast<const double*>(&pBlob[queryElements[16].dataOffset]);
        const auto animationError = *reinterpret_cast<const double*>(&pBlob[queryElements[17].dataOffset]);
        const auto animationTime = *reinterpret_cast<const double*>(&pBlob[queryElements[18].dataOffset]);
        const auto allInputToPhotonLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[19].dataOffset]);
        const auto clickToPhotonLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[20].dataOffset]);
        const auto instrumentedLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[21].dataOffset]);


        
        // Read rows until we find one with the process we are interested in
        // or we are out of data.
        for (;;) {
            if (ReadRow(true)) {
                if (v2MetricRow_.processId == processId_) {
                    break;
                }
            }
            else
            {
                return true;
            }
        }

        assert(v2MetricRow_.processId == processId_);

        // Go through all of the active headers and validate results
        for (const auto& pair : activeColHeadersMap_) {

            bool columnsMatch = false;
            switch (pair.second)
            {
            case Header_Application:
                columnsMatch = true;
                break;
            case Header_ProcessID:
                columnsMatch = Validate(v2MetricRow_.processId, processId_);
                break;
            case Header_SwapChainAddress:
                    columnsMatch = Validate(v2MetricRow_.swapChain, swapChain);
                break;
            case Header_Runtime:
                columnsMatch = Validate(v2MetricRow_.runtime, graphicsRuntime);
                break;
            case Header_SyncInterval:
                columnsMatch = Validate(v2MetricRow_.syncInterval, syncInterval);
                break;
            case Header_PresentFlags:
                columnsMatch = Validate(v2MetricRow_.presentFlags, presentFlags);
                break;
            case Header_AllowsTearing:
                columnsMatch = Validate(v2MetricRow_.allowsTearing, (uint32_t)allowsTearing);
                break;
            case Header_PresentMode:
                columnsMatch = Validate(v2MetricRow_.presentMode, presentMode);
                break;
            case Header_FrameType:
                columnsMatch = Validate(v2MetricRow_.frameType, frameType);
                break;
            case Header_CPUStartQPC:
                columnsMatch = Validate(v2MetricRow_.cpuFrameQpc, cpuFrameQpc);
                break;
            case Header_FrameTime:
                columnsMatch = Validate(v2MetricRow_.cpuFrameTime, cpuFrameTime);
                break;
            case Header_CPUBusy:
                columnsMatch = Validate(v2MetricRow_.cpuBusy, cpuBusy);
                break;
            case Header_CPUWait:
                columnsMatch = Validate(v2MetricRow_.cpuWait, cpuWait);
                break;
            case Header_GPULatency:
                columnsMatch = Validate(v2MetricRow_.gpuLatency, gpuLatency);
                break;
            case Header_GPUTime:
                columnsMatch = Validate(v2MetricRow_.gpuTime, gpuTime);
                break;
            case Header_GPUBusy:
                columnsMatch = Validate(v2MetricRow_.gpuBusy, gpuBusy);
                break;
            case Header_GPUWait:
                columnsMatch = Validate(v2MetricRow_.gpuWait, gpuWait);
                break;
            case Header_DisplayLatency:
                if (v2MetricRow_.displayLatency.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.displayLatency.value(), displayLatency);
                }
                else
                {
                    if (std::isnan(displayLatency)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;

            case Header_DisplayedTime:
                if (v2MetricRow_.displayedTime.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.displayedTime.value(), displayedTime);
                }
                else
                {
                    if (std::isnan(displayedTime)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_AnimationError:
                if (v2MetricRow_.animationError.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.animationError.value(), animationError);
                }
                else
                {
                    if (std::isnan(animationError)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_AnimationTime:
                if (v2MetricRow_.animationTime.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.animationTime.value(), animationTime);
                }
                else
                {
                    if (std::isnan(animationTime)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_ClickToPhotonLatency:
                if (v2MetricRow_.clickToPhotonLatency.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.clickToPhotonLatency.value(), clickToPhotonLatency);
                }
                else
                {
                    if (std::isnan(clickToPhotonLatency)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_AllInputToPhotonLatency:
                if (v2MetricRow_.AllInputToPhotonLatency.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.AllInputToPhotonLatency.value(), allInputToPhotonLatency);
                }
                else
                {
                    if (std::isnan(allInputToPhotonLatency)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_InstrumentedLatency:
                if (v2MetricRow_.InstrumentedLatency.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.InstrumentedLatency.value(), instrumentedLatency);
                }
                else
                {
                    if (std::isnan(instrumentedLatency)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            default:
                columnsMatch = true;
                break;
            }
            Assert::IsTrue(columnsMatch, CreateErrorString(pair.second, line_).c_str());
        }
    }

    return true;
}

bool CsvParser::FindFirstRowWithPid(const unsigned int& searchProcessId)
{
    for (;;) {
        if (!ReadRow(true)) {
            break;
        }
        if (headerColumnIndex_[Header_ProcessID] != SIZE_MAX) {
            auto processColIdx = headerColumnIndex_[Header_ProcessID];
            char const* processIdString = nullptr;
            if (processColIdx < cols_.size()) {
                processIdString = cols_[Header_ProcessID];
                unsigned int currentProcessId = 0;
                int succeededCount = sscanf_s(processIdString, "%ud", &currentProcessId);
                if (searchProcessId == currentProcessId) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool CsvParser::ResetCsv()
{
    // Rewind the file pointer and then read the header to get
    // to the data
    rewind(fp_);
    ReadRow();

    return true;
}

bool CsvParser::Open(std::wstring const& path, uint32_t processId) {
    for (uint32_t i = 0; i < _countof(headerColumnIndex_); ++i) {
        headerColumnIndex_[i] = SIZE_MAX;
    }
    cols_.clear();

    if (_wfopen_s(&fp_, path.c_str(), L"r")) {
        return false;
    }

    // Remove UTF-8 marker if there is one.
    if (fread(row_, 3, 1, fp_) != 1 ||
        row_[0] != -17 || // 0xef
        row_[1] != -69 || // 0xbb
        row_[2] != -65) { // 0xbf
        fseek(fp_, 0, SEEK_SET);
    }

    // Read the header and ensure required columns are present
    ReadRow();

    for (size_t i = 0, n = cols_.size(); i < n; ++i) {
        auto h = FindHeader(cols_[i]);
        switch (h) {
        case KnownHeaderCount:
        case UnknownHeader:
            Assert::Fail(CreateErrorString(h, line_).c_str());
        default:
            if ((size_t)h < KnownHeaderCount) {
                if (headerColumnIndex_[(size_t)h] != SIZE_MAX) {
                    std::wstring errorMessage = L"Duplicate column: ";
                    errorMessage += pmon::util::str::ToWide(cols_[i]);
                    Assert::Fail(errorMessage.c_str());
                }
                else {
                    headerColumnIndex_[(size_t)h] = i;
                }
                break;
            }
            else {
                std::wstring errorMessage = L"Index outside of known headers.";
                Assert::Fail(errorMessage.c_str());
            }
        }
    }

    bool columnsOK = true;
    CheckAll(headerColumnIndex_, &columnsOK, { Header_Application,
                                               Header_ProcessID,
                                               Header_SwapChainAddress,
                                               Header_PresentRuntime,
                                               Header_SyncInterval,
                                               Header_PresentFlags,
                                               Header_AllowsTearing,
                                               Header_PresentMode,
                                               Header_FrameType,
                                               Header_CPUStartQPC,
                                               Header_FrameTime,
                                               Header_CPUBusy,
                                               Header_CPUWait,
                                               Header_GPULatency,
                                               Header_GPUTime,
                                               Header_GPUBusy,
                                               Header_GPUWait,
                                               Header_VideoBusy,
                                               Header_DisplayLatency,
                                               Header_DisplayedTime,
                                               Header_AnimationError,
                                               Header_AnimationTime,
                                               Header_ClickToPhotonLatency,
                                               Header_AllInputToPhotonLatency,
                                               Header_InstrumentedLatency });

    if (!columnsOK) {
        Assert::Fail(L"Missing required columns");
    }

    // Create a vector of active headers to be used when reading
    // metric data
    for (uint32_t i = 0; i < KnownHeaderCount; ++i) {
        if (headerColumnIndex_[i] != SIZE_MAX) {
            activeColHeadersMap_[headerColumnIndex_[i]] = (Header)i;
        }
    }

    // Set the process id for this CSV parser
    processId_ = processId;

    return true;
}

void CsvParser::CheckAll(size_t const* columnIndex, bool* ok, std::initializer_list<Header> const& headers)
{
    for (auto const& h : headers) {
        if (columnIndex[h] == SIZE_MAX) {
            *ok = false;
            return;
        }
    }
}

void CsvParser::Close()
{
    fclose(fp_);
    fp_ = nullptr;
}

void CsvParser::ConvertToMetricDataType(const char* data, Header columnId)
{
    switch (columnId)
    {
    case Header_Application:
    {
        v2MetricRow_.appName = data;
    }
    break;
    case Header_ProcessID:
    {
        CharConvert<uint32_t> converter;
        converter.Convert(data, v2MetricRow_.processId, columnId, line_);
    }
    break;
    case Header_SwapChainAddress:
    {
        CharConvert<uint64_t> converter;
        converter.Convert(data, v2MetricRow_.swapChain, columnId, line_);
    }
    break;
    case Header_PresentRuntime:
    {
        CharConvert<PM_GRAPHICS_RUNTIME> converter;
        converter.Convert(data, v2MetricRow_.runtime, columnId, line_);
    }
    break;
    case Header_SyncInterval:
    {
        CharConvert<int32_t> converter;
        converter.Convert(data, v2MetricRow_.syncInterval, columnId, line_);
    }
    break;
    case Header_PresentFlags:
    {
        CharConvert<uint32_t> converter;
        converter.Convert(data, v2MetricRow_.presentFlags, columnId, line_);
    }
    break;
    case Header_AllowsTearing:
    {
        CharConvert<uint32_t> converter;
        converter.Convert(data, v2MetricRow_.allowsTearing, columnId, line_);
    }
    break;
    case Header_PresentMode:
    {
        CharConvert<PM_PRESENT_MODE> converter;
        converter.Convert(data, v2MetricRow_.presentMode, columnId, line_);
    }
    break;
    case Header_FrameType:
    {
        CharConvert<PM_FRAME_TYPE> converter;
        converter.Convert(data, v2MetricRow_.frameType, columnId, line_);
    }
    break;
    case Header_CPUStartQPC:
    {
        CharConvert<uint64_t> converter;
        converter.Convert(data, v2MetricRow_.cpuFrameQpc, columnId, line_);
    }
    break;
    case Header_FrameTime:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.cpuFrameTime, columnId, line_);
    }
    break;
    case Header_CPUBusy:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.cpuBusy, columnId, line_);
    }
    break;
    case Header_CPUWait:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.cpuWait, columnId, line_);
    }
    break;
    case Header_GPULatency:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.gpuLatency, columnId, line_);
    }
    break;
    case Header_GPUTime:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.gpuTime, columnId, line_);
    }
    break;
    case Header_GPUBusy:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.gpuBusy, columnId, line_);
    }
    break;
    case Header_GPUWait:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.gpuWait, columnId, line_);
    }
    break;
    case Header_VideoBusy:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.videoBusy, columnId, line_);
    }
    break;
    case Header_DisplayLatency:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.displayLatency = convertedData;
        }
        else
        {
            v2MetricRow_.displayLatency.reset();
        }
    }
    break;
    case Header_DisplayedTime:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.displayedTime = convertedData;
        }
        else {
            v2MetricRow_.displayedTime.reset();
        }
    }
    break;
    case Header_AnimationError:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.animationError = convertedData;
        }
        else {
            v2MetricRow_.animationError.reset();
        }
    }
    break;
    case Header_AnimationTime:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.animationTime = convertedData;
        }
        else {
            v2MetricRow_.animationTime.reset();
        }
    }
    break;
    case Header_ClickToPhotonLatency:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.clickToPhotonLatency = convertedData;
        }
        else {
            v2MetricRow_.clickToPhotonLatency.reset();
        }
    }
    break;
    case Header_AllInputToPhotonLatency:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.AllInputToPhotonLatency = convertedData;
        }
        else {
            v2MetricRow_.AllInputToPhotonLatency.reset();
        }
    }
    break;
    case Header_InstrumentedLatency:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.InstrumentedLatency = convertedData;
        }
        else
        {
            v2MetricRow_.InstrumentedLatency.reset();
        }
    }
    break;

    default:
        Assert::Fail(CreateErrorString(UnknownHeader, line_).c_str());
    }

    return;
}

bool CsvParser::ReadRow(bool gatherMetrics)
{
    row_[0] = '\0';
    cols_.clear();

    // Read a line
    if (fgets(row_, _countof(row_), fp_) == nullptr) {
        if (ferror(fp_) != 0) {
            std::wstring errorMessage = L"File read error at line: ";
            errorMessage += std::to_wstring(line_);
            Assert::Fail(errorMessage.c_str());
        }
        return false;
    }

    line_ += 1;

    // Split line into columns, skipping leading/trailing whitespace
    auto p0 = row_;
    for (; *p0 == ' ' || *p0 == '\t'; ++p0) *p0 = '\0';
    uint32_t columnId = 0;
    for (auto p = p0; ; ++p) {
        auto ch = *p;
        if (ch == ',' || ch == '\0') {
            *p = '\0';
            cols_.push_back(p0);
            if (gatherMetrics) {
                ConvertToMetricDataType(p0, activeColHeadersMap_[columnId++]);
            }
            for (p0 = p + 1; *p0 == ' ' || *p0 == '\t'; ++p0) *p0 = '\0';
            for (auto q = p - 1; *q == ' ' || *q == '\t' || *q == '\n' || *q == '\r'; --q) *q = '\0';
            if (ch == '\0') break;
        }
    }

    return true;
}

size_t CsvParser::GetColumnIndex(char const* header)
{
    auto h = FindHeader(header);
    return h < KnownHeaderCount ? headerColumnIndex_[h] : SIZE_MAX;
}

Header CsvParser::FindHeader(char const* header)
{
    for (uint32_t i = 0; i < KnownHeaderCount; ++i) {
        auto h = (Header)i;
        if (strcmp(header, GetHeaderString(h)) == 0) {
            return h;
        }
    }
    return UnknownHeader;
}

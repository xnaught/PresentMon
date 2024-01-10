// Copyright (C) 2020-2023 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <windows.h>

struct PresentMonCsv
{
    enum Header {
        // Common:
        Header_Application,
        Header_ProcessID,
        Header_SwapChainAddress,
        Header_Runtime,
        Header_SyncInterval,
        Header_PresentFlags,
        Header_CPUFrameTime,
        Header_CPUFrameQPC,
        Header_CPUFrameQPCTime,
        Header_CPUFrameDateTime,
        Header_CPUDuration,
        Header_CPUFramePacingStall,
        Header_AllowsTearing,
        Header_PresentMode,
        Header_DisplayLatency,
        Header_DisplayDuration,
        Header_GPULatency,
        Header_GPUDuration,
        Header_GPUBusy,
        Header_VideoBusy,
        Header_InputLatency,

        // --v1_metrics
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

    static constexpr char const* GetHeaderString(Header h)
    {
        switch (h) {
        case Header_Application:            return "Application";
        case Header_ProcessID:              return "ProcessID";
        case Header_SwapChainAddress:       return "SwapChainAddress";
        case Header_Runtime:                return "Runtime";
        case Header_SyncInterval:           return "SyncInterval";
        case Header_PresentFlags:           return "PresentFlags";
        case Header_CPUFrameTime:           return "CPUFrameTime";
        case Header_CPUFrameQPC:            return "CPUFrameQPC";
        case Header_CPUFrameQPCTime:        return "CPUFrameQPCTime";
        case Header_CPUFrameDateTime:       return "CPUFrameDateTime";
        case Header_CPUDuration:            return "CPUDuration";
        case Header_CPUFramePacingStall:    return "CPUFramePacingStall";
        case Header_AllowsTearing:          return "AllowsTearing";
        case Header_PresentMode:            return "PresentMode";
        case Header_DisplayLatency:         return "DisplayLatency";
        case Header_DisplayDuration:        return "DisplayDuration";
        case Header_GPULatency:             return "GPULatency";
        case Header_GPUDuration:            return "GPUDuration";
        case Header_GPUBusy:                return "GPUBusy";
        case Header_VideoBusy:              return "VideoBusy";
        case Header_InputLatency:           return "InputLatency";
        case Header_Dropped:                return "Dropped";
        case Header_TimeInSeconds:          return "TimeInSeconds";
        case Header_msBetweenPresents:      return "msBetweenPresents";
        case Header_msInPresentAPI:         return "msInPresentAPI";
        case Header_msBetweenDisplayChange: return "msBetweenDisplayChange";
        case Header_msUntilRenderComplete:  return "msUntilRenderComplete";
        case Header_msUntilDisplayed:       return "msUntilDisplayed";
        case Header_msUntilRenderStart:     return "msUntilRenderStart";
        case Header_msGPUActive:            return "msGPUActive";
        case Header_msGPUVideoActive:       return "msGPUVideoActive";
        case Header_msSinceInput:           return "msSinceInput";
        case Header_QPCTime:                return "QPCTime";
        case Header_WasBatched:             return "WasBatched";
        case Header_DwmNotified:            return "DwmNotified";
        }
        return "<unknown>";
    }

    std::wstring path_;
    size_t line_ = 0;
    FILE* fp_ = nullptr;

    // headerColumnIndex_[h] is the file column index where h was found, or SIZE_MAX if
    // h wasn't found in the file.
    size_t headerColumnIndex_[KnownHeaderCount];

    char row_[1024];
    std::vector<char const*> cols_;
    std::vector<wchar_t const*> params_;

    bool Open(char const* file, int line, std::wstring const& path);
    void Close();
    bool ReadRow();

    size_t GetColumnIndex(char const* header) const;
};

#define CSVOPEN(_P) Open(__FILE__, __LINE__, _P)

struct PresentMon : PROCESS_INFORMATION {
    static std::wstring exePath_;
    std::wstring cmdline_;
    bool csvArgSet_;

    PresentMon();
    ~PresentMon();

    void AddEtlPath(std::wstring const& etlPath);
    void AddCsvPath(std::wstring const& csvPath);
    void Add(wchar_t const* args);
    void Start(char const* file, int line);

    // Returns true if the process is still running for timeoutMilliseconds
    bool IsRunning(DWORD timeoutMilliseconds=0) const;

    // Expect the process to exit with expectedExitCode within
    // timeoutMilliseconds (or kill it otherwise).
    void ExpectExited(char const* file, int line, DWORD timeoutMilliseconds=INFINITE, DWORD expectedExitCode=0);
};

#define PMSTART() Start(__FILE__, __LINE__)
#define PMEXITED(...) ExpectExited(__FILE__, __LINE__, __VA_ARGS__)

// PresentMonTests.cpp
extern std::wstring outDir_;
extern bool reportAllCsvDiffs_;
extern bool warnOnMissingCsv_;
extern std::wstring diffPath_;

bool EnsureDirectoryCreated(std::wstring path);
std::string Convert(std::wstring const& s);
std::wstring Convert(std::string const& s);

// PresentMon.cpp
void AddTestFailure(char const* file, int line, char const* fmt, ...);

// GoldEtlCsvTests.cpp
void AddGoldEtlCsvTests(std::wstring const& dir, size_t relIdx);

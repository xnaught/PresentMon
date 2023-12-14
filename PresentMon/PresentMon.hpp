// Copyright (C) 2017,2019-2023 Intel Corporation
// SPDX-License-Identifier: MIT

#pragma once

/*
ETW Architecture:

    Controller -----> Trace Session <----- Providers (e.g., DXGI, D3D9, DXGK, DWM, Win32K)
                           |
                           \-------------> Consumers (e.g., ../PresentData/PresentMonTraceConsumer)

PresentMon Architecture:

    MainThread: starts and stops the trace session and coordinates user
    interaction.

    ConsumerThread: is controlled by the trace session, and collects and
    analyzes ETW events.

    OutputThread: is controlled by the trace session, and outputs analyzed
    events to the CSV and/or console.

The trace session and ETW analysis is always running, but whether or not
collected data is written to the CSV file(s) is controlled by a recording state
which is controlled from MainThread based on user input or timer.
*/

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../PresentData/PresentMonTraceSession.hpp"

#include <unordered_map>

// Verbosity of console output for normal operation:
//     None = none
//     Simple = recording changes, etc.
//     Full = statistics about captured presents
enum class ConsoleOutput {
    None,
    Simple,
    Full
};

struct CommandLineArgs {
    std::vector<std::wstring> mTargetProcessNames;
    std::vector<std::wstring> mExcludeProcessNames;
    const wchar_t *mOutputCsvFileName;
    const wchar_t *mEtlFileName;
    const wchar_t *mSessionName;
    UINT mTargetPid;
    UINT mDelay;
    UINT mTimer;
    UINT mHotkeyModifiers;
    UINT mHotkeyVirtualKeyCode;
    ConsoleOutput mConsoleOutputType;
    bool mTrackDisplay;
    bool mTrackInput;
    bool mTrackGPU;
    bool mTrackGPUVideo;
    bool mOutputCsvToFile;
    bool mOutputCsvToStdout;
    bool mOutputQpcTime;
    bool mOutputQpcTimeInSeconds;
    bool mOutputDateTime;
    bool mScrollLockIndicator;
    bool mExcludeDropped;
    bool mTerminateExistingSession;
    bool mTerminateOnProcExit;
    bool mStartTimer;
    bool mTerminateAfterTimer;
    bool mHotkeySupport;
    bool mTryToElevate;
    bool mMultiCsv;
    bool mStopExistingSession;
};

// Metrics computed per-frame
struct FrameMetrics {
    double msBetweenPresents;
    double msInPresentApi;
    double msUntilRenderComplete;
    double msUntilDisplayed;
    double msBetweenDisplayChange;
    double msUntilRenderStart;
    double msGPUDuration;
    double msVideoDuration;
    double msSinceInput;
};

// We store SwapChainData per process and per swapchain, where we maintain:
// - information on previous presents needed for console output or to compute metrics for upcoming
//   presents,
// - exponential averages of key metrics displayed in console output.
struct SwapChainData {
    // Present info
    std::shared_ptr<PresentEvent> mLastPresented;
    std::shared_ptr<PresentEvent> mLastDisplayed;

    // Frame statistics
    float mAvgCPUDuration = 0.f;
    float mAvgGPUDuration = 0.f;
    float mAvgDisplayLatency = 0.f;
    float mAvgDisplayDuration = 0.f;
};

struct ProcessInfo {
    std::wstring mModuleName;
    std::unordered_map<uint64_t, SwapChainData> mSwapChain;
    HANDLE mHandle;
    FILE* mOutputCsv;
    bool mIsTargetProcess;
};

// CommandLine.cpp:
bool ParseCommandLine(int argc, wchar_t** argv);
CommandLineArgs const& GetCommandLineArgs();

// Console.cpp:
void InitializeConsole();
void FinalizeConsole();
bool StdOutIsConsole();
bool BeginConsoleUpdate();
void EndConsoleUpdate();
void ConsolePrint(wchar_t const* format, ...);
void ConsolePrintLn(wchar_t const* format, ...);
void UpdateConsole(uint32_t processId, ProcessInfo const& processInfo);
int PrintWarning(wchar_t const* format, ...);
int PrintError(wchar_t const* format, ...);

// ConsumerThread.cpp:
void StartConsumerThread(TRACEHANDLE traceHandle);
void WaitForConsumerThreadToExit();

// CsvOutput.cpp:
void IncrementRecordingCount();
FILE* GetOutputCsv(ProcessInfo* processInfo, uint32_t processId);
void CloseOutputCsv(ProcessInfo* processInfo);
void UpdateCsv(PMTraceSession const& pmSession, ProcessInfo* processInfo, PresentEvent const& p, FrameMetrics const& metrics);
const char* FinalStateToDroppedString(PresentResult res);
const char* PresentModeToString(PresentMode mode);
const char* RuntimeToString(Runtime rt);

// MainThread.cpp:
void ExitMainThread();

// OutputThread.cpp:
void StartOutputThread(PMTraceSession const& pmSession);
void StopOutputThread();
void SetOutputRecordingState(bool record);
void CanonicalizeProcessName(std::wstring* path);

// Privilege.cpp:
bool InPerfLogUsersGroup();
bool EnableDebugPrivilege();
int RestartAsAdministrator(int argc, wchar_t** argv);


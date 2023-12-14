// Copyright (C) 2019-2023 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMon.hpp"

#include <algorithm>
#include <shlwapi.h>
#include <thread>

static std::thread gThread;
static bool gQuit = false;

// When we collect realtime ETW events, we don't receive the events in real
// time but rather sometime after they occur.  Since the user might be toggling
// recording based on realtime cues (e.g., watching the target application) we
// maintain a history of realtime record toggle events from the user.  When we
// consider recording an event, we can look back and see what the recording
// state was at the time the event actually occurred.
//
// gRecordingToggleHistory is a vector of QueryPerformanceCounter() values at
// times when the recording state changed, and gIsRecording is the recording
// state at the current time.
//
// CRITICAL_SECTION used as this is expected to have low contention (e.g., *no*
// contention when capturing from ETL).

static CRITICAL_SECTION gRecordingToggleCS;
static std::vector<uint64_t> gRecordingToggleHistory;
static bool gIsRecording = false;

void SetOutputRecordingState(bool record)
{
    auto const& args = GetCommandLineArgs();

    EnterCriticalSection(&gRecordingToggleCS);

    if (gIsRecording != record) {
        gIsRecording = record;

        // When capturing from an ETL file, just use the current recording state.
        // It's not clear how best to map realtime to ETL QPC time, and there
        // aren't any realtime cues in this case.
        if (args.mEtlFileName == nullptr) {
            uint64_t qpc = 0;
            QueryPerformanceCounter((LARGE_INTEGER*) &qpc);
            gRecordingToggleHistory.emplace_back(qpc);
        }
    }

    LeaveCriticalSection(&gRecordingToggleCS);
}

static bool CopyRecordingToggleHistory(std::vector<uint64_t>* recordingToggleHistory)
{
    EnterCriticalSection(&gRecordingToggleCS);
    recordingToggleHistory->assign(gRecordingToggleHistory.begin(), gRecordingToggleHistory.end());
    auto isRecording = gIsRecording;
    LeaveCriticalSection(&gRecordingToggleCS);

    auto recording = recordingToggleHistory->size() + (isRecording ? 1 : 0);
    return (recording & 1) == 1;
}

// Remove recording toggle events that we've processed.
static void UpdateRecordingToggles(size_t nextIndex)
{
    if (nextIndex > 0) {
        EnterCriticalSection(&gRecordingToggleCS);
        gRecordingToggleHistory.erase(gRecordingToggleHistory.begin(), gRecordingToggleHistory.begin() + nextIndex);
        LeaveCriticalSection(&gRecordingToggleCS);
    }
}

// Processes are handled differently when running in realtime collection vs.
// ETL collection.  When reading an ETL, we receive NT_PROCESS events whenever
// a process is created or exits which we use to update the active processes.
//
// When collecting events in realtime and with elevated privilege, we should
// get similar ProcessStart/ProcessStop events, but only if PresentMon is
// running when the process started/stopped.  If we don't have elevated
// privilege or we missed a process start/stop we update the active processes
// whenever we notice an event with a new process id.  If it's a target
// process, we obtain a handle to the process, and periodically check it to see
// if it has exited.

static std::unordered_map<uint32_t, ProcessInfo> gProcesses;
static uint32_t gTargetProcessCount = 0;

// Removes any directory and extension, and converts the remaining name to
// lower case.
void CanonicalizeProcessName(std::wstring* name)
{
    size_t i = name->find_last_of(L"./\\");
    if (i != std::wstring::npos && (*name)[i] == L'.') {
        name->resize(i);
        i = name->find_last_of(L"/\\");
    }

    *name = name->substr(i + 1);

    std::transform(name->begin(), name->end(), name->begin(),
                   [](wchar_t c) { return (wchar_t) ::towlower(c); });
}

static bool IsTargetProcess(uint32_t processId, std::wstring const& processName)
{
    auto const& args = GetCommandLineArgs();

    std::wstring compareName;
    if (args.mExcludeProcessNames.size() + args.mTargetProcessNames.size() > 0) {
        compareName = processName;
        CanonicalizeProcessName(&compareName);
    }

    // -exclude
    for (auto excludeProcessName : args.mExcludeProcessNames) {
        if (excludeProcessName == compareName) {
            return false;
        }
    }

    // -capture_all
    if (args.mTargetPid == 0 && args.mTargetProcessNames.empty()) {
        return true;
    }

    // -process_id
    if (args.mTargetPid != 0 && args.mTargetPid == processId) {
        return true;
    }

    // -process_name
    for (auto targetProcessName : args.mTargetProcessNames) {
        if (targetProcessName == compareName) {
            return true;
        }
    }

    return false;
}

static ProcessInfo CreateProcessInfo(uint32_t processId, HANDLE handle, std::wstring const& processName)
{
    auto isTarget = IsTargetProcess(processId, processName);
    if (isTarget) {
        gTargetProcessCount += 1;
    }

    ProcessInfo info;
    info.mHandle          = handle;
    info.mModuleName      = processName;
    info.mOutputCsv       = nullptr;
    info.mIsTargetProcess = isTarget;
    return info;
}

static ProcessInfo* GetProcessInfo(uint32_t processId)
{
    auto ii = gProcesses.find(processId);
    if (ii == gProcesses.end()) {

        // In case we didn't get a ProcessStart event for this process (e.g.,
        // if the process started before PresentMon did) try to open a limited
        // handle into the process in order to query its name and also
        // periodically check if it has terminated.  This will fail (with
        // GetLastError() == ERROR_ACCESS_DENIED) if the process was run on
        // another account, unless we're running with SeDebugPrivilege.
        auto const& args = GetCommandLineArgs();
        HANDLE handle = NULL;
        wchar_t const* processName = L"<error>";
        if (args.mEtlFileName == nullptr) {
            wchar_t path[MAX_PATH];
            DWORD numChars = _countof(path);
            handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
            if (QueryFullProcessImageName(handle, 0, path, &numChars)) {
                processName = PathFindFileName(path);
            }
        }

        ii = gProcesses.emplace(processId, CreateProcessInfo(processId, handle, processName)).first;
    }

    return &ii->second;
}

// Check if any realtime processes terminated and add them to the terminated
// list.
//
// We assume that the process terminated now, which is wrong but conservative
// and functionally ok because no other process should start with the same PID
// as long as we're still holding a handle to it.
static void CheckForTerminatedRealtimeProcesses(std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses)
{
    for (auto& pair : gProcesses) {
        auto processId = pair.first;
        auto processInfo = &pair.second;

        DWORD exitCode = 0;
        if (processInfo->mHandle != NULL && GetExitCodeProcess(processInfo->mHandle, &exitCode) && exitCode != STILL_ACTIVE) {
            uint64_t qpc = 0;
            QueryPerformanceCounter((LARGE_INTEGER*) &qpc);
            terminatedProcesses->emplace_back(processId, qpc);
            CloseHandle(processInfo->mHandle);
            processInfo->mHandle = NULL;
        }
    }
}

static void HandleTerminatedProcess(uint32_t processId)
{
    auto const& args = GetCommandLineArgs();

    auto iter = gProcesses.find(processId);
    if (iter == gProcesses.end()) {
        return; // shouldn't happen.
    }

    auto processInfo = &iter->second;
    if (processInfo->mIsTargetProcess) {
        // Close this process' CSV.
        CloseMultiCsv(processInfo);

        // Quit if this is the last process tracked for -terminate_on_proc_exit.
        gTargetProcessCount -= 1;
        if (args.mTerminateOnProcExit && gTargetProcessCount == 0) {
            ExitMainThread();
        }
    }

    gProcesses.erase(std::move(iter));
}

static void UpdateProcesses(std::vector<ProcessEvent> const& processEvents, std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses)
{
    for (auto const& e : processEvents) {
        auto ii = gProcesses.find(e.ProcessId);
        if (ii == gProcesses.end()) {
            gProcesses.emplace(e.ProcessId, CreateProcessInfo(e.ProcessId, NULL, e.ImageFileName));
        }

        if (!e.IsStartEvent) {
            // Note any process termination in terminatedProcess, to be handled
            // once the present event stream catches up to the termination time.
            terminatedProcesses->emplace_back(e.ProcessId, e.QpcTime);
        }
    }
}

static void UpdateAverage(float* avg, double value)
{
    float constexpr expAvgScale = 0.0165f; // similar result to 120-present moving average

    if (value == 0.0) {
        *avg = 0.f;
    } else if (*avg == 0.f) {
        *avg = float(value);
    } else {
        *avg = (1.f - expAvgScale) * *avg + expAvgScale * float(value);
    }
}

static void AddPresents(
    PMTraceSession const& pmSession,
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    size_t* presentEventIndex,
    bool recording,
    bool checkStopQpc,
    uint64_t stopQpc,
    bool* hitStopQpc)
{
    auto const& args = GetCommandLineArgs();
    auto computeAvg = args.mConsoleOutput == ConsoleOutput::Statistics;

    auto i = *presentEventIndex;
    for (auto n = presentEvents.size(); i < n; ++i) {
        auto p = presentEvents[i];
        assert(p->IsCompleted);
        auto presented = p->FinalState == PresentResult::Presented;

        // Stop processing events if we hit the next stop time.
        if (checkStopQpc && p->PresentStartTime >= stopQpc) {
            *hitStopQpc = true;
            break;
        }

        // Look up the process this present belongs to; if not a target simply
        // discard the present.
        auto processInfo = GetProcessInfo(p->ProcessId);
        if (!processInfo->mIsTargetProcess) {
            continue;
        }

        // Look up the swapchain this present belongs to.
        auto chain = &processInfo->mSwapChain[p->SwapChainAddress];

        // If we are recording or presenting metrics to console, and there is at least one previous
        // present (required to compute metrics) then update the metrics
        //
        // Note: We avoid branching here for performance, so some of these calculations will be
        // meaningless depending on the present and options.  E.g., metrics.msUntilDisplayed will
        // only be valid if presented, and metrics.msUntilRenderStart will only be valid if
        // args.mTrackGPU
        if ((recording || computeAvg) && chain->mLastPresented != nullptr) {
            uint64_t lastPresentedStartTime  = chain->mLastPresented->PresentStartTime;
            uint64_t lastDisplayedScreenTime = chain->mLastDisplayed == nullptr ? 0 : chain->mLastDisplayed->ScreenTime;

            FrameMetrics metrics;
            metrics.msBetweenPresents      = pmSession.QpcDeltaToUnsignedMilliSeconds(lastPresentedStartTime, p->PresentStartTime);
            metrics.msInPresentApi         = pmSession.QpcDeltaToUnsignedMilliSeconds(p->PresentStartTime, p->PresentStopTime);
            metrics.msUntilRenderComplete  = pmSession.QpcDeltaToMilliSeconds(p->PresentStartTime, p->ReadyTime);
            metrics.msUntilDisplayed       = pmSession.QpcDeltaToUnsignedMilliSeconds(p->PresentStartTime, p->ScreenTime);
            metrics.msBetweenDisplayChange = pmSession.QpcDeltaToUnsignedMilliSeconds(lastDisplayedScreenTime, p->ScreenTime);
            metrics.msUntilRenderStart     = pmSession.QpcDeltaToMilliSeconds(p->PresentStartTime, p->GPUStartTime);
            metrics.msGPUDuration          = pmSession.QpcDeltaToMilliSeconds(p->GPUDuration);
            metrics.msVideoDuration        = pmSession.QpcDeltaToMilliSeconds(p->GPUVideoDuration);
            metrics.msSinceInput           = p->InputTime == 0 ? 0 : pmSession.QpcDeltaToMilliSeconds(p->PresentStartTime - p->InputTime);

            if (recording) {
                UpdateCsv(pmSession, processInfo, *p, metrics);
            }

            if (computeAvg) {
                UpdateAverage(&chain->mAvgCPUDuration, metrics.msBetweenPresents);
                UpdateAverage(&chain->mAvgGPUDuration, metrics.msGPUDuration);
                if (presented) {
                    UpdateAverage(&chain->mAvgDisplayLatency, pmSession.QpcDeltaToMilliSeconds(p->PresentStartTime, p->ScreenTime));
                    if (lastDisplayedScreenTime != 0) {
                        UpdateAverage(&chain->mAvgDisplayDuration, metrics.msBetweenDisplayChange);
                    }
                }
            }
        }

        // Update chain presents
        chain->mLastPresented = p;
        if (presented) {
            chain->mLastDisplayed = p;
        }
    }

    *presentEventIndex = i;
}

static void PruneOldSwapChainData(
    PMTraceSession const& pmSession,
    std::vector<ProcessEvent> const& processEvents,
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents)
{
    assert(processEvents.size() + presentEvents.size() > 0);

    auto latestQpc = std::max(
        processEvents.empty() ? 0ull : processEvents.back().QpcTime,
        presentEvents.empty() ? 0ull : presentEvents.back()->PresentStartTime);

    auto minQpc = latestQpc - pmSession.MilliSecondsDeltaToQpc(4000.0);

    for (auto& pair : gProcesses) {
        auto processInfo = &pair.second;
        for (auto ii = processInfo->mSwapChain.begin(), ie = processInfo->mSwapChain.end(); ii != ie; ) {
            auto chain = &ii->second;
            if (chain->mLastPresented == nullptr || chain->mLastPresented->PresentStartTime < minQpc) {
                ii = processInfo->mSwapChain.erase(ii);
            } else {
                ++ii;
            }
        }
    }
}

static void ProcessEvents(
    PMTraceSession const& pmSession,
    std::vector<ProcessEvent> const& processEvents,
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    std::vector<uint64_t>* recordingToggleHistory,
    std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses)
{
    auto const& args = GetCommandLineArgs();

    // Copy the record range history form the MainThread.
    auto recording = CopyRecordingToggleHistory(recordingToggleHistory);

    // Handle Process events; created processes are added to gProcesses and
    // terminated processes are added to terminatedProcesses.
    //
    // Handling of terminated processes need to be deferred until we observe a
    // present event that started after the termination time.  This is because
    // while a present must start before termination, it can complete after
    // termination.
    //
    // We don't have to worry about the recording toggles here because
    // NTProcess events are only captured when parsing ETL files and we don't
    // use recording toggle history for ETL files.
    UpdateProcesses(processEvents, terminatedProcesses);

    // Next, iterate through the recording toggles (if any)...
    size_t presentEventIndex = 0;
    size_t recordingToggleIndex = 0;
    size_t terminatedProcessIndex = 0;
    for (;;) {
        auto checkRecordingToggle   = recordingToggleIndex < recordingToggleHistory->size();
        auto nextRecordingToggleQpc = checkRecordingToggle ? (*recordingToggleHistory)[recordingToggleIndex] : 0ull;
        auto hitNextRecordingToggle = false;

        // First iterate through the terminated process history up until the
        // next recording toggle.  If we hit a present that started after the
        // termination, we can handle the process termination and continue.
        // Otherwise, we're done handling all the presents and any outstanding
        // terminations will have to wait for the next batch of events.
        for (; terminatedProcessIndex < terminatedProcesses->size(); ++terminatedProcessIndex) {
            auto const& pair = (*terminatedProcesses)[terminatedProcessIndex];
            auto terminatedProcessId = pair.first;
            auto terminatedProcessQpc = pair.second;

            if (checkRecordingToggle && nextRecordingToggleQpc < terminatedProcessQpc) {
                break;
            }

            auto hitTerminatedProcess = false;
            AddPresents(pmSession, presentEvents, &presentEventIndex, recording, true, terminatedProcessQpc, &hitTerminatedProcess);
            if (!hitTerminatedProcess) {
                goto done;
            }
            HandleTerminatedProcess(terminatedProcessId);
        }

        // Process present events up until the next recording toggle.  If we
        // reached the toggle, handle it and continue.  Otherwise, we're done
        // handling all the presents and any outstanding toggles will have to
        // wait for next batch of events.
        AddPresents(pmSession, presentEvents, &presentEventIndex, recording, checkRecordingToggle, nextRecordingToggleQpc, &hitNextRecordingToggle);
        if (!hitNextRecordingToggle) {
            break;
        }

        // Toggle recording.
        recordingToggleIndex += 1;
        recording = !recording;
        if (!recording) {
            IncrementRecordingCount();

            if (args.mMultiCsv) {
                for (auto& pair : gProcesses) {
                    CloseMultiCsv(&pair.second);
                }
            } else {
                CloseGlobalCsv();
            }
        }
    }

done:

    // Prune any SwapChainData that hasn't seen an update for over 4 seconds.
    PruneOldSwapChainData(pmSession, processEvents, presentEvents);

    // Finished processing all events.  Erase the recording toggles and
    // terminated processes that we also handled now.
    recordingToggleHistory->clear();
    UpdateRecordingToggles(recordingToggleIndex);
    if (terminatedProcessIndex > 0) {
        terminatedProcesses->erase(terminatedProcesses->begin(), terminatedProcesses->begin() + terminatedProcessIndex);
    }
}

void Output(PMTraceSession const* pmSession)
{
    auto const& args = GetCommandLineArgs();

    // Structures to track processes and statistics from recorded events.
    std::vector<ProcessEvent> processEvents;
    std::vector<std::shared_ptr<PresentEvent>> presentEvents;
    std::vector<std::shared_ptr<PresentEvent>> lostPresentEvents;
    std::vector<uint64_t> recordingToggleHistory;
    std::vector<std::pair<uint32_t, uint64_t>> terminatedProcesses;
    processEvents.reserve(128);
    presentEvents.reserve(4096);
    recordingToggleHistory.reserve(16);
    terminatedProcesses.reserve(16);

    for (;;) {
        // Read gQuit here, but then check it after processing queued events.
        // This ensures that we call Dequeue*() at least once after
        // events have stopped being collected so that all events are included.
        auto quit = gQuit;

        // Copy any information collected from ConsumerThread.
        pmSession->mPMConsumer->DequeueProcessEvents(processEvents);
        pmSession->mPMConsumer->DequeuePresentEvents(presentEvents);
        pmSession->mPMConsumer->DequeueLostPresentEvents(lostPresentEvents);
        lostPresentEvents.clear();

        // Process all the collected events, and update the various tracking
        // and statistics data structures.
        if (!processEvents.empty() || !presentEvents.empty()) {
            ProcessEvents(*pmSession, processEvents, presentEvents, &recordingToggleHistory, &terminatedProcesses);
            processEvents.clear();
            presentEvents.clear();
        }

        // Display information to console if requested.  If debug build and
        // simple console, print a heartbeat if recording.
        //
        // gIsRecording is the real timeline recording state.  Because we're
        // just reading it without correlation to gRecordingToggleHistory, we
        // don't need the critical section.
        auto realtimeRecording = gIsRecording;
        switch (args.mConsoleOutput) {
        #if _DEBUG
        case ConsoleOutput::Simple:
            if (realtimeRecording) {
                wprintf(L".");
            }
            break;
        #endif
        case ConsoleOutput::Statistics:
            if (BeginConsoleUpdate()) {
                for (auto const& pair : gProcesses) {
                    UpdateConsole(pair.first, pair.second);
                }

                if (realtimeRecording) {
                    ConsolePrintLn(L"** RECORDING **");
                }

                EndConsoleUpdate();
            }
            break;
        }

        // Everything is processed and output out at this point, so if we're
        // quiting we don't need to update the rest.
        if (quit) {
            break;
        }

        // Update tracking information.
        CheckForTerminatedRealtimeProcesses(&terminatedProcesses);

        // Sleep to reduce overhead.
        Sleep(100);
    }

    // Close all CSV and process handles
    for (auto& pair : gProcesses) {
        auto processInfo = &pair.second;
        if (processInfo->mHandle != NULL) {
            CloseHandle(processInfo->mHandle);
        }
        CloseMultiCsv(processInfo);
    }
    gProcesses.clear();
    CloseGlobalCsv();
}

void StartOutputThread(PMTraceSession const& pmSession)
{
    InitializeCriticalSection(&gRecordingToggleCS);
    gQuit = false;
    gThread = std::thread(Output, &pmSession); // Doesn't work to pass a reference, it makes a copy
}

void StopOutputThread()
{
    if (gThread.joinable()) {
        gQuit = true;
        gThread.join();

        DeleteCriticalSection(&gRecordingToggleCS);
    }
}


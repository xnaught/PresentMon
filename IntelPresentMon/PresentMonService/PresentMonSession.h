// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Windows.h>

#include <cmath>
#include <random>
#include <atomic>
#include <VersionHelpers.h>

#include "../ControlLib/PowerTelemetryProvider.h"
#include "../ControlLib/CpuTelemetry.h"
#include "../Streamer/Streamer.h"
#include "../../PresentData/PresentMonTraceConsumer.hpp"
#include "../../PresentData/PresentMonTraceSession.hpp"
#include "PowerTelemetryContainer.h"


struct SwapChainData {
    uint32_t mPresentHistoryCount;
    uint64_t mLastPresentQPC;
    uint64_t mLastDisplayedPresentQPC;
};

struct ProcessInfo {
    std::wstring mModuleName;
    std::unordered_map<uint64_t, SwapChainData> mSwapChain;
    HANDLE mHandle;
    bool mTargetProcess;
};

struct HandleDeleter {
    void operator()(HANDLE handle) const { CloseHandle(handle); }
};

class PresentMonSession {
public:

    virtual bool IsTraceSessionActive() = 0;
    virtual PM_STATUS StartStreaming(uint32_t client_process_id, uint32_t target_process_id, std::string& nsmFileName) = 0;
    virtual void StopStreaming(uint32_t client_process_id, uint32_t target_process_id) = 0;
    virtual bool CheckTraceSessions(bool forceTerminate) = 0;
    virtual HANDLE GetStreamingStartHandle() = 0;

    void SetCpu(const std::shared_ptr<pwr::cpu::CpuTelemetry>& pCpu);
    std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> EnumerateAdapters();
    std::string GetCpuName();
    double GetCpuPowerLimit();

    PM_STATUS SelectAdapter(uint32_t adapter_id);
    PM_STATUS SetGpuTelemetryPeriod(uint32_t period_ms);
    uint32_t GetGpuTelemetryPeriod();
    int GetActiveStreams();
    void SetPowerTelemetryContainer(PowerTelemetryContainer* ptc);

    // data
    std::wstring pm_session_name_;

    pwr::cpu::CpuTelemetry* cpu_ = nullptr;
    PowerTelemetryContainer* telemetry_container_ = nullptr;

    uint32_t current_telemetry_adapter_id_ = 0;

    // Set the initial telemetry period to 16ms
    uint32_t gpu_telemetry_period_ms_ = 16;

    Streamer streamer_;
};


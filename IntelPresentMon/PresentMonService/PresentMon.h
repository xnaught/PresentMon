// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "RealtimePresentMonSession.h"
#include "MockPresentMonSession.h"

class PresentMon {
 public:
  ~PresentMon();

  // Check the status of both ETW logfile and real time trace sessions.
  // When an ETW logfile has finished processing the associated
  // trace session must be destroyed to allow for other etl sessions
  // to be processed. In the case of real-time session if for some reason
  // there are zero active streams and a trace session is still active
  // clean it up.
  void CheckTraceSessions();

  // Force stop trace sessions
  void StopTraceSessions();

  PM_STATUS StartStreaming(uint32_t client_process_id,
                           uint32_t target_process_id,
                           std::string& nsm_file_name);
  void StopStreaming(uint32_t client_process_id, uint32_t target_process_id);

  std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> EnumerateAdapters();
  std::string GetCpuName() { return real_time_session_.GetCpuName(); }
  double GetCpuPowerLimit() { return real_time_session_.GetCpuPowerLimit(); }

  PM_STATUS SelectAdapter(uint32_t adapter_id);
  
  PM_STATUS SetGpuTelemetryPeriod(uint32_t period_ms) {
    // Only the real time trace sets GPU telemetry period
    return real_time_session_.SetGpuTelemetryPeriod(period_ms);

  }
  uint32_t GetGpuTelemetryPeriod() {
    // Only the real time trace sets GPU telemetry period
    return real_time_session_.GetGpuTelemetryPeriod();
  }

  void SetCpu(const std::shared_ptr<pwr::cpu::CpuTelemetry>& pCpu) {
    // Only the real time trace uses the control libary interface
    real_time_session_.SetCpu(pCpu);
  }

  HANDLE GetStreamingStartHandle() {
    // Only the real time trace uses the control libary interface
    return real_time_session_.GetStreamingStartHandle();
  }

  int GetActiveStreams() {
    // Only the real time trace uses the control libary interface
    return real_time_session_.GetActiveStreams();
  }

  void SetPowerTelemetryContainer(PowerTelemetryContainer* ptc) {
    // the real time trace session uses the control library
    // interface
    return real_time_session_.SetPowerTelemetryContainer(ptc);
  }

  void FlushEvents() {
      real_time_session_.FlushEvents();
  }

 private:
  RealtimePresentMonSession real_time_session_;
  MockPresentMonSession mock_session_;
};
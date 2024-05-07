// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "PresentMon.h"

#include "CliOptions.h"
#include "..\CommonUtilities\str\String.h"
#include <VersionHelpers.h>
#include <shlwapi.h>
#include <span>

PresentMon::~PresentMon() {
  real_time_session_.CheckTraceSessions(true);
  mock_session_.CheckTraceSessions(true);
}

PM_STATUS PresentMon::StartStreaming(uint32_t client_process_id,
    uint32_t target_process_id,
    std::string& nsm_file_name) {
    auto& opt = clio::Options::Get();

    if (opt.etlTestFile.AsOptional().has_value()) {
        return mock_session_.StartStreaming(client_process_id, target_process_id,
            nsm_file_name);
    }
    else
    {
        return real_time_session_.StartStreaming(client_process_id,
            target_process_id, nsm_file_name);
    }

}

void PresentMon::StopStreaming(uint32_t client_process_id,
                               uint32_t target_process_id) {
    auto& opt = clio::Options::Get();
    if (opt.etlTestFile.AsOptional().has_value()) {
        return mock_session_.StopStreaming(client_process_id, target_process_id);
    }
    else
    {
        return real_time_session_.StopStreaming(client_process_id, target_process_id);
    }
}

std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>>
PresentMon::EnumerateAdapters() {
  // Only the real time trace uses the control libary interface
  return real_time_session_.EnumerateAdapters();
}

PM_STATUS PresentMon::SelectAdapter(uint32_t adapter_id) {
  // Only the real time trace uses the control libary interface
  return real_time_session_.SelectAdapter(adapter_id);
}

void PresentMon::CheckTraceSessions() {
    real_time_session_.CheckTraceSessions(false);
    mock_session_.CheckTraceSessions(false);
}

void PresentMon::StopTraceSessions() {
    real_time_session_.CheckTraceSessions(true);
    mock_session_.CheckTraceSessions(true);
}

// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Windows.h>
#include <tchar.h>
#include <string>

#include <thread>
#include <vector>
#include <atomic>
#include <optional>

const int MaxBufferLength = MAX_PATH;
const int NumReportEventStrings = 2;

class Service
{
public:
	void SignalServiceStop(std::optional<int> errCode = {});
	std::optional<int> GetErrorCode() const;
	virtual HANDLE GetServiceStopHandle() = 0;
	virtual HANDLE GetResetPowerTelemetryHandle() = 0;
private:
	std::optional<int> errCode_;
};

class ConsoleDebugMockService : public Service
{
public:
	static ConsoleDebugMockService& Get();
	void Run();
	HANDLE GetServiceStopHandle() override;
	HANDLE GetResetPowerTelemetryHandle() override;
private:
	ConsoleDebugMockService();
	~ConsoleDebugMockService();
	static BOOL WINAPI ConsoleHandler(DWORD signal);
	HANDLE stopEvent_;
	HANDLE resetTelemetryEvent_;
};

class ConcreteService : public Service
{
public:
	ConcreteService(const TCHAR* serviceName);

  void ServiceMain();

  HANDLE GetServiceStopHandle() override { return mServiceStopEventHandle; }
  HANDLE GetResetPowerTelemetryHandle() override {
	  return mResetPowerTelemetryEventHandle;
  }
  void ReportServiceStatus(DWORD currentState, DWORD win32ExitCode,
                                  DWORD waitHint);
 private:
  void ServiceInit();

  SERVICE_STATUS mServiceStatus{};
  SERVICE_STATUS_HANDLE mServiceStatusHandle{};
  HANDLE mEventLogHandle = nullptr;
  std::wstring mServiceName;
  HANDLE mServiceStopEventHandle = nullptr;
  HANDLE mResetPowerTelemetryEventHandle = nullptr;
  HDEVNOTIFY mDeviceNotifyHandle = nullptr;
  DWORD mCheckPoint = 1;
  std::atomic<bool> mResetProviders = true;
};
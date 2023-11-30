// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Service.h"
#include "NamedPipeServer.h"
#include "PresentMon.h"
#include "PowerTelemetryContainer.h"
#include "..\ControlLib\WmiCpu.h"
#include "..\PresentMonUtils\StringUtils.h"
#include <filesystem>
#include "../Interprocess/source/Interprocess.h"
#include "CliOptions.h"

#define GOOGLE_GLOG_DLL_DECL
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

using namespace pmon;

void InitializeLogging(const char* servicename, const char* location, const char* basename,
                       const char* extension, int level) {
  // initialize glog
  if (!google::IsGoogleLoggingInitialized()) {
    google::InitGoogleLogging(servicename);
    if (!basename) {
      basename = "pmon-srv";
    }
    if (location) {
      google::SetLogDestination(
          google::GLOG_INFO,
          std::format("{}\\{}-info-", location, basename).c_str());
      google::SetLogDestination(
          google::GLOG_WARNING,
          std::format("{}\\{}-warn-", location, basename).c_str());
      google::SetLogDestination(
          google::GLOG_ERROR,
          std::format("{}\\{}-err-", location, basename).c_str());
      google::SetLogDestination(
          google::GLOG_FATAL,
          std::format("{}\\{}-fatal-", location, basename).c_str());
    }
    if (extension) {
      google::SetLogFilenameExtension(extension);
    }
    FLAGS_minloglevel = std::clamp(level, 0, 3);
  }
}

bool NanoSleep(int32_t ms) {
  HANDLE timer;
  LARGE_INTEGER li;
  // Convert from ms to 100ns units and negate
  int64_t ns = -10000 * (int64_t)ms;
  // Create a high resolution table
  if (!(timer = CreateWaitableTimerEx(NULL, NULL,
                                      CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                      TIMER_ALL_ACCESS))) {
    return false;
  }
  li.QuadPart = ns;
  if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
    CloseHandle(timer);
    return false;
  }
  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
  return true;
}

void SetupFileLogging()
{
    auto& opt = clio::Options::Get();
    auto& logDir = *opt.logDir;
    if (std::filesystem::is_directory(logDir)) {
        InitializeLogging(opt.GetName().c_str(), logDir.c_str(), "pm-srv", ".log", 0);
    }
}

// Attempt to use a high resolution sleep but if not
// supported use regular Sleep().
void PmSleep(int32_t ms) {
  if (!NanoSleep(ms)) {
    Sleep(ms);
  }
  return;
}

void IPCCommunication(Service* srv, PresentMon* pm)
{
    // alias for options
    auto& opt = clio::Options::Get();

    bool createNamedPipeServer = true;

    if (srv == nullptr) {
        // TODO: log
        return;
    }

    auto nps = std::make_unique<NamedPipeServer>(srv, pm, opt.controlPipe.AsOptional());
    if (!nps) {
        // TODO: log
        return;
    }

    while (createNamedPipeServer) {
            DWORD result = nps->RunServer();
            if (result == ERROR_SUCCESS) {
                createNamedPipeServer = false;
            }
            else {
                // We were unable to start our named pipe server. Sleep for
                // a bit and then try again.
                PmSleep(3000);
            }
    }

    return;
}

void PowerTelemetry(Service* const srv, PresentMon* const pm,
	PowerTelemetryContainer* const ptc, ipc::ServiceComms* const pComms)
{
	if (srv == nullptr || pm == nullptr || ptc == nullptr) {
		// TODO: log error here
		return;
	}

	// this flag indicates that no clients have ever connected yet
	bool firstConnection = true;

	// Get the streaming start event
	const HANDLE events[] {
	  pm->GetStreamingStartHandle(),
	  srv->GetServiceStopHandle(),
	};
	while (1) {
		auto waitResult = WaitForMultipleObjects((DWORD)std::size(events), events, FALSE, INFINITE);
		// TODO: check for wait result error
		auto i = waitResult - WAIT_OBJECT_0;
		// if events[1] was signalled, that means service is stopping so exit thread
		if (i == 1) {
			return;
		}
		// if events[0] was signalled, a client has connected where previously there were 0
		// if this is the first connection, populate container and poll once for availability
		if (firstConnection) {
			// TODO: log error here or inside of repopulate
			ptc->Repopulate();
			for (auto& adapter : ptc->GetPowerTelemetryAdapters()) {
				adapter->Sample();
                // TODO: actually get vendor, need to make adapter use new vendor enum
                pComms->RegisterGpuDevice(PM_DEVICE_VENDOR_INTEL, adapter->GetName(), adapter->GetPowerTelemetryCapBits());
			}
            pComms->FinalizeGpuDevices();
			firstConnection = false;
		}
		while (WaitForSingleObject(srv->GetServiceStopHandle(), 0) != WAIT_OBJECT_0) {
			if (WaitForSingleObject(srv->GetResetPowerTelemetryHandle(), 0) == WAIT_OBJECT_0) {
				// TODO: log error here or inside of repopulate
				ptc->Repopulate();
			}
			for (auto& adapter : ptc->GetPowerTelemetryAdapters()) {
				adapter->Sample();
			}
			PmSleep(pm->GetGpuTelemetryPeriod());
			// Get the number of currently active streams
			auto num_active_streams = pm->GetActiveStreams();
			if (num_active_streams == 0) {
				break;
			}
		}
	}
}

void CpuTelemetry(Service* const srv, PresentMon* const pm,
	pwr::cpu::CpuTelemetry* const cpu)
{
	if (srv == nullptr || pm == nullptr) {
		// TODO: log error on this condition
		return;
	}

    const HANDLE events[] {
        pm->GetStreamingStartHandle(),
        srv->GetServiceStopHandle(),
    };

	while (1) {
		auto waitResult = WaitForMultipleObjects((DWORD)std::size(events), events, FALSE, INFINITE);
		auto i = waitResult - WAIT_OBJECT_0;
		if (i == 1) {
			return;
		}
		while (WaitForSingleObject(srv->GetServiceStopHandle(), 0) !=
			WAIT_OBJECT_0) {
			cpu->Sample();
			PmSleep(pm->GetGpuTelemetryPeriod());
			// Get the number of currently active streams
			auto num_active_streams = pm->GetActiveStreams();
			if (num_active_streams == 0) {
				break;
			}
		}
	}
}

DWORD WINAPI PresentMonMainThread(LPVOID lpParam)
{
    try {
        if (lpParam == nullptr) {
            return ERROR_INVALID_DATA;
        }

        // alias for options
        auto& opt = clio::Options::Get();

        // Extract out the PresentMon Service pointer
        const auto srv = static_cast<Service*>(lpParam);

        // Grab the stop service event handle
        const auto serviceStopHandle = srv->GetServiceStopHandle();

        // check if the service is to be debugged while starting up
        auto debug_service = opt.debug;

        // spin here waiting for debugger to attach, after which debugger should set
        // debug_service to false in order to proceed
        while (debug_service) {
            if (WaitForSingleObject(serviceStopHandle, 0) != WAIT_OBJECT_0) {
                PmSleep(500);
            }
            else {
                return ERROR_SUCCESS;
            }
        }

        if (opt.logDir) {
            SetupFileLogging();
        }

        PresentMon pm;
        PowerTelemetryContainer ptc;
        auto pComms = ipc::MakeServiceComms();

        // Set the created power telemetry container 
        pm.SetPowerTelemetryContainer(&ptc);

        // Start IPC communication thread
        std::jthread ipc_thread(IPCCommunication, srv, &pm);

        // Launch telemetry thread
        std::jthread telemetry_thread;

        try {
            telemetry_thread = std::jthread{ PowerTelemetry, srv, &pm, &ptc, pComms.get() };
        }
        catch (...) {}

        // Create CPU telemetry
        std::shared_ptr<pwr::cpu::CpuTelemetry> cpu;
        std::jthread cpu_telemetry_thread;
        try {
            // Try to use WMI for metrics sampling
            cpu = std::make_shared<pwr::cpu::wmi::WmiCpu>();
        }
        catch (const std::runtime_error& e) {
            LOG(INFO) << "WMI Failure Status: " << e.what() << std::endl;
        }
        catch (...) {}

        if (cpu) {
            cpu_telemetry_thread = std::jthread{ CpuTelemetry, srv, &pm, cpu.get() };
            pm.SetCpu(cpu);
            // register cpu telemetry info with introspection
            // TODO: get cpu vendor and pass info onto introspection
            pComms->RegisterCpuDevice(PM_DEVICE_VENDOR_INTEL, cpu->GetCpuName(), cpu->GetCpuTelemetryCapBits());
        }

        while (WaitForSingleObject(serviceStopHandle, 0) != WAIT_OBJECT_0) {
            pm.CheckTraceSessions();
            PmSleep(500);
        }

        // Stop the PresentMon session
        pm.StopTraceSession();
    }
    catch (...) {
        return E_FAIL;
    }

    return ERROR_SUCCESS;
}

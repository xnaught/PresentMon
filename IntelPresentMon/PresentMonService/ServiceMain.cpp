// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include <Windows.h>
#include <tchar.h>
#include <iostream>

#include "Service.h"
#include "CliOptions.h"

TCHAR serviceName[MaxBufferLength] = TEXT("Intel PresentMon Service");

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);

int __cdecl _tmain(int argc, TCHAR* argv[])
{
	if (auto e = clio::Options::Init()) {
		return *e;
	}
	auto& opt = clio::Options::Get();
	if (opt.testIntOption) {
		std::cout << "run as command line app now, option: " << *opt.testIntOption << std::endl;
		return 0;
	}

	const SERVICE_TABLE_ENTRY dispatchTable[] = {
		{serviceName, static_cast<LPSERVICE_MAIN_FUNCTION>(ServiceMain)},
		{NULL, NULL}
	};

	if (!StartServiceCtrlDispatcher(dispatchTable)) {
		return -1;
	}

	return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
  Service present_mon_service(serviceName);

  present_mon_service.ServiceMain(argc, argv);
}
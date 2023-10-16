// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#ifdef PRESENTMONAPI2_EXPORTS
#define PRESENTMON_API_EXPORT __declspec(dllexport)
#else
#define PRESENTMON_API_EXPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	enum PM_STATUS
	{
		PM_STATUS_SUCCESS = 0,
		PM_STATUS_FAILURE,
		PM_STATUS_SESSION_NOT_OPEN,
	};


	PRESENTMON_API_EXPORT PM_STATUS pmOpenSession();
	PRESENTMON_API_EXPORT PM_STATUS pmCloseSession();


#ifdef __cplusplus
} // extern "C"
#endif
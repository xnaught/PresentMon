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


PRESENTMON_API_EXPORT void pmSetMiddlewareAsMock(bool mocked);     
PRESENTMON_API_EXPORT void pmMiddlewareSpeak(char* buffer);


#ifdef __cplusplus
} // extern "C"
#endif
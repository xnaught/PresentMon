// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#ifdef PRESENTMONAPI2_EXPORTS
#define PRESENTMON_API_EXPORT __declspec(dllexport)
#else
#define PRESENTMON_API_EXPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
#define PM_ENUM enum class
extern "C" {
#else
#define PM_ENUM enum
#endif


PM_ENUM PM_TEST_ENUM
{
    PM_TEST_ENUM_1,
    PM_TEST_ENUM_2,
};
     
PRESENTMON_API_EXPORT int pmTest(int input);


#ifdef __cplusplus
} // extern "C"
#endif

#undef PM_ENUM
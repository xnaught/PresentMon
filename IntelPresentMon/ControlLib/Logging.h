// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <format>
#include "igcl_api.h"
#include "../CommonUtilities/log/Log.h"

namespace pwr::v
{
#ifndef VVV_GPU_TELEMETRY // system that reads power, temperature, etc. telemetry of graphics adapter devices
    inline constexpr bool gpu = false;
#else
    inline constexpr bool gpu = true;
#endif	
}

namespace pwr::log
{
    inline std::string MakeErrorLocationString(int line, const char* file, const char* function)
    {
        return std::format("{} at {}:{}", function, file, line);
    }

    inline std::string MakeIgclDebugErrorString(ctl_result_t code, int line, const char* file, const char* function)
    {
        try
        {
            return std::format("(TEL) IGCL_ERR [{}] in {}\n", (int)code, MakeErrorLocationString(line, file, function));
        } catch (...) {
            std::string exception_string = "(TEL) IGCL_ERR debug error string failed";
            return exception_string;
        }
    }

    inline std::string MakeTelemetryDebugErrorString(const char* msg, int line, const char* file, const char* function)
    {
        return std::format("(TEL) {} in {}\n", msg, MakeErrorLocationString(line, file, function));
    }
}

#define IGCL_ERR(ec) pmlog_error("IGCL").code((unsigned int)ec);
#define TELE_ERR(msg) OutputDebugStringA(pwr::log::MakeTelemetryDebugErrorString((msg), __LINE__, __FILE__, __FUNCTION__).c_str())
// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../../DataBindAccessor.h"
#include "../CefValues.h"
#include <CommonUtilities\str\String.h>

namespace p2c::client::util::async
{
    class LaunchKernel : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "launchKernel"; }
        LaunchKernel() : AsyncEndpoint{ AsyncEndpoint::Environment::RenderProcess } {}
        // {} => null
        Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor& accessor) const override
        {
            accessor.LaunchKernel();
            return Result{ true, CefValueNull() };
        }
    };
}
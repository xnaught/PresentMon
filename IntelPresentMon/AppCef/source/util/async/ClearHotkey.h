// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include <include/cef_task.h>
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"
#include "../../DataBindAccessor.h"
#include "../CefValues.h"

namespace p2c::client::util::async
{
    class ClearHotkey : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "clearHotkey"; }
        ClearHotkey() : AsyncEndpoint{ AsyncEndpoint::Environment::RenderProcess } {}
        // {action:int} => null
        Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor& accessor) const override
        {
            return Result{ accessor.ClearHotkey(*pArgObj), CefValueNull() };
        }
    };
}
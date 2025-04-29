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
	class BindHotkey : public AsyncEndpoint
	{
	public:
        static constexpr std::string GetKey() { return "bindHotkey"; }
        BindHotkey() : AsyncEndpoint{ AsyncEndpoint::Environment::RenderProcess } {}
        // {combination: {modifiers:[], hotkey:int}, action:int} => null
		Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor& accessor) const override
		{
			return Result{ accessor.BindHotkey(*pArgObj), CefValueNull() };
		}
	};
}
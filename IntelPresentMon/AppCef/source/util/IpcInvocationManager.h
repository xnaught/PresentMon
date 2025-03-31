// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/WinAPI.h>
#include <include/cef_v8.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <future>
#include <unordered_map>
#include "AsyncEndpointCollection.h"
#include "ActionClientServer.h"


namespace p2c::client::util
{
	// this class exists to:
	// A) bridge from key string to action
	// B) encode conversion from V8 to Params{}
	// C) encode conversion from Response{} to V8
	class IpcInvocationManager
	{
	public:
		// types
		struct CallbackContext
		{
			CefRefPtr<CefV8Value> pV8ResolveFunction;
			CefRefPtr<CefV8Value> pV8RejectFunction;
			CefRefPtr<CefV8Context> pV8Context;
		};
		struct Invocation
		{
			CallbackContext context;
		};
		struct Bindings
		{
			std::function<void(CefV8Value&)> dispatch;
		};
		// functions
		void DispatchInvocation(const std::string& key, CallbackContext ctx, CefRefPtr<CefV8Value> pObj, CefBrowser& browser, cef::DataBindAccessor& accessor, kern::Kernel& kernel);
		void ResolveInvocation(uint64_t uid, bool success, CefRefPtr<CefValue> pArgs);
		bool HasHandler(const std::string& key) const;
	private:
		uint64_t nextUid = 0;
		AsyncEndpointCollection endpoints;
		// todo: use separate mutex for endpoints+uid than for invocations
		std::mutex mtx;
		std::unordered_map<uint64_t, Invocation> invocations;
		CefClient& client;
	};
}

#undef KEY_LIST
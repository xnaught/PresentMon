// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/WinAPI.h>
#include <include/cef_v8.h>
#include <include/wrapper/cef_helpers.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <future>
#include <unordered_map>
#include "AsyncEndpointCollection.h"
#include "ActionClientServer.h"
#include "V8Transfer.h"


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
		// functions
		void DispatchInvocation(const std::string& key, CallbackContext ctx, CefRefPtr<CefV8Value> pArgs)
		{
			// TODO: explicit lookup and handle missing error
			dispatchBindings_[key](std::move(ctx), std::move(pArgs));
		}
		bool HasHandler(const std::string& key) const
		{
			return dispatchBindings_.contains(key);
		}
		template<class A>
		void RegisterDispatchBinding()
		{
			using Params = typename A::Params;
			using Response = typename A::Response;
			dispatchBindings_[A::Identifier] = [this](CallbackContext ctx, CefRefPtr<CefV8Value> pObj) {
				client_.DispatchDetachedWithContinuation(FromV8<Params>(*pObj), [ctx = std::move(ctx)](Response&& res) {
					CefPostTask(TID_RENDERER, base::BindOnce(
						&IpcInvocationManager::ResolveInvocation_<Response>,
						// TODO: populate an actual success here (catch exception somewhere?)
						true, std::move(ctx), std::move(res)
					));
				});
			};
		}
		IpcInvocationManager(CefClient& client) : client_{ client } {}
	private:
		// function
		template<class Response>
		static void ResolveInvocation_(bool success, CallbackContext ctx, Response res)
		{
			CEF_REQUIRE_RENDERER_THREAD();

			ctx.pV8Context->Enter();
			if (success) {
				ctx.pV8ResolveFunction->ExecuteFunction(nullptr, { ToV8(res) });
			}
			else {
				ctx.pV8RejectFunction->ExecuteFunction(nullptr, { ToV8(res) });
			}
			ctx.pV8Context->Exit();
		}
		// data
		std::unordered_map<std::string, std::function<void(CallbackContext, CefRefPtr<CefV8Value>)>> dispatchBindings_;
		CefClient& client_;
	};
}

#undef KEY_LIST
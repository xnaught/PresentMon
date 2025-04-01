// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/WinAPI.h>
#include <include/cef_v8.h>
#include <include/wrapper/cef_helpers.h>
#include <include/cef_task.h>
#include <include/base/cef_callback.h>
#include <include/wrapper/cef_closure_task.h>
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
			dispatchBindings_[key](std::move(ctx), std::move(pArgs), client_);
		}
		bool HasHandler(const std::string& key) const
		{
			return dispatchBindings_.contains(key);
		}
		template<class A>
		static void RegisterDispatchBinding()
		{
			using Params = typename A::Params;
			using Response = typename A::Response;
			// register handler function for the endpoint identifer (e.g. SetCapture) that takes in V8 context and V8 action args
			// this handler will be called when the DBA receives an Execute call on the invokeEndpoint channel
			dispatchBindings_[A::Identifier] = [](CallbackContext ctx, CefRefPtr<CefV8Value> pObj, CefClient& client) {
				// convert the V8 args to the Params struct associated with the action A
				// create a lambda which accepts the incoming result of this action and stores the V8 context
				// lamba will be invoked after the response is received from the server, causing the response
				// to be returned to the original V8 caller in js land
				client.DispatchDetachedWithContinuation(FromV8<Params>(*pObj), [ctx = std::move(ctx)](Response&& res) {
					CefPostTask(TID_RENDERER, base::BindOnce(
						&IpcInvocationManager::ResolveInvocation_<Response>,
						// TODO: populate an actual success here (catch exception somewhere?)
						// more likely the lambda should receive some error info to propagate
						// alternatively Dispatch... will throw error that should be caught providing
						// alternative resolution path below
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
		static std::unordered_map<std::string, std::function<void(CallbackContext, CefRefPtr<CefV8Value>, CefClient&)>> dispatchBindings_;
		CefClient& client_;
	};
}

#undef KEY_LIST
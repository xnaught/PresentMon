// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "DataBindAccessor.h"
#include "util/Logging.h"
#include <include/cef_task.h> 
#include <include/base/cef_callback.h> 
#include <include/wrapper/cef_closure_task.h> 
#include "util/CefValues.h"
#include <Interprocess/source/act/SymmetricActionServer.h>
#include "util/cact/TargetLostAction.h"
#include "util/cact/OverlayDiedAction.h"
#include "util/cact/PresentmonInitFailedAction.h"
#include "util/cact/StalePidAction.h"
#include "util/SignalManager.h"


namespace p2c::client::cef
{
    DataBindAccessor::DataBindAccessor(CefRefPtr<CefBrowser> pBrowser, util::KernelWrapper* pKernelWrapper_)
        :
        pBrowser{ std::move(pBrowser) },
        pKernelWrapper{ pKernelWrapper_ }
    {
        pKernelWrapper->pHotkeys = std::make_unique<util::Hotkeys>();
        // set the hotkey listener component to call hotkey signal on the signal manager when a hotkey chord is detected
        pKernelWrapper->pHotkeys->SetHandler([this](Action action) {
            CefPostTask(TID_RENDERER, base::BindOnce(&util::SignalManager::SignalHotkeyFired,
                base::Unretained(&pKernelWrapper->signals), uint32_t(action)));
        });
    }

    bool DataBindAccessor::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
    {
        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper)
        {
            // this cannot be made an async endpoint object because it deals with V8 function objects
            // signals are registered before kernel and do not require its existence
            if (name == "registerSignalHandler") {
                if (arguments.size() == 2 && arguments[0]->IsString() && arguments[1]->IsFunction()) {
                    pKernelWrapper->signals.RegisterCallback(arguments[0]->GetStringValue(), { arguments[1], CefV8Context::GetCurrentContext() });
                }
                else {
                    pmlog_error("registerSignalHandler called with incorrect parameter signature");
                    exception = "registerSignalHandler called with incorrect parameter signature";
                }
                return true;
            }

            // async endpoint entry function
            if (name == "invokeEndpoint") {
                if (arguments.size() == 4 && arguments[0]->IsString() && arguments[1]->IsObject() &&
                    arguments[2]->IsFunction() && arguments[3]->IsFunction())
                {
                    const auto& key = arguments[0]->GetStringValue();
                    if (pKernelWrapper->pInvocationManager->HasHandler(key)) {
                        pKernelWrapper->pInvocationManager->DispatchInvocation(key,
                            { arguments[2], arguments[3], CefV8Context::GetCurrentContext() },
                            arguments[1]
                        );
                    }
                    else {
                        pKernelWrapper->asyncEndpoints.DispatchInvocation(key,
                            { arguments[2], arguments[3], CefV8Context::GetCurrentContext() },
                            arguments[1],
                            *pBrowser,
                            *this
                        );
                    }
                }
                else
                {
                    pmlog_error("invokeEndpoint called with incorrect parameter signature");
                    exception = "invokeEndpoint called with incorrect parameter signature";
                }
                return true;
            }
        }
        return false;
    }

    void DataBindAccessor::ResolveAsyncEndpoint(uint64_t uid, bool success, CefRefPtr<CefValue> pArgs)
    {
        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper)
        {
            pKernelWrapper->asyncEndpoints.ResolveInvocation(uid, success, std::move(pArgs));
        }
    }

    bool DataBindAccessor::BindHotkey(CefValue& pArgObj)
    {
        // {action:int, combination: {modifiers:[], key:int}}

        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper) {
            const auto payload = pArgObj.GetDictionary();
            const auto comboJs = payload->GetDictionary("combination");
            const auto modsJs = comboJs->GetList("modifiers");

            auto mods = win::Mod::Null;
            for (int i = 0; i < modsJs->GetSize(); i++) {
                const auto modCode = modsJs->GetValue(i)->GetInt();
                mods = mods | *win::ModSet::SingleModFromCode(modCode);
            }

            return pKernelWrapper->pHotkeys->BindAction(
                (Action)payload->GetInt("action"),
                win::Key{ (win::Key::Code)comboJs->GetInt("key") },
                mods
            );
        }
        return false;
    }

    bool DataBindAccessor::ClearHotkey(CefValue& pArgObj)
    {
        // {action:int}

        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper) {
            return pKernelWrapper->pHotkeys->ClearAction(
                (Action)pArgObj.GetDictionary()->GetInt("action")
            );
        }
        return false;
    }

    void DataBindAccessor::ClearKernelWrapper()
    {
        std::lock_guard lk{ kernelMtx };
        pKernelWrapper = nullptr;
    }
}
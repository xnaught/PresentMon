// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NanoCefBrowserClient.h"
#include <include/wrapper/cef_helpers.h>
#include <Core/source/infra/Logging.h>
#include <format>
#include <fstream>
#include <streambuf>
#include "util/AsyncEndpointManager.h"
#include "NanoCefProcessHandler.h"

// implemented in winmain.cpp
void AppQuitMessageLoop();

using namespace pmon::util;

namespace p2c::client::cef
{
    NanoCefBrowserClient::NanoCefBrowserClient()
    {
        class NullMenuHandler : public CefContextMenuHandler {
        public:
            NullMenuHandler() = default;
            void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefContextMenuParams> params,
                CefRefPtr<CefMenuModel> model) override {
                // Clear the context menu to disable the default right-click behavior.
                model->Clear();
            }

            IMPLEMENT_REFCOUNTING(NullMenuHandler);
            DISALLOW_COPY_AND_ASSIGN(NullMenuHandler);
        };
        pContextMenuHandler = new NullMenuHandler{};
    }

    CefRefPtr<CefBrowser> NanoCefBrowserClient::GetBrowser()
    {
        return pBrowser;
    }

    CefRefPtr<CefLifeSpanHandler> NanoCefBrowserClient::GetLifeSpanHandler()
    {
        return this;
    }

    CefRefPtr<CefDisplayHandler> NanoCefBrowserClient::GetDisplayHandler()
    {
        return this;
    }

    void NanoCefBrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser_)
    {
        pBrowser = browser_;

        CefLifeSpanHandler::OnAfterCreated(std::move(browser_));
    }

    void NanoCefBrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser_)
    {
        pBrowser.reset();
        AppQuitMessageLoop();

        CefLifeSpanHandler::OnBeforeClose(browser_);
    }

    bool NanoCefBrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
    {
        if (message->GetName() == util::AsyncEndpointManager::GetDispatchMessageName())
        {
            if (auto pEndpoint = endpoints.Find(message->GetArgumentList()->GetString(0)))
            {
                pEndpoint->ExecuteOnBrowser(message->GetArgumentList()->GetInt(1), message->GetArgumentList()->GetValue(2), browser);
            }
            else
            {
                pmlog_warn(std::format(L"Endpoint async not found: {}", message->GetArgumentList()->GetString(0).ToWString()));
            }
            return true;
        }
        else if (message->GetName() == NanoCefProcessHandler::GetShutdownMessageName())
        {
            shutdownAcknowledgementFlag = true;
            // this causes another WM_CLOSE
            pBrowser->GetHost()->CloseBrowser(true);
            return true;
        }
        return false;
    }

    std::optional<LRESULT> NanoCefBrowserClient::HandleCloseMessage()
    {
        // if a previous close was sent and acknowledged to/from render process (or timed out), proceed with close
        if (shutdownAcknowledgementFlag && shutdownSemaphore.try_acquire()) {
            return {};
        }
        // shutdown was requested, but not acked yet (or timeout fired already)
        if (shutdownRequestFlag) {
            return 0;
        }
        // if this is the first close request, don't close and instead send request to render process
        else {
            shutdownRequestFlag = true;
            // send message to render process to destroy the kernel and kernel-associated objects
            auto msg = CefProcessMessage::Create(NanoCefProcessHandler::GetShutdownMessageName());
            pBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, std::move(msg));
            // set a timeout to force close if ack is not received from render process
            std::thread{ [this] {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1000ms);
                shutdownAcknowledgementFlag = true;
                pBrowser->GetHost()->CloseBrowser(true);
                pmlog_warn(L"Shutdown ack timed out");
            } }.detach();
            return 0;
        }
    }

    CefRefPtr<CefContextMenuHandler> NanoCefBrowserClient::GetContextMenuHandler()
    {
        return pContextMenuHandler;
    }

#define xjs_pmlog_(lvl) ((PMLOG_BUILD_LEVEL < lvl) || (::pmon::util::log::GlobalPolicy::Get().GetLogLevel() < lvl)) \
	? (void)0 : (void)::pmon::util::log::EntryBuilder{ lvl, source.ToWString(), {}, line } \
	.to(::pmon::util::log::GetDefaultChannel()).no_trace().note(message.ToWString())

    bool NanoCefBrowserClient::OnConsoleMessage(
        CefRefPtr<CefBrowser> browser,
        cef_log_severity_t level,
        const CefString& message,
        const CefString& source,
        int line)
    {
        switch (level) {
        case cef_log_severity_t::LOGSEVERITY_FATAL:
            xjs_pmlog_(log::Level::Fatal); break;
        case cef_log_severity_t::LOGSEVERITY_ERROR:
            xjs_pmlog_(log::Level::Error); break;
        case cef_log_severity_t::LOGSEVERITY_WARNING:
            xjs_pmlog_(log::Level::Warning); break;
        case cef_log_severity_t::LOGSEVERITY_DEFAULT:
        case cef_log_severity_t::LOGSEVERITY_INFO:
            xjs_pmlog_(log::Level::Info); break;
        case cef_log_severity_t::LOGSEVERITY_DEBUG:
            xjs_pmlog_(log::Level::Debug); break;
        }
        return true;
    }
}
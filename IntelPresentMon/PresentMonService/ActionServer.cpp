// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "..\CommonUtilities\str\String.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../Interprocess/source/act/AsyncActionManager.h"
#include <boost/asio/windows/object_handle.hpp>
#include "CliOptions.h"
#include "ActionServer.h"
#include "GlobalIdentifiers.h"
#include <thread>
#include <sddl.h>


namespace pmon::svc
{
    using namespace std::literals;
    using namespace pmon;
    using namespace util;
    using namespace ipc;
    namespace as = boost::asio;

    class ActionServerImpl_
    {
    public:
        ActionServerImpl_(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName)
            :
            pipeName_{ pipeName.value_or(gid::defaultControlPipeName) },
            elevatedSecurity_{ !pipeName },
            stopEvent_{ ioctx_, win::Handle::CreateCloned(pSvc->GetServiceStopHandle()).Release() }
        {
            actionManager_.ctx_.pSvc = pSvc;
            actionManager_.ctx_.pPmon = pPmon;
            thread_ = std::jthread{ [this] {
                // coroutine to exit thread on stop signal
                as::co_spawn(ioctx_, [this]() -> as::awaitable<void> {
                    co_await stopEvent_.async_wait(as::use_awaitable);
                    pmlog_dbg("ActionServer received stop signal");
                    ioctx_.stop();
                }, as::detached);
                // maintain 3 available pipe instances at all times
                for (int i = 0; i < 3; i++) {
                    as::co_spawn(ioctx_, AcceptConnection_(), as::detached);
                }
                // run the io context event handler until signalled to exit
                ioctx_.run();
                pmlog_info("ActionServer exiting");
            } };
        }
        ActionServerImpl_(const ActionServerImpl_&) = delete;
        ActionServerImpl_& operator=(const ActionServerImpl_&) = delete;
        ActionServerImpl_(ActionServerImpl_&&) = delete;
        ActionServerImpl_& operator=(ActionServerImpl_&&) = delete;
        ~ActionServerImpl_() = default;
    private:
        // functions
        as::awaitable<void> AcceptConnection_()
        {
            try {
                // create pipe instance object
                auto pipe = pipe::DuplexPipe::Make(pipeName_, ioctx_, GetSecurityString_());
                // wait for a client to connect
                co_await pipe.Accept();
                // fork this acceptor coroutine
                as::co_spawn(ioctx_, AcceptConnection_(), as::detached);
                // run the action handler until client session is terminated
                while (true) {
                    co_await actionManager_.SyncHandleRequest(pipe, pipe.readBuf_, pipe.writeBuf_);
                }
            }
            catch (...) {
                pmlog_dbg("Action pipe disconnected");
            }
        }
        std::string GetSecurityString_() const
        {
            return elevatedSecurity_ ?
                // for when running as server
                "D:PNO_ACCESS_CONTROLS:(ML;;NW;;;LW)"s :
                // for when running as a child process
                "D:(A;OICI;GA;;;WD)"s;
        }
        // data
        std::string pipeName_;
        bool elevatedSecurity_; // for now, assume that security is elevated only when using default pipe name
        as::io_context ioctx_;
        act::AsyncActionManager<ServiceExecutionContext> actionManager_;
        as::windows::object_handle stopEvent_;
        std::jthread thread_;
    };

    ActionServer::ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName)
        :
        pImpl_{ std::make_shared<ActionServerImpl_>(pSvc, pPmon, std::move(pipeName)) }
    {}
}
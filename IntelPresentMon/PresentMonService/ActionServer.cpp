// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/pipe/Pipe.h"
#include "..\CommonUtilities\str\String.h"
#include "../Interprocess/source/act/AsyncActionManager.h"
#include <boost/asio/windows/object_handle.hpp>
#include "CliOptions.h"
#include "ActionServer.h"
#include "GlobalIdentifiers.h"
#include "ServiceExecutionContext.h"
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
        }
        void Run_()
        {
            try {
                // there are issues with calling .stop() and subsequently destroying ioctx
                // for now we will detach the thread and allow it to run independently until
                // process exit
                // 
                //// coroutine to exit thread on stop signal
                //as::co_spawn(ioctx_, [this]() -> as::awaitable<void> {
                //    co_await stopEvent_.async_wait(as::use_awaitable);
                //    pmlog_dbg("ActionServer received stop signal");
                //    ioctx_.stop();
                //    }, as::detached);
                
                // maintain 2 available pipe instances at all times
                for (int i = 0; i < 2; i++) {
                    as::co_spawn(ioctx_, AcceptConnection_(), as::detached);
                }
                // run the io context event handler until signalled to exit
                ioctx_.run();
                pmlog_info("ActionServer exiting");
            }
            catch (...) {
                pmlog_error(ReportException());
                // if the action server crashes for any reason, the service should restart at this point
                log::GetDefaultChannel()->Flush();
                std::terminate();
            }
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
            auto& sessions = actionManager_.ctx_.sessions;
            std::optional<uint32_t> sessionId;
            try {
                // create pipe instance object
                std::shared_ptr pPipe = pipe::DuplexPipe::MakeAsPtr(pipeName_, ioctx_, GetSecurityString_());
                // wait for a client to connect
                co_await pPipe->Accept();
                // insert a session context object for this connection, will be initialized properly upon OpenSession action
                sessionId = pPipe->GetId();
                sessions[*sessionId].pPipe = pPipe;
                // fork this acceptor coroutine
                as::co_spawn(ioctx_, AcceptConnection_(), as::detached);
                // run the action handler until client session is terminated
                while (true) {
                    co_await actionManager_.SyncHandleRequest(*pPipe);
                }
            }
            catch (...) {
                pmlog_dbg(util::ReportException());
                if (sessionId) {
                    uint32_t clientPid = 0;
                    if (auto i = sessions.find(*sessionId); i != sessions.end()) {
                        clientPid = i->second.clientPid;
                    }
                    actionManager_.ctx_.DisposeSession(*sessionId);
                    pmlog_info(std::format("Action pipe disconnected id:{} pid:{}", *sessionId, clientPid));
                    co_return;
                }
                pmlog_info("Sessionless pipe disconnected");
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
    };

    ActionServer::ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName)
    {
        std::thread([=, pipeName = std::move(pipeName)] {
            ActionServerImpl_ impl(pSvc, pPmon, std::move(pipeName));
            impl.Run_();
        }).detach();
    }
}
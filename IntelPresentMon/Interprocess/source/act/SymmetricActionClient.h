// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "SymmetricActionConnector.h"
#include "../../../CommonUtilities/str/String.h"
#include "../../../CommonUtilities/pipe/ManualAsyncEvent.h"
#include "AsyncActionCollection.h"
#include "ActionContext.h"
#include <thread>


namespace pmon::ipc::act
{
    using namespace std::literals;
    using namespace pmon;
    using namespace util;
    using namespace ipc;
    namespace as = boost::asio;


    template<class ExecCtx, class OpenSessionAction>
    class SymmetricActionClient
    {
        using SessionContextType = typename ExecCtx::SessionContextType;
    public:
        SymmetricActionClient(std::string pipeName)
            :
            basePipeName_{ std::move(pipeName) },
            stopEvt_{ ioctx_ }
        {
            stx_.pConn = SymmetricActionConnector<ExecCtx>::ConnectToServer(basePipeName_, ioctx_);
            as::co_spawn(ioctx_, [this]() -> as::awaitable<void> {
                bool alive = true;
                while (alive) {
                    auto res = co_await(stx_.pConn->SyncHandleRequest(ctx_, stx_) || stopEvt_.AsyncWait());
                    alive = res.index() == 0;
                }
            }, as::detached);
            runner_ = std::jthread{ &SymmetricActionClient::Run_, this };
            auto res = DispatchSync(typename OpenSessionAction::Params{
                .clientPid = GetCurrentProcessId(),
            });
            stx_.remotePid = res.serverPid;
            pmlog_info("Opened session with server").pmwatch(res.serverPid);
        }

        SymmetricActionClient(const SymmetricActionClient&) = delete;
        SymmetricActionClient& operator=(const SymmetricActionClient&) = delete;
        SymmetricActionClient(SymmetricActionClient&&) = delete;
        SymmetricActionClient& operator=(SymmetricActionClient&&) = delete;
        ~SymmetricActionClient()
        {
            stopEvt_.Signal();
        }

        template<class Params>
        auto DispatchSync(Params&& params)
        {
            return stx_.pConn->DispatchSync(std::forward<Params>(params), ioctx_, stx_);
        }

    private:
        // function
        void Run_()
        {
            ioctx_.run();
        }
        // data
        std::string basePipeName_;
        pipe::as::io_context ioctx_;
        SessionContextType stx_;
        ExecCtx ctx_;
        pipe::ManualAsyncEvent stopEvt_;
        std::jthread runner_;
    };
}
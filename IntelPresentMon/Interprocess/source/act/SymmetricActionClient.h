// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "SymmetricActionConnector.h"
#include "../../../CommonUtilities/str/String.h"
#include "../../../CommonUtilities/pipe/ManualAsyncEvent.h"
#include "AsyncActionCollection.h"
#include "ActionContext.h"
#include <thread>
#include <boost/asio/experimental/awaitable_operators.hpp>


namespace pmon::ipc::act
{
    using namespace std::literals;
    using namespace pmon;
    using namespace util;
    using namespace ipc;
    namespace as = boost::asio;
    using namespace as::experimental::awaitable_operators;

    template<class ExecCtx>
    class SymmetricActionClient
    {
        using SessionContextType = typename ExecCtx::SessionContextType;
    public:
        SymmetricActionClient(std::string pipeName, ExecCtx context = {})
            :
            basePipeName_{ std::move(pipeName) },
            ctx_{ std::move(context) },
            stx_{ .stopEvt = { ioctx_ } }
        {
            stx_.pConn = SymmetricActionConnector<ExecCtx>::ConnectToServer(basePipeName_, ioctx_);
            as::co_spawn(ioctx_, SessionStrand_(), as::detached);
            runner_ = std::jthread{ &SymmetricActionClient::Run_, this };
        }

        SymmetricActionClient(const SymmetricActionClient&) = delete;
        SymmetricActionClient& operator=(const SymmetricActionClient&) = delete;
        SymmetricActionClient(SymmetricActionClient&&) = delete;
        SymmetricActionClient& operator=(SymmetricActionClient&&) = delete;
        ~SymmetricActionClient()
        {
            stx_.stopEvt.Signal();
        }

        template<class Params>
        auto DispatchSync(Params&& params)
        {
            return stx_.pConn->DispatchSync(std::forward<Params>(params), ioctx_, stx_);
        }
        template<class Params>
        auto DispatchAsync(Params&& params)
        {
            return stx_.pConn->DispatchAsync(std::forward<Params>(params), ioctx_, stx_);
        }
        template<class Params>
        void DispatchDetachedWithContinuation(Params&& params, std::function<void(ResponseFromParams<Params>&&)> cont)
        {
            return stx_.pConn->DispatchDetachedWithContinuation(std::forward<Params>(params), ioctx_, stx_, std::move(cont));
        }

    protected:
        void EstablishSession_(uint32_t serverPid)
        {
            stx_.remotePid = serverPid;
        }

    private:
        // function
        void Run_()
        {
            // TODO: investigate heap issue with this line
            //log::IdentificationTable::AddThisThread("act-cli-worker");
            try {
                ioctx_.run();
                pmlog_info("ActionClient exiting");
            }
            catch (...) {
                pmlog_error(ReportException());
                // if the action server crashes for any reason, the service should restart at this point
                // TODO: don't make this mandatory for all uses of the server (client in this case)
                log::GetDefaultChannel()->Flush();
                std::terminate();
            }
        }
        as::awaitable<void> SessionStrand_()
        {
            // TODO: work on propagation of server disconnection event errors
            try {
                bool alive = true;
                while (alive) {
                    auto res = co_await(stx_.pConn->SyncHandleRequest(ctx_, stx_) || stx_.stopEvt.AsyncWait());
                    alive = res.index() == 0;
                }
            }
            catch (const pipe::BenignPipeError&) {
                pmlog_dbg(util::ReportException());
            }
            catch (...) {
                pmlog_error(util::ReportException());
            }
            pmlog_info("Exiting action server session strand");
        }
        // data
        std::string basePipeName_;
        pipe::as::io_context ioctx_;
        SessionContextType stx_;
        ExecCtx ctx_;
        std::jthread runner_;
    };
}
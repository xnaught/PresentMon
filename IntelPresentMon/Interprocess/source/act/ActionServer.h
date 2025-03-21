// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "../../../CommonUtilities/str/String.h"
#include "Transfer.h"
#include "AsyncActionCollection.h"
#include <thread>
#include <sddl.h>


namespace pmon::ipc::act
{
    using namespace std::literals;
    using namespace pmon;
    using namespace util;
    using namespace ipc;
    namespace as = boost::asio;

    template<class ExecCtx>
    class ActionServerImpl_
    {
    public:
        ActionServerImpl_(ExecCtx context, std::string pipeName,
            uint32_t reservedPipeInstanceCount, std::string securityString)
            :
            reservedPipeInstanceCount_{ reservedPipeInstanceCount },
            pipeName_{ std::move(pipeName) },
            security_{ std::move(securityString) },
            // stopEvent_{ ioctx_, win::Handle::CreateCloned(pSvc->GetServiceStopHandle()).Release() },
            ctx_{ std::move(context) }
        {
            assert(reservedPipeInstanceCount_ > 0);
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

                // maintain N available pipe instances at all times
                for (uint32_t i = 0; i < reservedPipeInstanceCount_; i++) {
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
        static std::thread LaunchThread(ExecCtx context, std::string pipeName,
            uint32_t maxPipeInstanceCount, std::string securityString)
        {
            return std::thread([](auto&& context, std::string pipeName,
                uint32_t maxPipeInstanceCount, std::string securityString) {
                ActionServerImpl_ impl(std::move(context), std::move(pipeName),
                    maxPipeInstanceCount, std::move(securityString));
                impl.Run_();
            }, std::move(context), std::move(pipeName), maxPipeInstanceCount, std::move(securityString));
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
            auto& sessions = ctx_.sessions;
            std::optional<uint32_t> sessionId;
            try {
                // create pipe instance object
                std::shared_ptr pPipe = pipe::DuplexPipe::MakeAsPtr(pipeName_, ioctx_, security_);
                // wait for a client to connect
                co_await pPipe->Accept();
                // insert a session context object for this connection, will be initialized properly upon OpenSession action
                sessionId = pPipe->GetId();
                sessions[*sessionId].pPipe = pPipe;
                pmlog_info(std::format("Action pipe connected id:{}", *sessionId));
                // fork this acceptor coroutine
                as::co_spawn(ioctx_, AcceptConnection_(), as::detached);
                // run the action handler until client session is terminated
                while (true) {
                    co_await SyncHandleRequest_(*pPipe);
                }
            }
            catch (...) {
                pmlog_dbg(util::ReportException());
                if (sessionId) {
                    uint32_t clientPid = 0;
                    if (auto i = sessions.find(*sessionId); i != sessions.end()) {
                        clientPid = i->second.clientPid;
                    }
                    ctx_.DisposeSession(*sessionId);
                    pmlog_info(std::format("Action pipe disconnected, session closed id:{} pid:{}", *sessionId, clientPid));
                    co_return;
                }
                pmlog_info("Sessionless pipe disconnected");
            }
        }
        as::awaitable<void> SyncHandleRequest_(pipe::DuplexPipe& pipe)
        {
            // read packet from the pipe into buffer, partially deserialize (header only)
            const auto header = co_await pipe.ReadPacketConsumeHeader<PacketHeader>();
            // lookup the command by identifier and execute it with remaining buffer contents
            // response is then transmitted over the pipe to remote
            co_await AsyncActionCollection<ExecCtx>::Get()
                .Find(header.identifier).Execute(ctx_, header, pipe);
        }
        // data
        uint32_t reservedPipeInstanceCount_;
        uint32_t maxPipeInstanceCount_;
        std::string pipeName_;
        std::string security_;
        as::io_context ioctx_;
        // as::windows::object_handle stopEvent_;
        ExecCtx ctx_;
    };
}
// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "../../../CommonUtilities/str/String.h"
#include "Transfer.h"
#include "AsyncActionCollection.h"
#include <boost/asio/experimental/awaitable_operators.hpp>


namespace pmon::ipc::act
{
	template<class ExecCtx>
	class SymmetricActionConnector
	{
        using SessionContextType = typename ExecCtx::SessionContextType;
	public:
        as::awaitable<void> SyncHandleRequest(ExecCtx& ctx, SessionContextType& stx)
        {
            PacketHeader header;
            try {
                // read packet from the pipe into buffer, partially deserialize (header only)
                header = co_await pInPipe_->ReadPacketConsumeHeader<PacketHeader>();
                // -- do per-action processing based on received header --
                // any action other than OpenSession without having clientPid is an anomaly
                // TODO: make this processing a customization point in ExecutionContext and move it out of here
                if (header.identifier != "OpenSession") {
                    assert(bool(stx.remotePid));
                    if (!stx.remotePid) {
                        pmlog_warn("Received action without a valid session opened");
                    }
                }
                // bookkeeping
                stx.lastTokenSeen = header.commandToken;
                stx.lastReceived = std::chrono::high_resolution_clock::now();
                stx.receiveCount++;
                // lookup the command by identifier and execute it with remaining buffer contents
                // response is then transmitted over the pipe to remote
                // TODO: make this return result code (increment error count based on this)
                co_await AsyncActionCollection<ExecCtx>::Get().Find(header.identifier).Execute(ctx, stx, header, *pInPipe_);
                co_return;
            }
            catch (...) {
                pmlog_error(util::ReportException());
                // if the output buffer is dirty, we're not sure what state we're in so just clear it
                if (pInPipe_->GetWriteBufferPending()) {
                    pInPipe_->ClearWriteBuffer();
                }
            }
            auto resHeader = MakeResponseHeader(header, TransportStatus::TransportFailure, PM_STATUS_SUCCESS);
            co_await pInPipe_->WritePacket(std::move(resHeader), EmptyPayload{}, ctx.responseWriteTimeoutMs);
        }
        template<class Params>
        auto DispatchSync(Params&& params, as::io_context& ioctx, SessionContextType& stx)
        {
            using Action = typename ipc::act::ActionParamsTraits<std::decay_t<Params>>::Action;
            using Response = Action::Response;
            // wrap the SyncRequest in a coro so we can assure non-concurrent increment of the token
            const auto coro = [this, &stx](auto&& params) -> as::awaitable<Response> {
                co_return co_await SyncRequest<Action>(std::forward<Params>(params), stx.nextCommandToken++, *pOutPipe_);
            };
            return as::co_spawn(ioctx, coro(std::forward<Params>(params)), as::use_future).get();
        }
        uint32_t GetId() const
        {
            return pInPipe_->GetId();
        }
        static as::awaitable<std::unique_ptr<SymmetricActionConnector>> AcceptClientConnection(
            const std::string& basePipeName, as::io_context& ioctx, const std::string& security)
        {
            using namespace as::experimental::awaitable_operators;
            auto pConn = std::make_unique<SymmetricActionConnector>(basePipeName, ioctx, security);
            co_await(pConn->pInPipe_->Accept() && pConn->pOutPipe_->Accept());
            co_return pConn;
        }
        static std::unique_ptr<SymmetricActionConnector> ConnectToServer(
            const std::string& basePipeName, as::io_context& ioctx)
        {
            return std::make_unique<SymmetricActionConnector>(basePipeName, ioctx);
        }
        SymmetricActionConnector(const std::string& basePipeName, as::io_context& ioctx, const std::string& security)
            :
            pOutPipe_{ pipe::DuplexPipe::MakeAsPtr(basePipeName + "-out", ioctx, security) },
            pInPipe_{ pipe::DuplexPipe::MakeAsPtr(basePipeName + "-in", ioctx, security) }
        {}
        SymmetricActionConnector(const std::string& basePipeName, as::io_context& ioctx)
            :
            pOutPipe_{ pipe::DuplexPipe::ConnectAsPtr(basePipeName + "-in", ioctx) },
            pInPipe_{ pipe::DuplexPipe::ConnectAsPtr(basePipeName + "-out", ioctx) }
        {}
	private:
		// data
		std::unique_ptr<pipe::DuplexPipe> pOutPipe_;
		std::unique_ptr<pipe::DuplexPipe> pInPipe_;
	};
}
#pragma once
#include "../../../CommonUtilities/win/WinAPI.h"
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "../../../CommonUtilities/log/Log.h"
#include <cereal/archives/binary.hpp>
#include "Packet.h"
#include "ActionExecutionError.h"

namespace pmon::ipc::act
{
	using namespace util;

	template<class ExecutionContext>
	class AsyncAction
	{
	public:
		virtual const char* GetIdentifier() const = 0;
		virtual pipe::as::awaitable<void> Execute(ExecutionContext& ctx,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const = 0;
	};

	template<class T, class ExecutionContext>
	class AsyncActionBase_ : public AsyncAction<ExecutionContext>
	{
	protected:
		using SessionContext = typename ExecutionContext::SessionContextType;
	public:
		pipe::as::awaitable<void> Execute(ExecutionContext& ctx,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const final
		{
			PacketHeader resHeader;
			typename T::Response output;
			auto& stx = ctx.sessions.at(pipe.GetId());
			try {
				// session should be initialized except in the special case of the OpenSession action
				// TODO: find a way of pushing this check down to the app-specific layer rather than hardcode identifier here
				if (header.identifier != "OpenSession") {
					assert(bool(stx.clientPid));
					if (!stx.clientPid) {
						pmlog_warn("Received action without a valid session opened");
					}
				}
				stx.lastTokenSeen = header.commandToken;
				stx.lastReceived = std::chrono::high_resolution_clock::now();
				stx.receiveCount++;
				output = T::Execute_(ctx, stx, pipe.ConsumePacketPayload<typename T::Params>());
				resHeader = MakeResponseHeader_(header, TransportStatus::Success, PM_STATUS_SUCCESS);
			}
			catch (const ActionExecutionError& e) {
				pmlog_error(std::format("Error in action [{}] execution", GetIdentifier())).code(e.GetCode());
				resHeader = MakeResponseHeader_(header, TransportStatus::ExecutionFailure, e.GetCode());
			}
			catch (...) {
				pmlog_error(util::ReportException());
				// if the output buffer is dirty, we're not sure what state we're in so just clear it
				if (pipe.GetWriteBufferPending()) {
					pipe.ClearWriteBuffer();
				}
				resHeader = MakeResponseHeader_(header, TransportStatus::TransportFailure, PM_STATUS_SUCCESS);
			}
			if (resHeader.transportStatus == TransportStatus::Success) {
				// if no errors occured transmit standard packet with header and action response payload
				co_await pipe.WritePacket(resHeader, output, ctx.responseWriteTimeoutMs);
			}
			else {
				stx.errorCount++;
				// if there was an error, transmit header (configured with error status) and empty payload
				co_await pipe.WritePacket(resHeader, EmptyPayload{}, ctx.responseWriteTimeoutMs);
			}
		}
		const char* GetIdentifier() const final
		{
			return T::Identifier;
		}
		// default version for all actions
		static constexpr uint16_t Version = 1;
	private:
		static PacketHeader MakeResponseHeader_(const PacketHeader& reqHeader, TransportStatus txs, int exs)
		{
			auto resHeader = reqHeader;
			resHeader.transportStatus = txs;
			resHeader.executionStatus = exs;
			return resHeader;
		}
	};

	template<class P>
	struct ActionParamsTraits;
}
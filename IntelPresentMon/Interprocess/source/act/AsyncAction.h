#pragma once
#include "../../../CommonUtilities/win/WinAPI.h"
#include "../../../CommonUtilities/log/Log.h"
#include <boost/asio.hpp>
#include <cereal/archives/binary.hpp>
#include "Encoding.h"
#include "Packet.h"
#include "ActionResponseError.h"

namespace pmon::ipc::act
{
	template<class ExecutionContext>
	class AsyncAction
	{
	public:
		virtual const char* GetIdentifier() const = 0;
		virtual void Execute(const ExecutionContext& ctx, uint32_t commandToken,
			boost::asio::streambuf& inputBuffer, boost::asio::streambuf& outputBuffer) const = 0;
	};

	template<class T, class ExecutionContext>
	class AsyncActionBase_ : public AsyncAction<ExecutionContext>
	{
	public:
		void Execute(const ExecutionContext& ctx, uint32_t commandToken,
			boost::asio::streambuf& inputBuffer, boost::asio::streambuf& outputBuffer) const final
		{
			try {
				typename T::Params input;
				std::istream is{ &inputBuffer };
				cereal::BinaryInputArchive{ is }(input);
				const auto output = T::Execute_(ctx, std::move(input));
				EncodeTransmissionPacket(ResponseHeader{ .commandToken = commandToken, .status = 0 }, output, outputBuffer);
			}
			catch (const ActionResponseError& e) {
				pmlog_error(std::format("Error in action [{}] execution", GetIdentifier())).code(e.GetCode());
				EncodeTransmissionPacket(ResponseHeader{ .commandToken = commandToken, .status = e.GetCode() }, EmptyPayload{}, outputBuffer);
			}
			catch (...) {
				pmlog_error(util::ReportException());
				EncodeTransmissionPacket(ResponseHeader{ .commandToken = commandToken, .status = PM_STATUS_FAILURE }, EmptyPayload{}, outputBuffer);
			}
		}
		const char* GetIdentifier() const final
		{
			return T::Identifier;
		}
	};

	template<class P>
	struct ActionParamsTraits;
}
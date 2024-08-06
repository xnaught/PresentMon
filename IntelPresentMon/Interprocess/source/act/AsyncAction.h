#pragma once
#include "../../../CommonUtilities/win/WinAPI.h"
#include <boost/asio.hpp>
#include <cereal/archives/binary.hpp>
#include "Encoding.h"
#include "Packet.h"

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
			typename T::Params input;
			std::istream is{ &inputBuffer };
			cereal::BinaryInputArchive{ is }(input);
			try {
				const auto output = T::Execute_(ctx, std::move(input));
				EncodeTransmissionPacket(ResponseHeader{ .commandToken = commandToken, .status = 0 }, output, outputBuffer);
			}
			catch (...) {
				EncodeTransmissionPacket(ResponseHeader{ .commandToken = commandToken, .status = -1 }, EmptyPayload{}, outputBuffer);
			}
		}
		const char* GetIdentifier() const final
		{
			return T::Identifier;
		}
	};
}
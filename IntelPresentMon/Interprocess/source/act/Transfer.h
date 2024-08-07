#pragma once
#include "../../../CommonUtilities/win/WinAPI.h"
#include <boost/asio.hpp>
#include <cereal/archives/binary.hpp>
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "Packet.h"
#include "Encoding.h"
#include "ActionResponseError.h"

namespace pmon::ipc::act
{
	namespace as = boost::asio;
	using namespace util::pipe;

	template<class H>
	as::awaitable<H> ReadPacketWithHeader(DuplexPipe& pipe, as::streambuf& inBuf)
	{
		std::istream is{ &inBuf };
		cereal::BinaryInputArchive iar{ is };
		// read in request
		// first read the number of bytes in the request payload (always 4-byte read)
		uint32_t payloadSize;
		co_await as::async_read(pipe.stream_, inBuf, as::transfer_exactly(sizeof(payloadSize)), as::use_awaitable);
		is.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
		// read the payload
		co_await as::async_read(pipe.stream_, inBuf, as::transfer_exactly(payloadSize), as::use_awaitable);
		// deserialize header portion of request payload
		H header;
		iar(header);
		co_return header;
	}

	template<class C>
	auto SyncRequest(const typename C::Params& params, uint32_t commandToken, DuplexPipe& pipe,
		as::streambuf& outBuf, as::streambuf& inBuf) -> as::awaitable<typename C::Response>
	{
		EncodeTransmissionPacket(RequestHeader{ C::Identifier, commandToken }, params, outBuf);
		co_await as::async_write(pipe.stream_, outBuf, as::use_awaitable);
		auto header = co_await ReadPacketWithHeader<ResponseHeader>(pipe, inBuf);
		std::istream is{ &inBuf };
		cereal::BinaryInputArchive iar{ is };
		if (header.status) {
			// consume the empty payload to leave the pipe stream in a clean state
			EmptyPayload ep;
			iar(ep);
			throw util::Except<ActionResponseError>((PM_STATUS)header.status);
		}
		typename C::Response response;
		iar(response);
		co_return response;
	}
}
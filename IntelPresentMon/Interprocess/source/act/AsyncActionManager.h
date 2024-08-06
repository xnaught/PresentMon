#pragma once
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "AsyncActionCollection.h"
#include "Transfer.h"

namespace pmon::ipc::act
{
	namespace as = boost::asio;
	using namespace util::pipe;

	template<class ExecutionContext>
	class AsyncActionManager
	{
	public:
		as::awaitable<void> SyncHandleRequest(DuplexPipe& pipe, as::streambuf& inBuf, as::streambuf& outBuf)
		{
			// read packet from the pipe into buffer, partially deserialize (header only)
			const auto header = co_await ReadPacketWithHeader<RequestHeader>(pipe, inBuf);
			// lookup the command by identifier and execute it with remaining buffer contents
			// response packet is written to the outBuf ready for transmission
			AsyncActionCollection<ExecutionContext>::Get().Find(header.identifier).Execute(ctx_, header.commandToken, inBuf, outBuf);
			// transmit the response
			co_await as::async_write(pipe.stream_, outBuf, as::use_awaitable);
		}
		// TODO: encapsulate this, and template it to not tie to any one receiver context (support symmetric usage)
		ExecutionContext ctx_;
	};
}

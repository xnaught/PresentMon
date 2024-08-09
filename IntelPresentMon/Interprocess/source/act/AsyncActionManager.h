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
		as::awaitable<void> SyncHandleRequest(DuplexPipe& pipe)
		{
			// read packet from the pipe into buffer, partially deserialize (header only)
			const auto header = co_await pipe.ReadPacketConsumeHeader<PacketHeader>();
			// lookup the command by identifier and execute it with remaining buffer contents
			// response is then transmitted over the pipe to remote
			co_await AsyncActionCollection<ExecutionContext>::Get()
				.Find(header.identifier).Execute(ctx_, header, pipe);
		}
		// TODO: encapsulate this, and template it to not tie to any one receiver context (support symmetric usage)
		ExecutionContext ctx_;
	};
}

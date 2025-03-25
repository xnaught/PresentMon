#pragma once
#include "../win/WinAPI.h"
#include "../win/Event.h"
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>


namespace pmon::util::pipe
{
	namespace as = boost::asio;

	class ManualAsyncEvent
	{
	public:
		ManualAsyncEvent(as::io_context& ctx)
			:
			asioEventView_{ ctx, winEventView_.Clone().Release() }
		{}
		as::awaitable<void> AyncWait()
		{
			co_await asioEventView_.async_wait(as::use_awaitable);
			winEventView_.Reset();
		}
		void Signal()
		{
			winEventView_.Set();
		}
	private:
		win::Event winEventView_;
		as::windows::object_handle asioEventView_;
	};
}
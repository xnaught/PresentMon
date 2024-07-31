#pragma once
#include "../win/WinAPI.h"
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include "CoroMutex.h"
#include "../win/Handle.h"
#include "../win/Event.h"
#include "../Exception.h"
#include "../log/Log.h"

namespace pmon::util::pipe
{
	namespace as = boost::asio;
	using namespace std::literals;

	PM_DEFINE_EX(PipeError);

	class DuplexPipe
	{
	public:
		DuplexPipe(const DuplexPipe&) = delete;
		DuplexPipe & operator=(const DuplexPipe&) = delete;
		DuplexPipe(DuplexPipe&&) = default;
		DuplexPipe & operator=(DuplexPipe&&) = default;
		~DuplexPipe() = default;
		as::awaitable<void> Accept()
		{
			as::windows::object_handle connEvt{ co_await as::this_coro::executor, win::Event{}.Release() };
			OVERLAPPED over{ .hEvent = connEvt.native_handle() };
			const auto result = ConnectNamedPipe(stream_.native_handle(), &over);
			if (result) {
				// this is not expected in async operation
				// but if it happens let's interpret as "already connected"
				pmlog_warn("Non-zero return from async ConnectNamedPipe");
				co_return;
			}
			if (const auto error = GetLastError(); error == ERROR_IO_PENDING) {
				// not yet connected, await on connection event
				co_await connEvt.async_wait(as::use_awaitable);
			}
			else if (error == ERROR_PIPE_CONNECTED) {
				// connected even before we could await event
				co_return;
			}
			else {
				// some error has occurred during connection
				pmlog_error("Failure accepting pipe connection").hr().raise<PipeError>();
			}
		}
		static DuplexPipe ConnectPipe(const std::string& name, as::io_context& ioctx)
		{
			win::Handle handle(CreateFileA(
				name.c_str(),					// Pipe name 
				GENERIC_READ,	// Desired access: Read/Write 
				0,								// No sharing 
				NULL,							// Default security attributes
				OPEN_EXISTING,					// Opens existing pipe 
				FILE_FLAG_OVERLAPPED,			// Use overlapped (asynchronous) mode
				NULL));							// No template file 
			if (!handle) {
				pmlog_error("Client failed to connect to named pipe instance").hr().raise<PipeError>();
			}
			return DuplexPipe{ ioctx, handle.Release() };
		}
		static DuplexPipe MakePipe(const std::string& name, as::io_context& ioctx)
		{
			win::Handle handle(CreateNamedPipeA(
				name.c_str(),
				PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,	// open mode
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,	// pipe mode
				PIPE_UNLIMITED_INSTANCES,					// max instances
				4096,		// out buffer
				4096,		// in buffer
				0,			// timeout
				NULL));		// security
			if (!handle) {
				pmlog_error("Server failed to create named pipe instance").raise<PipeError>();
			}
			return DuplexPipe{ ioctx, handle.Release() };
		}
		template<class S>
		as::awaitable<void> WriteSerialized(const S& obj)
		{
			// prevent other coros from writing to pipe while we're working here
			auto lk = co_await CoroLock(writeMtx_);

			// serialize object to the transfer buffer
			std::ostream bufStream{ &writeBuf_ };
			cereal::BinaryOutputArchive archive(bufStream);
			archive(obj);

			// write the transfer header containing number of bytes in payload
			std::array header{ uint32_t(writeBuf_.size()) };
			co_await as::async_write(stream_, as::buffer(header), as::use_awaitable);
			// write the serialized object payload
			co_await as::async_write(stream_, writeBuf_, as::use_awaitable);
		}
		template<class S>
		as::awaitable<S> ReadSerialized()
		{
			// prevent other coros from reading from pipe while we're working here
			auto lk = co_await CoroLock(readMtx_);

			// read the transfer header containing number of bytes in payload
			std::array header{ uint32_t(-1) };
			co_await as::async_read(stream_, as::buffer(header), as::transfer_exactly(SizeInBytes(header)), as::use_awaitable);
			// read the serialized object payload
			co_await as::async_read(stream_, readBuf_, as::transfer_exactly(header[0]), as::use_awaitable);

			// deserialize object from the transfer buffer
			std::istream bufStream{ &readBuf_ };
			cereal::BinaryInputArchive archive(bufStream);
			S obj;
			archive(obj);
			co_return obj;
		}
	private:
		// functions
		DuplexPipe(as::io_context& ioctx, HANDLE pipeHandle)
			:
			stream_{ ioctx, pipeHandle },
			readMtx_{ ioctx },
			writeMtx_{ ioctx }
		{}
		// data
		as::windows::stream_handle stream_;
		CoroMutex readMtx_;
		as::streambuf readBuf_;
		CoroMutex writeMtx_;
		as::streambuf writeBuf_;
	};
}
#include "Pipe.h"
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <sddl.h>
#include <string_view>

namespace pmon::util::pipe
{
	using namespace as::experimental::awaitable_operators;

	std::atomic<uint32_t> DuplexPipe::nextUid_ = 0;

	as::awaitable<void> DuplexPipe::Accept()
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
	DuplexPipe DuplexPipe::Connect(const std::string& name, as::io_context& ioctx)
	{
		return DuplexPipe{ ioctx, Connect_(name) };
	}
	DuplexPipe DuplexPipe::Make(const std::string& name, as::io_context& ioctx, const std::string& security)
	{
		return DuplexPipe{ ioctx, Make_(name) };
	}
	std::unique_ptr<DuplexPipe> DuplexPipe::ConnectAsPtr(const std::string& name, as::io_context& ioctx)
	{
		return std::unique_ptr<DuplexPipe>(new DuplexPipe{ ioctx, Connect_(name) });
	}
	std::unique_ptr<DuplexPipe> DuplexPipe::MakeAsPtr(const std::string& name, as::io_context& ioctx, const std::string& security)
	{
		return std::unique_ptr<DuplexPipe>(new DuplexPipe{ ioctx, Make_(name, security) });
	}
	size_t DuplexPipe::GetWriteBufferPending() const
	{
		return writeBuf_.size();
	}
	void DuplexPipe::ClearWriteBuffer()
	{
		return writeBuf_.consume(GetWriteBufferPending());
	}
	bool DuplexPipe::WaitForAvailability(const std::string& name, uint32_t timeoutMs, uint32_t pollPeriodMs)
	{
		const auto start = std::chrono::high_resolution_clock::now();
		while (std::chrono::high_resolution_clock::now() - start < 1ms * timeoutMs) {
			if (WaitNamedPipeA(name.c_str(), 0)) {
				return true;
			}
			else {
				std::this_thread::sleep_for(1ms * pollPeriodMs);
			}
		}
		return false;
	}
	uint32_t DuplexPipe::GetId() const
	{
		return uid_;
	}
	DuplexPipe::DuplexPipe(as::io_context& ioctx, HANDLE pipeHandle)
		:
		stream_{ ioctx, pipeHandle },
		readMtx_{ ioctx },
		readStream_{ &readBuf_ },
		readArchive_{ readStream_ },
		writeMtx_{ ioctx },
		writeStream_{ &writeBuf_ },
		writeArchive_{ writeStream_ }
	{}
	HANDLE DuplexPipe::Connect_(const std::string& name)
	{
		win::Handle handle(CreateFileA(
			name.c_str(),					// Pipe name 
			GENERIC_READ | GENERIC_WRITE,	// Desired access: Read/Write 
			0,								// No sharing 
			NULL,							// Default security attributes
			OPEN_EXISTING,					// Opens existing pipe 
			FILE_FLAG_OVERLAPPED,			// Use overlapped (asynchronous) mode
			NULL));							// No template file 
		if (!handle) {
			pmlog_error("Client failed to connect to named pipe instance").hr().raise<PipeError>();
		}
		return handle.Release();
	}
	HANDLE DuplexPipe::Make_(const std::string& name, const std::string& security)
	{
		// structure required for creating named pipe, create with placeholder pointer for descriptor
		SECURITY_ATTRIBUTES securityAttributes{
			.nLength = sizeof(securityAttributes),
			.lpSecurityDescriptor = nullptr,
			.bInheritHandle = FALSE,
		};
		// if we have a security string, create the descriptor and have it owned by above structure
		if (!security.empty()) {
			if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
				security.c_str(), SDDL_REVISION_1,
				&securityAttributes.lpSecurityDescriptor, NULL)) {
				pmlog_error(std::format(
					"Failed creating security descriptor for pipe [{}], descriptor string was '{}'",
					name, security)).hr().raise<PipeError>();
			}
		}
		// if we have a security string, call create pipe with above structure, else call with nullptr
		SECURITY_ATTRIBUTES* pSecurityAttributes = security.empty() ? nullptr : &securityAttributes;
		// create the named pipe and retain the handle in a wrapper object
		win::Handle handle(CreateNamedPipeA(
			name.c_str(),
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,	// open mode
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,		// pipe mode
			PIPE_UNLIMITED_INSTANCES,					// max instances
			4096,					// out buffer
			4096,					// in buffer
			0,						// timeout
			pSecurityAttributes));	// security
		// regardless of result, if we are using security, free the descriptor owned by above structure
		if (!security.empty()) {
			if (LocalFree(securityAttributes.lpSecurityDescriptor) != NULL) {
				pmlog_warn("Failed freeing memory for security descriptor");
			}
		}
		if (!handle) {
			pmlog_error("Server failed to create named pipe instance").hr().raise<PipeError>();
		}
		// release the owned handle to be captured by some other owner
		return handle.Release();
	}
	as::awaitable<void> DuplexPipe::Read_(size_t byteCount, std::optional<uint32_t> timeoutMs)
	{
		if (timeoutMs) {
			const auto result = co_await(as::async_read(stream_, readBuf_, as::transfer_exactly(byteCount),
				as::as_tuple(as::use_awaitable)) || Timeout_(*timeoutMs));
			// 2nd index active means timed out
			if (result.index() == 1) {
				throw Except<PipeError>("Timeout during read");
			}
			// otherwise 1st index active => extract error code and transform
			auto&& [ec, n] = std::get<0>(result);
			TransformError_(ec);
		}
		else {
			const auto [ec, n] = co_await as::async_read(stream_, readBuf_, as::transfer_exactly(byteCount),
				as::as_tuple(as::use_awaitable));
			TransformError_(ec);
		}
	}
	as::awaitable<void> DuplexPipe::Write_(std::optional<uint32_t> timeoutMs)
	{
		if (timeoutMs) {
			const auto result = co_await(as::async_write(stream_, writeBuf_, as::as_tuple(as::use_awaitable))
				|| Timeout_(*timeoutMs));
			// 2nd index active means timed out
			if (result.index() == 1) {
				throw Except<PipeError>("Timeout during write");
			}
			// otherwise 1st index active => extract error code and transform
			auto&& [ec, n] = std::get<0>(result);
			TransformError_(ec);
		}
		else {
			const auto [ec, n] = co_await as::async_write(stream_, writeBuf_, as::as_tuple(as::use_awaitable));
			TransformError_(ec);
		}
	}
	as::awaitable<void> DuplexPipe::Timeout_(uint32_t ms)
	{
		as::deadline_timer timer{ co_await as::this_coro::executor };
		timer.expires_from_now(boost::posix_time::millisec{ ms });
		co_await timer.async_wait(as::use_awaitable);
	}
	void DuplexPipe::TransformError_(const boost::system::error_code& ec)
	{
	}
}
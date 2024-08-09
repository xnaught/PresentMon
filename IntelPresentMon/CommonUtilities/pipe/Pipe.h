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
#include <ranges>

namespace pmon::util::pipe
{
	namespace as = boost::asio;
	using namespace std::literals;

	PM_DEFINE_EX(PipeError);
	PM_DEFINE_EX_FROM(PipeError, PipeBroken);

	namespace {
		template<class T>
		void Check_(T&& t)
		{
			if (auto ec = std::get<0>(t)) {
				if (ec == as::error::broken_pipe) {
					throw Except<PipeBroken>();
				}
				throw boost::system::system_error{ ec };
			}
		}
	}

	class DuplexPipe
	{
	public:
		DuplexPipe(const DuplexPipe&) = delete;
		DuplexPipe& operator=(const DuplexPipe&) = delete;
		DuplexPipe(DuplexPipe&& other) = delete;
		DuplexPipe& operator=(DuplexPipe&&) = delete;
		~DuplexPipe() = default;
		as::awaitable<void> Accept();
		static DuplexPipe Connect(const std::string& name, as::io_context& ioctx);
		static DuplexPipe Make(const std::string& name, as::io_context& ioctx, const std::string& security = {});
		static std::unique_ptr<DuplexPipe> ConnectAsPtr(const std::string& name, as::io_context& ioctx);
		static std::unique_ptr<DuplexPipe> MakeAsPtr(const std::string& name, as::io_context& ioctx, const std::string& security = {});
		template<class H, class P>
		as::awaitable<void> WritePacket(const H& header, const P& payload)
		{
			assert(writeBuf_.size() == 0);
			// first we directly write bytes for the size of the body as a placeholder until we know how many are serialized
			const uint32_t placeholderSize = 'TEMP';
			writeStream_.write(reinterpret_cast<const char*>(&placeholderSize), sizeof(placeholderSize));
			// record how many bytes used for the serialization of the size
			const auto sizeSize = writeBuf_.size();
			// serialize the packet body
			writeArchive_(header, payload);
			// calculate size of body
			const auto payloadSize = uint32_t(writeBuf_.size() - sizeSize);
			// replace the placeholder with the actual body size
			auto bufSeq = writeBuf_.data();
			const auto iSize = const_cast<char*>(as::buffer_cast<const char*>(bufSeq));
			auto replacement = std::string_view{ reinterpret_cast<const char*>(&payloadSize), sizeof(payloadSize) };
			std::ranges::copy(replacement, iSize);
			// transmit the packet
			co_await as::async_write(stream_, writeBuf_, as::use_awaitable);
		}
		template<class H>
		as::awaitable<H> ReadPacketConsumeHeader()
		{
			assert(readBuf_.size() == 0);
			// read in request
			// first read the number of bytes in the request payload (always 4-byte read)
			uint32_t payloadSize;
			co_await as::async_read(stream_, readBuf_, as::transfer_exactly(sizeof(payloadSize)), as::use_awaitable);
			readStream_.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
			// read the payload
			co_await as::async_read(stream_, readBuf_, as::transfer_exactly(payloadSize), as::use_awaitable);
			// deserialize header portion of request payload
			H header;
			readArchive_(header);
			co_return header;
		}
		template<class P>
		P ConsumePacketPayload()
		{
			P payload;
			readArchive_(payload);
			if (const auto sz = readBuf_.size()) {
				assert("unexpected data when reading packet payload from buffer!!" && false);
				pmlog_warn(std::format("Buffer contained unexpected data of size", sz));
				readBuf_.consume(sz);
			}
			return payload;
		}
		size_t GetWriteBufferPending() const;
		void ClearWriteBuffer();
		static bool WaitForAvailability(const std::string& name, uint32_t timeoutMs, uint32_t pollPeriodMs = 10);
		uint32_t GetId() const;
	private:
		// functions
		DuplexPipe(as::io_context& ioctx, HANDLE pipeHandle);
		static HANDLE Connect_(const std::string& name);
		static HANDLE Make_(const std::string& name, const std::string& security = {});
		// data
		static std::atomic<uint32_t> nextUid_;
		uint32_t uid = nextUid_++;
		as::windows::stream_handle stream_;
		CoroMutex readMtx_;
		as::streambuf readBuf_;
		std::istream readStream_;
		cereal::BinaryInputArchive readArchive_;
		CoroMutex writeMtx_;
		as::streambuf writeBuf_;
		std::ostream writeStream_;
		cereal::BinaryOutputArchive writeArchive_;
	};
}
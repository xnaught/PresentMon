#include "NamedPipeMarshallSender.h"
#include "../win/WinAPI.h"
#include <stdexcept>
#include <sstream>
#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>

namespace pmon::util::log
{
    NamedPipeMarshallSender::NamedPipeMarshallSender(const std::wstring& pipeName) : pipeName_(L"\\\\.\\pipe\\" + pipeName) {
        // Create the named pipe with overlapped option enabled and outbound access
        hPipe_ = (win::Handle)CreateNamedPipeW(
            pipeName_.c_str(),
            PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, // Use overlapped (asynchronous) mode for sending
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, // Single instance
            4096, // Outbound buffer size
            4096, // Inbound buffer size (not used in this case)
            0, // Default timeout
            nullptr); // Default security
        if (hPipe_ == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to create named pipe for sending");
        }
    }

    NamedPipeMarshallSender::~NamedPipeMarshallSender() = default;

    void NamedPipeMarshallSender::Push(const Entry& entry)
    {
        // serialize the entry
        std::ostringstream stream;
        cereal::BinaryOutputArchive archive(stream);
        archive(entry);
        const auto bufferView = stream.rdbuf()->view();
        // transmit the header (payload size)
        const auto payloadSize = (DWORD)bufferView.size();
        DWORD bytesWritten{};
        if (BOOL result = WriteFile(hPipe_, &payloadSize, (DWORD)sizeof(payloadSize), &bytesWritten, nullptr);
            !result || bytesWritten != sizeof(bytesWritten)) {
            throw std::runtime_error{ "Failure writing header" };
        }
        // transmit the payload
        bytesWritten = 0;
        if (BOOL result = WriteFile(hPipe_, bufferView.data(), payloadSize, &bytesWritten, nullptr);
            !result || bytesWritten != payloadSize) {
            throw std::runtime_error{ "Failure writing header" };
        }
    }
}
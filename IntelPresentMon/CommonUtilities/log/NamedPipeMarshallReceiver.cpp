#include "NamedPipeMarshallSender.h"
#include "NamedPipeMarshallReceiver.h"
#include <stdexcept>
#include <sstream>
#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>

namespace pmon::util::log
{
    NamedPipeMarshallReceiver::NamedPipeMarshallReceiver(const std::wstring& pipeName)
        :
        pipeName_(L"\\\\.\\pipe\\" + pipeName)
    {
        // Open the named pipe with overlapped option enabled
        hPipe_ = (win::Handle)CreateFileW(
            pipeName_.c_str(),            // Pipe name 
            GENERIC_READ,                 // Desired access: Read access 
            0,                            // No sharing 
            NULL,                         // Default security attributes
            OPEN_EXISTING,                // Opens existing pipe 
            FILE_FLAG_OVERLAPPED,         // Use overlapped (asynchronous) mode
            NULL);                        // No template file 
        // Check if the pipe handle is valid
        if (!hPipe_) {
            throw std::runtime_error("Failed to open named pipe");
        }
    }

    NamedPipeMarshallReceiver::~NamedPipeMarshallReceiver()
    {
        SignalExit();
    }

    std::optional<Entry> NamedPipeMarshallReceiver::Pop()
    {
        if (sealed_) {
            throw std::runtime_error{ "Pop called after pipe sealed due to disconnection/error/exit signal" };
        }

        // perform the read sequence
        // 1. Initiate an overlapped read for the header
        DWORD bytesRead;
        uint32_t payloadSize = 0;
        overlapped_.Reset();
        if (!ReadFile(hPipe_, &payloadSize, sizeof(payloadSize), nullptr, overlapped_) &&
            GetLastError() != ERROR_IO_PENDING) {
            // TODO: differentiate types of error and log unexpected ones (e.g. pipe disconnected is not an error)
            sealed_ = true;
            return {};
        }

        // 2. Wait for the read operation or exit signal
        {
            const auto eventIndex = win::WaitAnyEvent(exitEvent_, overlapped_.GetEvent());
            if (eventIndex == 0) { // Exit event is signaled
                CancelIo(hPipe_);
                sealed_ = true;
                return {};
            }
        }

        // 3. Get the result of the read operation
        if (auto result = GetOverlappedResult(hPipe_, overlapped_, &bytesRead, FALSE);
            !result || bytesRead != sizeof(payloadSize)) {
            // TODO: differentiate types of error and log unexpected ones (e.g. pipe disconnected is not an error)
            sealed_ = true;
            return {};
        }

        // Payload size read successfully, prepare to read the payload using the same pattern

        // Reset OVERLAPPED for payload read
        overlapped_.Reset();
        inputBuffer_.resize(payloadSize); // Resize buffer for payload

        // Initiate an overlapped read for the payload
        if (!ReadFile(hPipe_, inputBuffer_.data(), (DWORD)inputBuffer_.size(), nullptr, overlapped_) &&
            GetLastError() != ERROR_IO_PENDING) {
            // TODO: differentiate types of error and log unexpected ones (e.g. pipe disconnected is not an error)
            sealed_ = true;
            return {};
        }

        // Wait again for the payload read or exit signal
        {
            const auto eventIndex = win::WaitAnyEvent(exitEvent_, overlapped_.GetEvent());
            if (eventIndex == 0) { // Exit event is signaled
                CancelIo(hPipe_);
                sealed_ = true;
                return {};
            }
        }

        // Check the result of the payload read operation
        if (auto result = GetOverlappedResult(hPipe_, overlapped_, &bytesRead, FALSE);
            !result || bytesRead != payloadSize) {
            // TODO: differentiate types of error and log unexpected ones (e.g. pipe disconnected is not an error)
            sealed_ = true;
            return {};
        }

        // payload read, deserialize binary bytes
        // TODO: figure out a more efficient method, maybe a custom stream type
        std::istringstream stream{ inputBuffer_ };
        cereal::BinaryInputArchive archive(stream);
        std::optional<Entry> entry{ std::in_place };
        archive(*entry);
        return entry;
    }

    void NamedPipeMarshallReceiver::SignalExit()
    {
        // TODO: log error
        SetEvent(exitEvent_);
    }
}
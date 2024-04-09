#include "NamedPipeMarshallSender.h"
#include "NamedPipeMarshallReceiver.h"
#include <stdexcept>
#include <sstream>
#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>

namespace pmon::util::log
{
    NamedPipeMarshallReceiver::NamedPipeMarshallReceiver(const std::wstring& pipeName) : pipeName_(L"\\\\.\\pipe\\" + pipeName)
    {    
        // Initialize events for overlapped operations
        connectEvent_ = (win::Handle)CreateEvent(NULL, TRUE, FALSE, NULL); // Manual reset, initially non-signaled
        readEvent_ = (win::Handle)CreateEvent(NULL, TRUE, FALSE, NULL); // Manual reset, initially non-signaled
        exitEvent_ = (win::Handle)CreateEvent(NULL, TRUE, FALSE, NULL); // Manual reset event for exit signal

        if (!connectEvent_ || !readEvent_ || !exitEvent_) {
            throw std::runtime_error("Failed to create events for overlapped operations");
        }

        overlappedConnect_.hEvent = connectEvent_;
        overlappedRead_.hEvent = readEvent_;

        // Create the named pipe with overlapped option enabled
        hPipe_ = (win::Handle)CreateNamedPipeW(
            pipeName_.c_str(),
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, // Use overlapped (asynchronous) mode
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, // Single instance
            0, // Default buffer sizes
            0,
            0, // Default timeout
            nullptr); // Default security

        if (hPipe_ == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to create named pipe");
        }

        // Initiate an asynchronous connection        
        if (BOOL connected = ConnectNamedPipe(hPipe_, &overlappedConnect_); 
            !connected && GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_PIPE_CONNECTED) {
            throw std::runtime_error("Failed to initiate connect to named pipe");
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
        // only perform connection wait on the first call to Pop()
        if (!connected_) {
            // Wait for either the connection to be established or the exit signal
            {
                HANDLE waitHandles[] = { overlappedConnect_.hEvent, exitEvent_ };
                if (auto wait = WaitForMultipleObjects((DWORD)std::size(waitHandles), waitHandles, FALSE, INFINITE);
                    wait == WAIT_OBJECT_0 + 1) { // Exit event is signaled
                    return {};
                }
            }
            // Ensure connection is established before proceeding
            {
                DWORD bytesTransferred;
                if (!GetOverlappedResult(hPipe_, &overlappedConnect_, &bytesTransferred, FALSE)) {
                    // Handle error or disconnection
                    sealed_ = true;
                    return {};
                }
            }
            connected_ = true;
        }


        // perform the read sequence
        // 1. Initiate an overlapped read for the header
        DWORD bytesRead;
        uint32_t payloadSize = 0;
        ResetEvent(readEvent_);
        if (!ReadFile(hPipe_, &payloadSize, sizeof(payloadSize), &bytesRead, &overlappedRead_) &&
            GetLastError() != ERROR_IO_PENDING) {
            // TODO: differentiate types of error and log unexpected ones (e.g. pipe disconnected is not an error)
            sealed_ = true;
            return {};
        }

        // 2. Wait for the read operation or exit signal
        {
            HANDLE waitHandles[] = { readEvent_, exitEvent_ };
            DWORD wait = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0 + 1) { // Exit event is signaled
                CancelIo(hPipe_);
                sealed_ = true;
                return {};
            }
        }

        // 3. Get the result of the read operation
        if (auto result = GetOverlappedResult(hPipe_, &overlappedRead_, &bytesRead, FALSE);
            !result || bytesRead != sizeof(payloadSize)) {
            sealed_ = true;
            return {};
        }

        // Payload size read successfully, prepare to read the payload using the same pattern

        // Reset OVERLAPPED for payload read
        ResetEvent(readEvent_);
        inputBuffer_.resize(payloadSize); // Resize buffer for payload

        // Initiate an overlapped read for the payload
        if (!ReadFile(hPipe_, inputBuffer_.data(), (DWORD)inputBuffer_.size(), &bytesRead, &overlappedRead_) &&
            GetLastError() != ERROR_IO_PENDING) {
            sealed_ = true;
            return {};
        }

        // Wait again for the payload read or exit signal
        {
            HANDLE waitHandles[] = { overlappedConnect_.hEvent, exitEvent_ };            
            if (auto wait = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE); 
                wait == WAIT_OBJECT_0 + 1) { // Exit event is signaled
                CancelIo(hPipe_); // Cancel the read operation
                sealed_ = true;
                return {};
            }
        }

        // Check the result of the payload read operation
        if (auto result = GetOverlappedResult(hPipe_, &overlappedRead_, &bytesRead, FALSE);
            !result || bytesRead != payloadSize) {
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
#include "NamedPipeMarshallSender.h"
#include "NamedPipeMarshallReceiver.h"
#include <stdexcept>
#include <sstream>
#include <vector>
#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>
#include "MarshallingProtocol.h"

namespace pmon::util::log
{
    NamedPipeMarshallReceiver::NamedPipeMarshallReceiver(const std::wstring& pipeName, class IdentificationTable* pTable)
        :
        pipeName_(L"\\\\.\\pipe\\" + pipeName),
        pIdTable_{ pTable }
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

        while (true) {
            // perform the read sequence
            // 1. Initiate an overlapped read for the header
            DWORD bytesRead;
            uint32_t payloadSize = 0;
            overlapped_.Reset();
            if (!ReadFile(hPipe_, &payloadSize, sizeof(payloadSize), nullptr, overlapped_) &&
                GetLastError() != ERROR_IO_PENDING) {
                // TODO: differentiate types of error and log unexpected ones (e.g. pipe disconnected is not an error)
                sealed_ = true;
                break;
            }

            // 2. Wait for the read operation or exit signal
            {
                const auto eventIndex = win::WaitAnyEvent(exitEvent_, overlapped_.GetEvent());
                if (eventIndex == 0) { // Exit event is signaled
                    CancelIo(hPipe_);
                    sealed_ = true;
                    break;
                }
            }

            // 3. Get the result of the read operation
            if (auto result = GetOverlappedResult(hPipe_, overlapped_, &bytesRead, FALSE);
                !result || bytesRead != sizeof(payloadSize)) {
                // TODO: differentiate types of error and log unexpected ones (e.g. pipe disconnected is not an error)
                sealed_ = true;
                break;
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
                break;
            }

            // Wait again for the payload read or exit signal
            {
                const auto eventIndex = win::WaitAnyEvent(exitEvent_, overlapped_.GetEvent());
                if (eventIndex == 0) { // Exit event is signaled
                    CancelIo(hPipe_);
                    sealed_ = true;
                    break;
                }
            }

            // Check the result of the payload read operation
            if (auto result = GetOverlappedResult(hPipe_, overlapped_, &bytesRead, FALSE);
                !result || bytesRead != payloadSize) {
                // TODO: differentiate types of error and log unexpected ones (e.g. pipe disconnected is not an error)
                sealed_ = true;
                break;
            }

            // payload read, deserialize binary bytes
            // TODO: figure out a more efficient method, maybe a custom stream type
            std::istringstream stream{ inputBuffer_ };
            cereal::BinaryInputArchive archive(stream);
            MarshallPacket packet;
            archive(packet);
            if (auto pBulk = std::get_if<IdentificationTable::Bulk>(&packet)) {
                if (pIdTable_) {
                    for (auto& p : pBulk->processes) {
                        pIdTable_->AddProcess(p.pid, std::move(p.name));
                    }
                    for (auto& t : pBulk->threads) {
                        pIdTable_->AddThread(t.tid, t.pid, std::move(t.name));
                    }
                }
            }
            else if (auto pEntry = std::get_if<Entry>(&packet)) {
                return { std::move(*pEntry) };
            }
            else {
                break;
            }
        }
        return {};
    }

    void NamedPipeMarshallReceiver::SignalExit()
    {
        exitEvent_.Set();
    }
}
#include <cstring>
#include <stdexcept>
#include "ConcreteMiddleware.h"
#include "../../PresentMonUtils/NamedPipeHelper.h"

namespace pmon::mid
{
    static const uint32_t kMaxRespBufferSize = 4096;
	ConcreteMiddleware::ConcreteMiddleware()
	{
        LPCTSTR pipe_name = TEXT("\\\\.\\pipe\\presentmonsvcnamedpipe");

        HANDLE namedPipeHandle;
        // Try to open a named pipe; wait for it, if necessary.
        while (1) {
            namedPipeHandle = CreateFile(pipe_name,
                GENERIC_READ | GENERIC_WRITE,
                0,              
                NULL,           
                OPEN_EXISTING,  
                0,              
                NULL);          

            // Break if the pipe handle is valid.
            if (namedPipeHandle != INVALID_HANDLE_VALUE) {
                break;
            }

            // Exit if an error other than ERROR_PIPE_BUSY occurs.
            if (GetLastError() != ERROR_PIPE_BUSY) {
                throw std::runtime_error{ "Service not found" };
            }

            // All pipe instances are busy, so wait for 20 seconds.
            if (!WaitNamedPipe(pipe_name, 20000)) {
                throw std::runtime_error{ "Pipe sessions full" };
            }
        }
        // The pipe connected; change to message-read mode.
        DWORD mode = PIPE_READMODE_MESSAGE;
        BOOL success = SetNamedPipeHandleState(namedPipeHandle,
            &mode,
            NULL,
            NULL);
        if (!success) {
            throw std::runtime_error{ "Pipe error" };
        }
        testHandle.reset(namedPipeHandle);
        clientProcessId = GetCurrentProcessId();
	}

	void ConcreteMiddleware::Speak(char* buffer) const
	{
		strcpy_s(buffer, 256, "concrete-middle");
	}

    PM_STATUS ConcreteMiddleware::SendRequest(MemBuffer* requestBuffer) {
        DWORD bytesWritten;
        BOOL success = WriteFile(
            uniqueHandleMember,
            requestBuffer->AccessMem(),
            static_cast<DWORD>(requestBuffer->GetCurrentSize()),
            &bytesWritten,
            NULL);

        if (success && requestBuffer->GetCurrentSize() == bytesWritten) {
            return PM_STATUS::PM_STATUS_SUCCESS;
        }
        else {
            return PM_STATUS::PM_STATUS_FAILURE;
        }
    }

    PM_STATUS ConcreteMiddleware::ReadResponse(MemBuffer* responseBuffer) {
        BOOL success;
        DWORD bytesRead;
        BYTE inBuffer[kMaxRespBufferSize];
        ZeroMemory(&inBuffer, sizeof(inBuffer));

        do {
            // Read from the pipe using a nonoverlapped read
            success = ReadFile(uniqueHandleMember,
                inBuffer,
                sizeof(inBuffer),
                &bytesRead,
                NULL);

            // If the call was not successful AND there was
            // no more data to read bail out
            if (!success && GetLastError() != ERROR_MORE_DATA) {
                break;
            }

            // Either the call was successful or there was more
            // data in the pipe. In both cases add the response data
            // to the memory buffer
            responseBuffer->AddItem(inBuffer, bytesRead);
        } while (!success);  // repeat loop if ERROR_MORE_DATA

        if (success) {
            return PM_STATUS::PM_STATUS_SUCCESS;
        }
        else {
            return PM_STATUS::PM_STATUS_FAILURE;
        }
    }

    PM_STATUS ConcreteMiddleware::CallPmService(MemBuffer* requestBuffer, MemBuffer* responseBuffer)
    {

    }

    PM_STATUS ConcreteMiddleware::OpenSession(uint32_t processId)
    {
        MemBuffer requestBuf;
        MemBuffer responseBuf;

        NamedPipeHelper::EncodeStartStreamingRequest(&requestBuf, clientProcessId,
            processId, nullptr);

        return PM_STATUS_SUCCESS;
    }
    
    PM_STATUS ConcreteMiddleware::CloseSession(uint32_t processId)
    {

        return PM_STATUS_SUCCESS;
    }
}
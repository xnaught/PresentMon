// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "..\CommonUtilities\str\String.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../Interprocess/source/act/AsyncActionManager.h"
#include "CliOptions.h"
#include "ActionServer.h"
#include "GlobalIdentifiers.h"
#include <thread>
#include <sddl.h>

using namespace pmon;
using namespace util;
using namespace ipc;
namespace as = boost::asio;

class ActionServerImpl_
{
public:
    ActionServerImpl_(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName)
        :
        pipeName_{ pipeName.value_or(gid::defaultControlPipeName) },
        elevatedSecurity_{ !pipeName }
    {
        actionManager_.ctx_.pSvc = pSvc;
        actionManager_.ctx_.pPmon = pPmon;
        thread_ = std::jthread{ [this] {
            // maintain 3 available pipe instances at all times
            for (int i = 0; i < 3; i++) {
                as::co_spawn(ioctx_, AcceptConnection_(), as::detached);
            }
            // run the io context event handler until signalled to exit
            ioctx_.run();
        } };
    }
    ActionServerImpl_(const ActionServerImpl_&) = delete;
    ActionServerImpl_& operator=(const ActionServerImpl_&) = delete;
    ActionServerImpl_(ActionServerImpl_&&) = delete;
    ActionServerImpl_& operator=(ActionServerImpl_&&) = delete;
    ~ActionServerImpl_() = default;
private:
    // functions
    as::awaitable<void> AcceptConnection_()
    {
        auto pipe = pipe::DuplexPipe::Make(pipeName_, ioctx_);
        co_await pipe.Accept();
        // fork acceptor coroutine
        as::co_spawn(ioctx_, AcceptConnection_(), as::detached);
        // run the action handler until client session is terminated
        while (true) {
            co_await actionManager_.SyncHandleRequest(pipe, pipe.readBuf_, pipe.writeBuf_);
        }
    }
    // data
    std::string pipeName_;
    bool elevatedSecurity_; // for now, assume that security is elevated only when using default pipe name
    as::io_context ioctx_;
    act::AsyncActionManager<ServiceExecutionContext> actionManager_;
    std::jthread thread_;
};

ActionServer::ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName)
    :
    pImpl_{ std::make_shared<ActionServerImpl_>(pSvc, pPmon, std::move(pipeName)) }
{}

//DWORD NamedPipeServerX::Pipe::CreatePipeInstance(LPCTSTR pipe_name, int max_pipes, uint32_t pipe_timeout) {
//    LOG(INFO) << "Creating control pipe with name: [" << util::str::ToNarrow(pipe_name) << "]";
//
//    auto& opt = clio::Options::Get();
//    if (opt.controlPipe.AsOptional().has_value()) {
//        
//        PSECURITY_DESCRIPTOR pSD = NULL;
//        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
//            L"D:(A;OICI;GA;;;WD)", SDDL_REVISION_1,
//            &pSD, NULL)) {
//            auto error = GetLastError();
//            LOG(INFO) << "Failed to create security descriptor: [" <<
//                util::str::ToNarrow(pipe_name) <<
//                "] Error: " <<
//                error << std::endl;
//            return error;
//        }
//        SECURITY_ATTRIBUTES sa = { .nLength = sizeof(sa),
//                                   .lpSecurityDescriptor = pSD,
//                                   .bInheritHandle = FALSE};
//        HANDLE tempPipeInstance =
//            CreateNamedPipe(pipe_name,  // pipe name
//                PIPE_ACCESS_DUPLEX |    // read/write access
//                FILE_FLAG_OVERLAPPED,   // overlapped mode
//                PIPE_TYPE_MESSAGE |     // message-type pipe
//                PIPE_READMODE_MESSAGE | // message-read mode
//                PIPE_WAIT,              // blocking mode
//                max_pipes,              // number of instances
//                MaxBufferSize,          // output buffer size
//                MaxBufferSize,          // input buffer size
//                pipe_timeout,           // client time-out
//                &sa);                   // security attributes
//        // Free the allocated security descriptor regardless if the
//        // named pipe was successfully created
//        LocalFree(sa.lpSecurityDescriptor);
//        if (tempPipeInstance == INVALID_HANDLE_VALUE) {
//            auto error = GetLastError();
//            LOG(INFO) << "Failed to create pipe: [" <<
//                util::str::ToNarrow(pipe_name) <<
//                "] Error: " <<
//                error << std::endl;
//            return error;
//        }
//        mPipeInstance.reset(tempPipeInstance);
//        return ERROR_SUCCESS;
//    }
//    else {
//        PSECURITY_DESCRIPTOR pSD = NULL;
//        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
//            L"D:PNO_ACCESS_CONTROLS:(ML;;NW;;;LW)", SDDL_REVISION_1,
//            &pSD, NULL)) {
//            auto error = GetLastError();
//            LOG(INFO) << "Failed to create security descriptor: [" <<
//                util::str::ToNarrow(pipe_name) <<
//                "] Error: " <<
//                error << std::endl;
//            return error;
//        }
//        SECURITY_ATTRIBUTES sa = { .nLength = sizeof(sa),
//                                   .lpSecurityDescriptor = pSD,
//                                   .bInheritHandle = FALSE };
//        HANDLE tempPipeInstance =
//            CreateNamedPipe(pipe_name,   // pipe name
//                PIPE_ACCESS_DUPLEX |     // read/write access
//                FILE_FLAG_OVERLAPPED,    // overlapped mode
//                PIPE_TYPE_MESSAGE |      // message-type pipe
//                PIPE_READMODE_MESSAGE |  // message-read mode
//                PIPE_WAIT,               // blocking mode
//                max_pipes,               // number of instances
//                MaxBufferSize,           // output buffer size
//                MaxBufferSize,           // input buffer size
//                pipe_timeout,            // client time-out
//                &sa);                    // use specified security attributes
//        // Free the allocated security descriptor regardless if the
//        // named pipe was successfully created
//        LocalFree(sa.lpSecurityDescriptor);
//        if (tempPipeInstance == INVALID_HANDLE_VALUE) {
//            auto error = GetLastError();
//            LOG(INFO) << "Failed to create pipe: [" <<
//                util::str::ToNarrow(pipe_name) <<
//                "] Error: " <<
//                error << std::endl;
//            return error;
//        }
//        mPipeInstance.reset(tempPipeInstance);
//        return ERROR_SUCCESS;
//    }
//}
// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/pipe/Pipe.h"
#include "../CommonUtilities/str/String.h"
#include "../Interprocess/source/act/ActionServer.h"
#include "ActionServer.h"
#include "GlobalIdentifiers.h"
#include "ServiceExecutionContext.h"


namespace pmon::svc
{
    using namespace pmon;
    using namespace util;
    using namespace ipc;

    ActionServer::ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName)
    {
        // if we have a pipe name override
        auto sec = pipe::DuplexPipe::GetSecurityString(!pipeName ?
            pipe::SecurityMode::Service : pipe::SecurityMode::Child);
        act::ActionServerImpl_<ServiceExecutionContext>::LaunchThread(
            ServiceExecutionContext{ .pSvc = pSvc, .pPmon = pPmon },
            pipeName.value_or(gid::defaultControlPipeName), 2, std::move(sec)
        ).detach();
    }
}
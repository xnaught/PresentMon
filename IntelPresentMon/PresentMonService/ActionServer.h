// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <optional>
#include "PresentMon.h"
#include "Service.h"

namespace pmon::svc
{
    struct ServiceExecutionContext
    {
        Service* pSvc;
        PresentMon* pPmon;
    };

    class ActionServer
    {
    public:
        ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName);
        ~ActionServer() = default;
        ActionServer(const ActionServer&) = delete;
        ActionServer& operator=(const ActionServer&) = delete;
        ActionServer(ActionServer&&) = delete;
        ActionServer& operator=(ActionServer&&) = delete;
    private:
        std::shared_ptr<class ActionServerImpl_> pImpl_;
    };
}

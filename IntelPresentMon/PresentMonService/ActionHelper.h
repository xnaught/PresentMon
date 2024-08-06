#pragma once
#include "../Interprocess/source/act/AsyncAction.h"
#include "../CommonUtilities/Macro.h"
#include "ActionServer.h"

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
#include "../Interprocess/source/act/AsyncActionCollection.h"
#define ACTION_REG(name) ::pmon::ipc::act::AsyncActionRegistrator<::pmon::svc::acts::name, ServiceExecutionContext> reg_##name##_;
#endif
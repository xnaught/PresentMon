#pragma once
#include "../Interprocess/source/act/AsyncAction.h"
#include "../CommonUtilities/Macro.h"
#include "../CommonUtilities/log/Log.h"
#include "ActionServer.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
#include "../Interprocess/source/act/AsyncActionCollection.h"
#define ACTION_REG(name) ::pmon::ipc::act::AsyncActionRegistrator<::pmon::svc::acts::name, ServiceExecutionContext> regSvcAct_##name##_;
#endif
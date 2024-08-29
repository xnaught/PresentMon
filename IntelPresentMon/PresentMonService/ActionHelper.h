#pragma once
#include "../Interprocess/source/act/AsyncAction.h"
#include "../CommonUtilities/Macro.h"
#include "../CommonUtilities/log/Log.h"
#include "ServiceExecutionContext.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/optional.hpp>

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
#include "../Interprocess/source/act/AsyncActionCollection.h"
#define ACTION_REG(name) ::pmon::ipc::act::AsyncActionRegistrator<::pmon::svc::acts::name, ::pmon::svc::ServiceExecutionContext> CONCATENATE(regSvcAct_, name)##_;
#endif

#define ACTION_TRAITS_DEF(name) namespace pmon::ipc::act { template<> struct ActionParamsTraits<::pmon::svc::acts::name::Params> { using Action = ::pmon::svc::acts::name; }; }
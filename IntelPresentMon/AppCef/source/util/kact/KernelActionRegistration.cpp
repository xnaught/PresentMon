#include "../../../CommonUtilities/Macro.h"
#define PM_ASYNC_ACTION_REGISTRATION_
#define PM_ASYNC_ACTION_CUSTOM_REG_(name) IpcActionRegistrator<name> CONCATENATE(reg_ibind_, name)_;

#include "../IpcInvocationManager.h"
namespace p2c::client::util::kact {
	template<class A>
	struct IpcActionRegistrator {
		IpcActionRegistrator() { ::p2c::client::util::IpcInvocationManager::RegisterDispatchBinding<A>(); }
	};
}

#include "AllActions.h"
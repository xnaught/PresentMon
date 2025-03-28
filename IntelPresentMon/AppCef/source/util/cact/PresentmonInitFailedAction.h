#pragma once
#include "../../../Interprocess/source/act/ActionHelper.h"
#include "CefExecutionContext.h"
#include "../../Action.h"
#include <format>
#include <include/cef_task.h> 
#include <include/base/cef_callback.h> 
#include <include/wrapper/cef_closure_task.h> 

#define ACT_NAME PresentmonInitFailedAction
#define ACT_EXEC_CTX CefExecutionContext
#define ACT_TYPE AsyncEventActionBase_
#define ACT_NS ::p2c::client::util::cact

namespace p2c::client::util::cact
{
	using namespace ::pmon::ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			template<class A> void serialize(A& ar) {}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static void Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			CefPostTask(TID_RENDERER, base::BindOnce(&SignalManager::SignalPresentmonInitFailed,
				base::Unretained(ctx.pSignalManager)));
		}
	};

#ifdef PM_ASYNC_ACTION_REGISTRATION_
	ACTION_REG();
#endif
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
#pragma once
#include "../../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include "../KernelWrapper.h"
#include <format>
#include <Core/source/kernel/Kernel.h>

#define ACT_NAME SetAdapter
#define ACT_EXEC_CTX KernelExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS ::p2c::client::util::kact

namespace p2c::client::util::kact
{
	using namespace ::pmon::ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			uint32_t id;

			template<class A> void serialize(A& ar) {
				ar(id);
			}
		};
		struct Response {
			template<class A> void serialize(A& ar) {
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			(*ctx.ppKernel)->SetAdapter(in.id);
			return {};
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
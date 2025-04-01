#pragma once
#include "../../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include "../KernelWrapper.h"
#include <format>
#include <Core/source/kernel/Kernel.h>

#define ACT_NAME EnumerateAdapters
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
			template<class A> void serialize(A& ar) {
			}
		};
		struct Response {
			std::vector<p2c::pmon::AdapterInfo> adapters;
			template<class A> void serialize(A& ar) {
				ar(adapters);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			return { (*ctx.ppKernel)->EnumerateAdapters() };
		}
	};

#ifdef PM_ASYNC_ACTION_REGISTRATION_
	ACTION_REG();
#endif
}

namespace cereal
{
	template<class Archive>
	void serialize(Archive& archive, p2c::pmon::AdapterInfo& s)
	{
		archive(s.id, s.name, s.vendor);
	}
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
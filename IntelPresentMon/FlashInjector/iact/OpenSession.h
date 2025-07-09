#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "InjectorExecutionContext.h"
#include <format>

#define ACT_NAME OpenSession
#define ACT_EXEC_CTX InjectorExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS inj::iact

namespace ACT_NS
{
	using namespace ::pmon::ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			uint32_t remotePid;
			InjectorSessionContext::Type remoteType;

			template<class A> void serialize(A& ar) {
				ar(remotePid, remoteType);
			}
		};
		struct Response {
			uint32_t injectorPid;

			template<class A> void serialize(A& ar) {
				ar(injectorPid);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			stx.remotePid = in.remotePid;
			stx.clientType = in.remoteType;
			const Response res{ .injectorPid = GetCurrentProcessId() };
			pmlog_info(std::format("Injector open action for cli={} inj={}", in.remotePid, res.injectorPid));
			return res;
		}
	};

	ACTION_REG();
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
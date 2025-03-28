#pragma once
#include "../../../Interprocess/source/act/ActionHelper.h"
#include "CefExecutionContext.h"
#include <format>

#define ACT_NAME HotkeyAction
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
			uint32_t clientPid;

			template<class A> void serialize(A& ar) {
				ar(clientPid);
			}
		};
		struct Response {
			uint32_t servicePid;

			template<class A> void serialize(A& ar) {
				ar(servicePid);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			stx.remotePid = in.clientPid;
			const Response res{ .servicePid = GetCurrentProcessId() };
			pmlog_info(std::format("Kernel open action for cli={} svc={}", in.clientPid, res.servicePid));
			return res;
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
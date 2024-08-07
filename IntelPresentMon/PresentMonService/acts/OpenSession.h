#pragma once
#include "../ActionHelper.h"
#include <format>

#define ACTNAME OpenSession

namespace pmon::svc::acts
{
	using namespace ipc::act;

	class ACTNAME : public AsyncActionBase_<ACTNAME, ServiceExecutionContext>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACTNAME);
		struct Params
		{
			uint32_t clientPid;

			template<class A> void serialize(A& ar) {
				ar(clientPid);
			}
		};
		struct Response
		{
			std::string str;

			template<class A> void serialize(A& ar) {
				ar(str);
			}
		};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, Params&& in)
		{
			ctx.pSvc->SignalClientSessionOpened();
			Response out;
			out.str = std::format("session-opened:{}", in.clientPid);
			pmlog_dbg(std::format("Received open session action from: {}", in.clientPid));
			return out;
		}
	};
}

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
ACTION_REG(ACTNAME);
#endif

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME
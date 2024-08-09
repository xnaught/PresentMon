#pragma once
#include "../ActionHelper.h"
#include "../../CommonUtilities/generated/build_id.h"
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
			std::string clientBuildId;

			template<class A> void serialize(A& ar) {
				ar(clientPid, clientBuildId);
			}
		};
		struct Response {
			std::string serviceBuildId;

			template<class A> void serialize(A& ar) {
				ar(serviceBuildId);
			}
		};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			stx.clientPid = in.clientPid;
			stx.clientBuildId = in.clientBuildId;
			ctx.pSvc->SignalClientSessionOpened();
			pmlog_dbg(std::format("Received open session action from {} BID={} vs {} (svc)",
				in.clientPid, in.clientBuildId, PM_BID_GIT_HASH_SHORT_NARROW));
			return Response{ .serviceBuildId = PM_BID_GIT_HASH_SHORT_NARROW };
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME
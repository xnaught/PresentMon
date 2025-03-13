#pragma once
#include "../ActionHelper.h"
#include "../../Versioning/BuildId.h"
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
			std::string clientBuildConfig;

			template<class A> void serialize(A& ar) {
				ar(clientPid, clientBuildId, clientBuildConfig);
			}
		};
		struct Response {
			std::string serviceBuildId;
			std::string serviceBuildConfig;

			template<class A> void serialize(A& ar) {
				ar(serviceBuildId, serviceBuildConfig);
			}
		};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			stx.clientPid = in.clientPid;
			stx.clientBuildId = in.clientBuildId;
			ctx.pSvc->SignalClientSessionOpened();
			pmlog_info(std::format("Open action for session #{} pid={}; [BID] cli={} svc={} [CFG] cli={} svc={}",
				stx.pPipe->GetId(), in.clientPid, in.clientBuildId, bid::BuildIdShortHash(),
				in.clientBuildConfig, bid::BuildIdConfig()));
			return Response{ .serviceBuildId = bid::BuildIdShortHash(), .serviceBuildConfig = bid::BuildIdConfig()};
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME
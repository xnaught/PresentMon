#pragma once
#include "../ActionHelper.h"
#include <format>

#define ACTNAME StopStream

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
			uint32_t targetPid;

			template<class A> void serialize(A& ar) {
				ar(clientPid, targetPid);
			}
		};
		struct Response {
			template<class A> void serialize(A& ar) {}
		};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, Params&& in)
		{
			ctx.pPmon->StopStreaming(in.clientPid, in.targetPid);
			pmlog_dbg(std::format("StopStreaming action from [{}] un-targeting [{}]", in.clientPid, in.targetPid));
			return {};
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME
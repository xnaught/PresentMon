#pragma once
#include "../ActionHelper.h"
#include <format>

#define ACTNAME StartTracking

namespace pmon::svc::acts
{
	using namespace ipc::act;

	class ACTNAME : public AsyncActionBase_<ACTNAME, ServiceExecutionContext>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACTNAME);
		struct Params
		{
			uint32_t targetPid;

			template<class A> void serialize(A& ar) {
				ar(targetPid);
			}
		};
		struct Response
		{
			std::string nsmFileName;

			template<class A> void serialize(A& ar) {
				ar(nsmFileName);
			}
		};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			std::string nsmFileName;
			if (auto sta = ctx.pPmon->StartStreaming(stx.clientPid, in.targetPid, nsmFileName); sta != PM_STATUS_SUCCESS) {
				pmlog_error("Start stream failed").code(sta);
				throw util::Except<ActionExecutionError>(sta);
			}
			stx.trackedPids.insert(in.targetPid);
			const Response out{ .nsmFileName = std::move(nsmFileName) };
			pmlog_info(std::format("StartTracking action from [{}] targeting [{}] assigned nsm [{}]",
				stx.clientPid, in.targetPid, out.nsmFileName));
			return out;
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME
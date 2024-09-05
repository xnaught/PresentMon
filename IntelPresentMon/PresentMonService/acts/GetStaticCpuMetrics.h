#pragma once
#include "../ActionHelper.h"
#include <format>

#define ACTNAME GetStaticCpuMetrics

namespace pmon::svc::acts
{
	using namespace ipc::act;

	class ACTNAME : public AsyncActionBase_<ACTNAME, ServiceExecutionContext>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACTNAME);
		struct Params {};
		struct Response
		{
			std::string cpuName;
			double cpuPowerLimit;

			template<class A> void serialize(A& ar) {
				ar(cpuName, cpuPowerLimit);
			}
		};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			const Response out{
				.cpuName = ctx.pPmon->GetCpuName(),
				.cpuPowerLimit = ctx.pPmon->GetCpuPowerLimit()
			};
			pmlog_dbg(std::format("static CPU metrics gotten for {} @ {}W", out.cpuName, out.cpuPowerLimit));
			return out;
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME
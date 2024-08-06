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
		struct Params
		{
			template<class A> void serialize(A&) {}
		};
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
		static Response Execute_(const ServiceExecutionContext& ctx, Params&& in)
		{
			const Response out{
				.cpuName = ctx.pPmon->GetCpuName(),
				.cpuPowerLimit = ctx.pPmon->GetCpuPowerLimit()
			};
			std::cout << std::format("GetStaticCpuMetrics action from [???] {} @ {}",
				out.cpuName, out.cpuPowerLimit) << std::endl;
			return out;
		}
	};
}

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
ACTION_REG(ACTNAME);
#endif

#undef ACTNAME
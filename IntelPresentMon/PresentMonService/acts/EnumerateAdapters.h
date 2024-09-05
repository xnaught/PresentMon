#pragma once
#include "../ActionHelper.h"
#include <format>
#include <ranges>

#define ACTNAME EnumerateAdapters

namespace pmon::svc::acts
{
	using namespace ipc::act;
	namespace rn = std::ranges;
	namespace vi = rn::views;

	class ACTNAME : public AsyncActionBase_<ACTNAME, ServiceExecutionContext>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACTNAME);
		struct Params {};
		struct Response
		{
			struct Adapter
			{
				uint32_t id;
				PM_DEVICE_VENDOR vendor;
				std::string name;
				double gpuSustainedPowerLimit;
				uint64_t gpuMemorySize;
				uint64_t gpuMemoryMaxBandwidth;

				template<class A> void serialize(A& ar) {
					ar(id, vendor, name, gpuSustainedPowerLimit, gpuMemorySize, gpuMemoryMaxBandwidth);
				}
			};
			std::vector<Adapter> adapters;

			template<class A> void serialize(A& ar) {
				ar(adapters);
			}
		};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			Response out;
			for (auto&&[i, adapter] : ctx.pPmon->EnumerateAdapters() | vi::enumerate) {
				out.adapters.push_back(Response::Adapter{
					.id = (uint32_t)i,
					.vendor = adapter->GetVendor(),
					.name = adapter->GetName(),
					.gpuSustainedPowerLimit = adapter->GetSustainedPowerLimit(),
					.gpuMemorySize = adapter->GetDedicatedVideoMemory(),
					.gpuMemoryMaxBandwidth = adapter->GetVideoMemoryMaxBandwidth(),
				});
			}
			pmlog_dbg(std::format("{} adapters enumerated", out.adapters.size()));
			return out;
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME

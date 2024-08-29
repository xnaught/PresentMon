#pragma once
#include "../ActionHelper.h"
#include <format>
#include <ranges>

#define ACTNAME SelectAdapter

namespace pmon::svc::acts
{
	using namespace ipc::act;
	namespace rn = std::ranges;
	namespace vi = rn::views;

	class ACTNAME : public AsyncActionBase_<ACTNAME, ServiceExecutionContext>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACTNAME);
		struct Params
		{
			uint32_t adapterId;

			template<class A> void serialize(A& ar) {
				ar(adapterId);
			}
		};
		struct Response {};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			if (auto sta = ctx.pPmon->SelectAdapter(in.adapterId); sta != PM_STATUS_SUCCESS) {
				pmlog_error("Select adapter failed").code(sta);
				throw util::Except<ActionExecutionError>(sta);
			}
			stx.requestedAdapterId = in.adapterId;
			pmlog_dbg(std::format("selecting adapter id [{}]", in.adapterId));
			return {};
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME

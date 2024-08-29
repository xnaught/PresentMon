#pragma once
#include "../ActionHelper.h"
#include <format>
#include <ranges>

#define ACTNAME SetEtwFlushPeriod

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
			std::optional<uint32_t> etwFlushPeriodMs;

			template<class A> void serialize(A& ar) {
				ar(etwFlushPeriodMs);
			}
		};
		struct Response {};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			if (auto sta = ctx.pPmon->SetEtwFlushPeriod(in.etwFlushPeriodMs); sta != PM_STATUS_SUCCESS) {
				pmlog_error("Set ETW flush period failed").code(sta);
				throw util::Except<ActionExecutionError>(sta);
			}
			stx.requestedEtwFlushEnabled = bool(in.etwFlushPeriodMs);
			if (in.etwFlushPeriodMs) {
				pmlog_dbg(std::format("Setting ETW flush period to {}ms", *in.etwFlushPeriodMs));
				stx.requestedEtwFlushPeriodMs = *in.etwFlushPeriodMs;
			}
			else {
				pmlog_dbg("Disabling manual ETW flush");
				stx.requestedEtwFlushPeriodMs.reset();
			}
			return {};
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME

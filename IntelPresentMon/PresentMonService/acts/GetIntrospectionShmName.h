#pragma once
#include "../ActionHelper.h"
#include "../PMMainThread.h"
#include <format>

#define ACTNAME GetIntrospectionShmName

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
			std::string name;

			template<class A> void serialize(A& ar) {
				ar(name);
			}
		};
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			const Response out{
				.name = ::GetIntrospectionShmName(),
			};
			pmlog_dbg(std::format("Reported introspection shm name: {}", out.name));
			return out;
		}
	};

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
	ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME
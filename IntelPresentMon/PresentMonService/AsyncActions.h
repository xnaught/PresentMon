#pragma once
#include "../Interprocess/source/act/AsyncAction.h"
#include "NamedPipeServer.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <unordered_map>
#include <format>
#include "NamedPipeServer.h"

namespace pmon::svc::acts
{
	using namespace ipc::act;

	class OpenSessionAction : public AsyncActionBase_<OpenSessionAction, ServiceExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "OpenSession";
		struct Params
		{
			int clientPid;

			template<class A> void serialize(A& ar) {
				ar(clientPid);
			}
		};
		struct Response
		{
			std::string str;

			template<class A> void serialize(A& ar) {
				ar(str);
			}
		};
	private:
		friend class AsyncActionBase_<OpenSessionAction, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, Params&& in)
		{
			ctx.pSvc->SignalClientSessionOpened();
			Response out;
			out.str = std::format("session-opened:{}", in.clientPid);
			std::cout << "Received open session action from: " << in.clientPid << std::endl;
			return out;
		}
	};
}
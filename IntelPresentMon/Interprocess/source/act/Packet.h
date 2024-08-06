#pragma once
#include <string>
#include <cstdint>

namespace pmon::ipc::act
{
	struct RequestHeader
	{
		std::string identifier;
		uint32_t commandToken;
		template<class A> void serialize(A& ar) {
			ar(identifier, commandToken);
		}
	};

	struct ResponseHeader
	{
		uint32_t commandToken;
		int32_t status;
		template<class A> void serialize(A& ar) {
			ar(commandToken, status);
		}
	};

	struct EmptyPayload {};
}
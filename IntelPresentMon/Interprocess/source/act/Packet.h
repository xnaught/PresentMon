#pragma once
#include <string>
#include <cstdint>

namespace pmon::ipc::act
{
	enum class TransportStatus
	{
		Success,
		ExecutionFailure,
		TransportFailure,
	};

	enum class PacketType
	{
		ActionRequest,
		ActionResponse,
		ActionEvent,
	};

	struct PacketHeader
	{
		std::string identifier;
		uint32_t commandToken;
		TransportStatus transportStatus;
		int executionStatus;
		PacketType packetType;
		uint16_t headerVersion;
		uint16_t actionVersion;

		template<class A> void serialize(A& ar) {
			ar(identifier, commandToken, transportStatus, executionStatus,
				packetType, headerVersion, actionVersion);
		}
	};

	struct EmptyPayload {};
}
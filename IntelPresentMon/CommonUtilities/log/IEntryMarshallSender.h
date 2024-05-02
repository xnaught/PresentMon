#pragma once
#include <optional>

namespace pmon::util::log
{
	struct Entry;

	class IEntryMarshallSender
	{
	public:
		virtual ~IEntryMarshallSender() = default;
		virtual void Push(const Entry& entry) = 0;
	};
}
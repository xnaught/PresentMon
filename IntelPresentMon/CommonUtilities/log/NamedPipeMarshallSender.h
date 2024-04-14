#pragma once
#include "IEntryMarshallSender.h"
#include <string>
#include <memory>

namespace pmon::util::log
{
	struct Entry;

	class NamedPipeMarshallSender : public IEntryMarshallSender
	{
	public:
		NamedPipeMarshallSender(const std::wstring& pipeName);
        ~NamedPipeMarshallSender();
        void Push(const Entry& entry);
	private:
		std::shared_ptr<void> pNamedPipe_;
	};
}


#pragma once
#include "IEntryMarshallSender.h"
#include <string>
#include <vector>
#include "../win/Handle.h"

namespace pmon::util::log
{
	class NamedPipeMarshallSender : public IEntryMarshallSender
	{
	public:
		NamedPipeMarshallSender(const std::wstring& pipeName);
        ~NamedPipeMarshallSender();
        void Push(const Entry& entry);
	private:
		std::wstring pipeName_;
        win::Handle hPipe_;
		std::vector<char> inputBuffer_;
	};
}


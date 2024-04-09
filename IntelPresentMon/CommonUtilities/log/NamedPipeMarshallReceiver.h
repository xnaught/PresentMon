#pragma once
#include "IEntryMarshallReceiver.h"
#include <string>
#include "../win/WinAPI.h"
#include "../win/Handle.h"


namespace pmon::util::log
{
	class NamedPipeMarshallReceiver : public IEntryMarshallReceiver
	{
    public:
        NamedPipeMarshallReceiver(const std::wstring& pipeName);
        ~NamedPipeMarshallReceiver();
        std::optional<Entry> Pop() override;
        void SignalExit() override;
    private:
        bool connected_ = false;
        bool sealed_ = false;
        win::Handle hPipe_;
        std::wstring pipeName_;
        std::string inputBuffer_;
        OVERLAPPED overlappedConnect_{};
        OVERLAPPED overlappedRead_{};
        win::Handle exitEvent_;
        win::Handle readEvent_;
        win::Handle connectEvent_;
	};
}


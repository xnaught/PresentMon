#pragma once
#include "IEntryMarshallReceiver.h"
#include <string>
#include "../win/WinAPI.h"
#include "../win/Handle.h"
#include "../win/Event.h"
#include "../win/Overlapped.h"


namespace pmon::util::log
{
	class NamedPipeMarshallReceiver : public IEntryMarshallReceiver
	{
    public:
        NamedPipeMarshallReceiver(const std::wstring& pipeName, class IdentificationTable* pTable = nullptr);
        ~NamedPipeMarshallReceiver();
        std::optional<Entry> Pop() override;
        void SignalExit() override;
    private:
        bool connected_ = false;
        bool sealed_ = false;
        win::Handle hPipe_;
        std::wstring pipeName_;
        std::string inputBuffer_;
        win::Overlapped overlapped_{};
        win::Event exitEvent_;
        class IdentificationTable* pIdTable_ = nullptr;
	};
}


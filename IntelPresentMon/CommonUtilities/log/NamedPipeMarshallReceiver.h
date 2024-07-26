#pragma once
#include "IEntryMarshallReceiver.h"
#include <string>
#include "../win/WinAPI.h"
#include "../win/Handle.h"
#include "../win/Event.h"
#include "../win/Overlapped.h"
#include "../Exception.h"


namespace pmon::util::log
{
    PM_DEFINE_EX(PipeConnectionError);

	class NamedPipeMarshallReceiver : public IEntryMarshallReceiver
	{
    public:
        NamedPipeMarshallReceiver(const std::string& pipeName, class IdentificationTable* pTable = nullptr);

        NamedPipeMarshallReceiver(const NamedPipeMarshallReceiver&) = delete;
        NamedPipeMarshallReceiver & operator=(const NamedPipeMarshallReceiver&) = delete;

        ~NamedPipeMarshallReceiver();
        std::optional<Entry> Pop() override;
        void SignalExit() override;
    private:
        bool connected_ = false;
        bool sealed_ = false;
        win::Handle hPipe_;
        std::string pipeName_;
        std::string inputBuffer_;
        win::Overlapped overlapped_{};
        win::Event exitEvent_;
        class IdentificationTable* pIdTable_ = nullptr;
	};
}


#pragma once
#include "IDriver.h"
#include "IChannel.h"
#include "../../PresentMonAPI2/PresentMonDiagnostics.h"
#include "../win/Event.h"
#include <atomic>
#include <concurrentqueue/concurrentqueue.h>
#include "Entry.h"


namespace pmon::util::log
{
	struct Message : public PM_DIAGNOSTIC_MESSAGE
	{
		std::string messageBuffer_;
	};

	class DiagnosticDriver : public IDriver
	{
	public:
		DiagnosticDriver(PM_DIAGNOSTIC_OUTPUT_FLAGS outflags);
		~DiagnosticDriver();
		void Submit(const Entry&) override;
		void Flush() override;
		uint32_t GetQueuedMessageCount();
		uint32_t GetMaxQueuedMessages();
		void SetMaxQueuedMessages(uint32_t);
		uint32_t GetDiscardedMessageCount();
		std::unique_ptr<Message> DequeueMessage();
		PM_DIAGNOSTIC_WAKE_REASON WaitForMessage(uint32_t timeoutMs);
		void UnblockWaitingThread();

	private:
		bool useWinDbg_;
		bool useCout_;
		bool useCerr_;
		std::atomic<uint32_t> maxQueuedMessages_ = 256;
		std::atomic<uint32_t> discardedCount_ = 0;
		win::Event messageWaitEvent_{ false };
		win::Event manualUnblockEvent_{ false };
		std::atomic<bool> dying_ = false;
		moodycamel::ConcurrentQueue<std::unique_ptr<Message>> messageQueue_;
	};
}

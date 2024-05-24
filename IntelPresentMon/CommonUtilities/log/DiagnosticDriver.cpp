#include "DiagnosticDriver.h"
#include "../Exception.h"

using namespace std::chrono_literals;

namespace pmon::util::log
{
	DiagnosticDriver::DiagnosticDriver(PM_DIAGNOSTIC_OUTPUT_FLAGS outflags)
		:
		useWinDbg_{ bool(outflags & PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER) },
		useCout_{ bool(outflags & PM_DIAGNOSTIC_OUTPUT_FLAGS_STDOUT) },
		useCerr_{ bool(outflags & PM_DIAGNOSTIC_OUTPUT_FLAGS_STDERR) }
	{}
	DiagnosticDriver::~DiagnosticDriver()
	{
		dying_ = true;
		manualUnblockEvent_.Set();
	}
	void DiagnosticDriver::Submit(const Entry& e)
	{
		// ignore non-diagnostic entries
		if (!e.diagnosticLayer_) {
			return;
		}
		// format message as string
		// output formatted string directly to active media
		if (useWinDbg_) {

		}
		if (useCerr_) {

		}
		if (useCout_) {

		}
		// create diagnostic message based on logging entry
		auto pMessage = std::make_unique<Message>();
		pMessage->level = (PM_DIAGNOSTIC_LEVEL)e.level_;
		pMessage->system = (PM_DIAGNOSTIC_SUBSYSTEM)e.subsystem_;
		pMessage->messageBuffer_ = str::ToNarrow(e.note_);
		pMessage->pText = pMessage->messageBuffer_.c_str();
		// ensure that # of queued does not exceed max
		while (GetQueuedMessageCount() >= GetMaxQueuedMessages()) {
			std::unique_ptr<Message> p;
			if (messageQueue_.try_dequeue(p)) {
				discardedCount_++;
			}			
		}
		// enqueue the message
		messageQueue_.enqueue(std::move(pMessage));
		// signal message availability
		messageWaitEvent_.Set();
	}
	void DiagnosticDriver::Flush() {}
	uint32_t DiagnosticDriver::GetQueuedMessageCount()
	{
		return (uint32_t)messageQueue_.size_approx();
	}
	uint32_t DiagnosticDriver::GetMaxQueuedMessages()
	{
		return maxQueuedMessages_;
	}
	void DiagnosticDriver::SetMaxQueuedMessages(uint32_t max)
	{
		maxQueuedMessages_ = max;
	}
	uint32_t DiagnosticDriver::GetDiscardedMessageCount()
	{
		return discardedCount_;
	}
	std::unique_ptr<Message> DiagnosticDriver::DequeueMessage()
	{
		messageWaitEvent_.Reset();
		std::unique_ptr<Message> pMessage;
		messageQueue_.try_dequeue(pMessage);
		return pMessage;
	}
	PM_DIAGNOSTIC_WAKE_REASON DiagnosticDriver::WaitForMessage(uint32_t timeoutMs)
	{
		if (GetQueuedMessageCount() > 0) {
			return PM_DIAGNOSTIC_WAKE_REASON_MESSAGE_AVAILABLE;
		}
		const auto wakeSignal = timeoutMs ?
			win::WaitAnyEventFor(timeoutMs * 1ms, messageWaitEvent_, manualUnblockEvent_) :
			win::WaitAnyEvent(messageWaitEvent_, manualUnblockEvent_);
		if (!wakeSignal) {
			return PM_DIAGNOSTIC_WAKE_REASON_TIMEOUT;
		}
		if (*wakeSignal == 0) {
			return PM_DIAGNOSTIC_WAKE_REASON_MESSAGE_AVAILABLE;
		}
		if (*wakeSignal == 1) {
			return dying_ ?
				PM_DIAGNOSTIC_WAKE_REASON_SHUTDOWN :
				PM_DIAGNOSTIC_WAKE_REASON_MANUAL_UNBLOCK;
		}
		throw Except<Exception>("Bad event index waiting on message+unblock events");
	}
	void DiagnosticDriver::UnblockWaitingThread()
	{
		manualUnblockEvent_.Set();
	}
}
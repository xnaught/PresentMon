#include "NamedPipeMarshallSender.h"
#include <vector>
#include <atomic>
#include <thread>
#include <span>
#include <variant>
#include "../win/Handle.h"
#include "../win/Event.h"
#include "../win/Overlapped.h"
#include <sstream>
#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>
#include <concurrentqueue/concurrentqueue.h>

namespace pmon::util::log
{
	namespace vi = std::views;

	// how to support per-pipe instance configuration of a filter
	class NamedPipeInstance
	{
	private:
		class TransmitHeaderStep
		{
		public:
			TransmitHeaderStep(std::span<const std::byte> data, win::Event* pDecomEvt)
				:
				sizeField_{ (DWORD)data.size() },
				pDecommissionEvent_{ pDecomEvt }
			{}
			// return true if caller needs to wait on overlapped, false if sequence is terminated
			bool Execute(NamedPipeInstance& inst)
			{
				try {
					std::span sizeBytes{ reinterpret_cast<const std::byte*>(&sizeField_), sizeof(sizeField_) };
					if (!initiated_) {
						inst.InitiateByteTransmission_(sizeBytes);
						initiated_ = true;
						return true;
					}
					else {
						inst.ResolveOverlapped_((DWORD)sizeBytes.size());
						inst.AdvanceStep_();
						return inst.ExecuteCurrentStep();
					}
				}
				catch (...) {
					inst.Decommision_();
					pDecommissionEvent_->Set();
					return false;
				}
			}
		private:
			DWORD sizeField_;
			win::Event* pDecommissionEvent_;
			bool initiated_ = false;
		};
		class TransmitPayloadStep
		{
		public:
			TransmitPayloadStep(std::span<const std::byte> data, win::Event* pDecomEvt)
				:
				data_{ std::move(data) },
				pDecommissionEvent_{ pDecomEvt }
			{}
			// return true if caller needs to wait on overlapped, false if sequence is terminated
			bool Execute(NamedPipeInstance& inst)
			{
				try {
					if (!initiated_) {
						inst.InitiateByteTransmission_(data_);
						initiated_ = true;
						return true;
					}
					else {
						inst.ResolveOverlapped_((DWORD)data_.size());
						inst.AdvanceStep_();
						return inst.ExecuteCurrentStep();
					}
				}
				catch (...) {
					inst.Decommision_();
					pDecommissionEvent_->Set();
					return false;
				}
			}
		private:
			std::span<const std::byte> data_;
			win::Event* pDecommissionEvent_;
			bool initiated_ = false;
		};
		class ConnectStep
		{
		public:
			// return true if caller needs to wait on overlapped, false if sequence is terminated
			bool Execute(NamedPipeInstance& inst)
			{
				if (!initiated_) {
 					const auto result = ConnectNamedPipe(inst.pipeHandle_, inst.overlapped_);
					initiated_ = true;
					if (result) {
						// this is not expected, but if it happens let's interpret as "already connected"
						// fall through down to finalization routine
					}
					if (const auto error = GetLastError(); error == ERROR_IO_PENDING) {
						// overlapped operation initiated and waiting for connection
						return true;
					}
					// TODO: handle pulsed client connect between create and connect
					else if (error != ERROR_PIPE_CONNECTED) {
						// any error other than already connected pseudo-error signal
						throw std::runtime_error{ "failed connecting named pipe" };
					}
					// if we reach here, pipe has already connected => fall through to finalization
				}
				// finalization routine, switch over to active mode
				inst.ResolveOverlapped_(std::nullopt);
				inst.state_ = State::Active;
				inst.AdvanceStep_();
				return inst.ExecuteCurrentStep();
			}
		private:
			bool initiated_ = false;
		};
		using Step = std::variant<TransmitHeaderStep, TransmitPayloadStep, ConnectStep>;
	public:
		// types
		enum class State
		{
			OutOfCommission,
			Connecting,
			Active,
		};
		// functions
		NamedPipeInstance(const std::wstring& address, size_t nInstances)
		{
			pipeHandle_ = (win::Handle)CreateNamedPipeW(
				address.c_str(),
				PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, // open mode
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, // pipe mode
				(DWORD)nInstances, // max instances
				4096,		// out buffer
				4096,		// in buffer
				0,			// timeout
				nullptr);	// security
			if (pipeHandle_ == INVALID_HANDLE_VALUE) {
				throw std::runtime_error("Failed to create named pipe instance for sending");
			}
		}
		bool ExecuteCurrentStep()
		{
			if (!steps_.empty()) {
				return std::visit([this](auto& step) { return step.Execute(*this); }, steps_.front());
			}
			return false;
		}
		win::Handle::HandleType GetOverlappedEventHandle()
		{
			return overlapped_.GetEvent();
		}
		void SetTransmissionSequence(std::span<const std::byte> data, win::Event& decomEvt)
		{
			steps_.push_back(TransmitHeaderStep{ data, &decomEvt });
			steps_.push_back(TransmitPayloadStep{ data, &decomEvt });
		}
		void SetConnectionSequence()
		{
			steps_.push_back(ConnectStep{});
		}
		bool IsActive() const
		{
			return state_.load() == State::Active;
		}
		bool NeedsConnection() const
		{
			return state_.load() == State::OutOfCommission;
		}
	private:
		// functions
		void InitiateByteTransmission_(std::span<const std::byte> data)
		{
			if (BOOL result = WriteFile(pipeHandle_, data.data(), (DWORD)data.size(), nullptr, overlapped_);
				!result && GetLastError() != ERROR_IO_PENDING) {
				throw std::runtime_error{ "Failure initiating transmission" };
			}
		}
		void ResolveOverlapped_(std::optional<DWORD> expectedBytesTransferred)
		{
			DWORD transferred = 0;
			if (!GetOverlappedResult(pipeHandle_, overlapped_, &transferred, FALSE)) {
				throw std::runtime_error{ "Failure resolving pipe operation" };
			}
			if (expectedBytesTransferred && transferred != *expectedBytesTransferred) {
				throw std::runtime_error{ "Failure resolving pipe operation, byte count does not match expected" };
			}
			overlapped_.ResetOverlapped();
		}
		void AdvanceStep_()
		{
			if (!steps_.empty()) {
				steps_.pop_front();
			}
		}
		void Decommision_()
		{
			// TODO: check this result and panic
			// or alternatively, dispose handle completely on this error
			DisconnectNamedPipe(pipeHandle_);
			overlapped_.ResetOverlapped();
			state_ = State::OutOfCommission;
		}
		// data
		win::Handle pipeHandle_;
		win::Overlapped overlapped_;
		std::deque<Step> steps_;
		std::atomic<State> state_ = State::OutOfCommission;
	};

	class ScheduledActions
	{
	public:
		template<std::same_as<win::Event>...E>
		ScheduledActions(const E&...interruptEvents)
			:
			nInterrupts_{ (uint32_t)sizeof...(interruptEvents) }
		{
			win::Event::HandleType handles[] = { interruptEvents... };
			// interrupt events go first in the array of wait events
			overlappedEvents_.append_range(handles);
		}
		ScheduledActions(std::span<win::Event::HandleType> interruptEvents)
			:
			nInterrupts_{ (uint32_t)interruptEvents.size() }
		{
			// interrupt events go first in the array of wait events
			overlappedEvents_.append_range(interruptEvents);
		}
		void Push(NamedPipeInstance* pInst)
		{
			if (pInst->ExecuteCurrentStep()) {
				instances_.push_back(pInst);
				overlappedEvents_.push_back(pInst->GetOverlappedEventHandle());
			}
		}
		// returns interrupt index if unblocking due to interrupt signal
		std::optional<uint32_t> RunAll()
		{
			while (!Complete()) {
				if (auto interrupt = RunOnce()) {
					return interrupt;
				}
			}
			return {};
		}
		// returns interrupt index if unblocking due to interrupt signal
		std::optional<uint32_t> RunOnce()
		{
			const auto nEvents = (DWORD)overlappedEvents_.size();
			const auto status = WaitForMultipleObjects(
				nEvents, overlappedEvents_.data(), FALSE, INFINITE);
			const DWORD eventIndex = status - WAIT_OBJECT_0;
			// check for interrupt events (first n handles)
			if (eventIndex < nInterrupts_) {
				return eventIndex;
			}
			if (eventIndex < nEvents) {
				// account for the exit event being element 0
				const int instanceIndex = eventIndex - nInterrupts_;
				// check for sequence completion
				if (!instances_[instanceIndex]->ExecuteCurrentStep()) {
					Remove_(instanceIndex);
				}
			}
			else {
				// failed, bailed
				auto hr = GetLastError();
				throw std::runtime_error{ "Failed waiting on multiple objects" };
			}
			return {};
		}
		bool Complete() const
		{
			return instances_.empty();
		}
	private:
		// functions
		void Remove_(size_t index)
		{
			instances_.erase(instances_.begin() + index);
			// account for the exit event being element 0
			overlappedEvents_.erase(overlappedEvents_.begin() + index + nInterrupts_);
		}
		// data
		std::vector<NamedPipeInstance*> instances_;
		std::vector<win::Handle::HandleType> overlappedEvents_;
		uint32_t nInterrupts_ = 0;
	};

	class NamedPipe
	{
	public:
		NamedPipe(const std::wstring& pipeSuffix, size_t nInstances)
			:
			pipeAddress_{ LR"(\\.\pipe\)" + pipeSuffix }
		{
			instances_.reserve(nInstances);
			for (size_t i = 0; i < nInstances; i++) {
				instances_.push_back(std::make_unique<NamedPipeInstance>(pipeAddress_, nInstances));
			}
			transmissionThread_ = std::jthread{ &NamedPipe::TransmissionThreadProcedure_, this };
			connectionThread_ = std::jthread{ &NamedPipe::ConnectionThreadProcedure_, this };
		}
		~NamedPipe()
		{
			using namespace std::chrono_literals;
			try { win::WaitAnyEventFor(50ms, emptyEvent_); }
			catch (...) {}
			try { exitEvent_.Set(); }
			catch (...) {}
		}
		void Send(const Entry& entry)
		{
			emptyEvent_.ResetEvent();
			entryQueue_.enqueue(entry);
			entryEvent_.Set();
		}
	private:
		// function
		void ConnectionThreadProcedure_()
		{
			SetThreadDescription(GetCurrentThread(), L"MarshallSender-Conn");
			ScheduledActions actions_{ exitEvent_, decommissionEvent_ };
			while (true) {
				// for all inactive pipe connections, set connection sequence and add to action list
				for (auto& pInst : instances_ | vi::filter(&NamedPipeInstance::NeedsConnection)) {
					pInst->SetConnectionSequence();
					actions_.Push(pInst.get());
				}
				// loop as long as not interrupted
				while (true) {
					if (auto interrupt = actions_.RunOnce()) {
						// exit signalled
						if (*interrupt == 0) {
							return;
						}
						// else decommission signalled
						else {
							decommissionEvent_.ResetEvent();
							break;
						}
					}
				}
			}
		}
		void TransmissionThreadProcedure_()
		{
			SetThreadDescription(GetCurrentThread(), L"MarshallSender-Tx");
			ScheduledActions actions_{ exitEvent_ };
			Entry entry;
			while (true) {
				// wait for either an Entry enqueue event or exit signal
				if (const auto eventId = *win::WaitAnyEvent(exitEvent_, entryEvent_);
					eventId == 0) {
					break;
				}

				// reset the event
				entryEvent_.ResetEvent();

				// dequeue any entries
				while (entryQueue_.try_dequeue(entry)) {
					// serialize the entry
					std::ostringstream stream;
					cereal::BinaryOutputArchive archive(stream);
					archive(entry);
					const auto bufferView = stream.rdbuf()->view();
					const std::span bufferSpan{ reinterpret_cast<const std::byte*>(bufferView.data()), bufferView.size() };

					// for all active pipe connections, set transmission sequence and add to action list
					for (auto& pInst : instances_ | vi::filter(&NamedPipeInstance::IsActive)) {
						pInst->SetTransmissionSequence(bufferSpan, decommissionEvent_);
						actions_.Push(pInst.get());
					}

					// process all sequences until complete or exit signal set
					if (actions_.RunAll()) {
						break;
					}
				}

				// signal that exiting is possible
				emptyEvent_.Set();
			}
		}
		// data
		std::wstring pipeAddress_;
		win::Event exitEvent_;
		win::Event decommissionEvent_;
		win::Event entryEvent_;
		win::Event emptyEvent_{ true, true };
		std::vector<std::unique_ptr<NamedPipeInstance>> instances_;
		moodycamel::ConcurrentQueue<Entry> entryQueue_;
		std::jthread connectionThread_;
		std::jthread transmissionThread_;
	};

    NamedPipeMarshallSender::NamedPipeMarshallSender(const std::wstring& pipeName)
		:
		pNamedPipe_{ std::make_shared<NamedPipe>(pipeName, 12) }
	{}

    NamedPipeMarshallSender::~NamedPipeMarshallSender() = default;

    void NamedPipeMarshallSender::Push(const Entry& entry)
    {
		std::static_pointer_cast<NamedPipe>(pNamedPipe_)->Send(entry);
    }
}
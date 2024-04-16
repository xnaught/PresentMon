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
#include "MarshallingProtocol.h"

namespace pmon::util::log
{
	namespace vi = std::views;

	// how to support per-pipe instance configuration of a filter
	class NamedPipeInstance
	{
	public: // types
		enum class StepResult // this could be completely replaced using just the State below
		{
			Completed,
			RequiresAwaiting,
			RequiresDisposal,
		};
		enum class State
		{
			OutOfCommission,
			Connecting,
			Active,
		};
	private: // Steps
		class TransmitHeaderStep
		{
		public:
			TransmitHeaderStep(std::span<const std::byte> data)
				:
				sizeField_{ (DWORD)data.size() }
			{}
			// return needs to signal whether a: sequence complete b: overlapped pending c: recycle pipe [exception]
			StepResult Execute(NamedPipeInstance& inst)
			{
				std::span sizeBytes{ reinterpret_cast<const std::byte*>(&sizeField_), sizeof(sizeField_) };
				if (!initiated_) {
					inst.InitiateByteTransmission_(sizeBytes);
					initiated_ = true;
					return StepResult::RequiresAwaiting;
				}
				else {
					inst.ResolveOverlapped_((DWORD)sizeBytes.size());
					return StepResult::Completed;
				}
			}
		private:
			DWORD sizeField_;
			bool initiated_ = false;
		};
		class TransmitPayloadStep
		{
		public:
			TransmitPayloadStep(std::span<const std::byte> data)
				:
				data_{ std::move(data) }
			{}
			// return needs to signal whether a: sequence complete b: overlapped pending c: recycle pipe [exception]
			StepResult Execute(NamedPipeInstance& inst)
			{
				if (!initiated_) {
					inst.InitiateByteTransmission_(data_);
					initiated_ = true;
					return StepResult::RequiresAwaiting;
				}
				else {
					inst.ResolveOverlapped_((DWORD)data_.size());
					return StepResult::Completed;
				}
			}
		private:
			std::span<const std::byte> data_;
			bool initiated_ = false;
		};
		class ConnectStep
		{
		public:
			// return needs to signal whether a: sequence complete b: overlapped pending c: recycle pipe [exception]
			StepResult Execute(NamedPipeInstance& inst)
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
						inst.state_ = State::Connecting;
						return StepResult::RequiresAwaiting;
					}
					// TODO: consider distinguishing between errors where pipe is recycled, and quarantine errors
					else if (error != ERROR_PIPE_CONNECTED) {
						// any error other than already connected pseudo-error signal
						// this will cause the instance to be recycled
						throw std::runtime_error{ "failed connecting named pipe" };
					}
					// if we reach here, pipe has already connected => fall through to finalization
				}
				// finalization routine, switch over to active mode
				inst.ResolveOverlapped_(std::nullopt);
				inst.state_ = State::Active;
				return StepResult::Completed;
			}
		private:
			bool initiated_ = false;
		};
		class IdTableBulkStep
		{
		public:
			IdTableBulkStep()
				:
				data_{ MakeDataBytes_() },
				headerStep_{ data_ },
				payloadStep_{ data_ }
			{}
			// return needs to signal whether a: sequence complete b: overlapped pending c: recycle pipe [exception]
			StepResult Execute(NamedPipeInstance& inst)
			{
				if (headerStepActive_) {
					if (auto result = headerStep_.Execute(inst); result != StepResult::Completed) {
						return result;
					}
					headerStepActive_ = false;
				}
				return payloadStep_.Execute(inst);
			}
		private:
			// functions
			static std::vector<std::byte> MakeDataBytes_()
			{
				MarshallPacket packet = IdentificationTable::GetBulk();
				std::ostringstream stream;
				cereal::BinaryOutputArchive archive(stream);
				archive(packet);
				const auto bufferView = stream.rdbuf()->view();
				const std::span bufferSpan{ reinterpret_cast<const std::byte*>(bufferView.data()), bufferView.size() };
				return { std::from_range, bufferSpan };
			}
			// data
			std::vector<std::byte> data_;
			TransmitHeaderStep headerStep_;
			TransmitPayloadStep payloadStep_;
			bool headerStepActive_ = true;
		};
		using Step = std::variant<TransmitHeaderStep, TransmitPayloadStep, ConnectStep, IdTableBulkStep>;
	public:
		// functions
		NamedPipeInstance(const std::wstring& address, size_t nInstances, win::Event& decomissionEvent)
			:
			decomissionEvent_{ decomissionEvent }
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
		StepResult ExecuteCurrentStep() noexcept
		{
			while (!steps_.empty()) {
				try {
					auto result = std::visit([this](auto& step) { return step.Execute(*this); }, steps_.front());
					if (result == StepResult::Completed) {
						AdvanceStep_();
						continue;
					}
					return result;
				}
				catch (...) {
					return StepResult::RequiresDisposal;
				}
			}
			return StepResult::Completed;
		}
		win::Handle::HandleType GetOverlappedEventHandle()
		{
			return overlapped_.GetEvent();
		}
		void SetTransmissionSequence(std::span<const std::byte> data)
		{
			steps_.push_back(TransmitHeaderStep{ data });
			steps_.push_back(TransmitPayloadStep{ data });
		}
		void SetConnectionSequence()
		{
			steps_.push_back(ConnectStep{});
			steps_.push_back(IdTableBulkStep{});
		}
		bool IsActive() const
		{
			return state_.load() == State::Active;
		}
		bool NeedsConnection() const
		{
			return state_.load() == State::OutOfCommission;
		}
		void Decommision()
		{
			// TODO: check this result and panic
			// or alternatively, dispose handle completely on this error
			CancelIo(pipeHandle_);
			DisconnectNamedPipe(pipeHandle_);
			steps_.clear();
			overlapped_.Reset();
			state_ = State::OutOfCommission;
			decomissionEvent_.Set();
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
			overlapped_.Reset();
		}
		void AdvanceStep_()
		{
			if (!steps_.empty()) {
				steps_.pop_front();
			}
		}
		// data
		win::Handle pipeHandle_;
		win::Overlapped overlapped_;
		std::deque<Step> steps_;
		std::atomic<State> state_ = State::OutOfCommission;
		win::Event& decomissionEvent_;
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
			// only overlapped events requiring await should be placed in action container
			// do first execution to see if insertion is necessary, or if complete synchronously
			auto result = pInst->ExecuteCurrentStep();
			if (result == NamedPipeInstance::StepResult::RequiresAwaiting) {
				instances_.push_back(pInst);
				overlappedEvents_.push_back(pInst->GetOverlappedEventHandle());
			}
			else if (result == NamedPipeInstance::StepResult::RequiresDisposal) {
				pInst->Decommision();
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
				const auto pInst = instances_[instanceIndex];
				// check for sequence completion
				switch (auto result = pInst->ExecuteCurrentStep()) {
				case NamedPipeInstance::StepResult::Completed:
					Remove_(instanceIndex);
					break;
				case NamedPipeInstance::StepResult::RequiresAwaiting:
					// default case, keep this instance in the actions container
					break;
				case NamedPipeInstance::StepResult::RequiresDisposal:
					pInst->Decommision();
					break;
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
				instances_.push_back(std::make_unique<NamedPipeInstance>(pipeAddress_, nInstances, decommissionEvent_));
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
			emptyEvent_.Reset();
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
							decommissionEvent_.Reset();
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
			MarshallPacket packet = Entry{};
			auto& entry = std::get<Entry>(packet);
			while (true) {
				// wait for either an Entry enqueue event or exit signal
				if (const auto eventId = *win::WaitAnyEvent(exitEvent_, entryEvent_);
					eventId == 0) {
					break;
				}

				// reset the event
				entryEvent_.Reset();

				// dequeue any entries
				while (entryQueue_.try_dequeue(entry)) {
					// serialize the entry
					std::ostringstream stream;
					cereal::BinaryOutputArchive archive(stream);
					archive(packet);
					const auto bufferView = stream.rdbuf()->view();
					const std::span bufferSpan{ reinterpret_cast<const std::byte*>(bufferView.data()), bufferView.size() };

					// for all active pipe connections, set transmission sequence and add to action list
					for (auto& pInst : instances_ | vi::filter(&NamedPipeInstance::IsActive)) {
						pInst->SetTransmissionSequence(bufferSpan);
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
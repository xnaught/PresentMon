#include "NamedPipeMarshallSender.h"
#include <vector>
#include <atomic>
#include <semaphore>
#include <span>
#include <variant>
#include <unordered_set>
#include "../mt/Thread.h"
#include "../win/Event.h"
#include <sstream>
#include <ranges>
#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>
#include <concurrentqueue/concurrentqueue.h>
#include "MarshallingProtocol.h"
#include "../Exception.h"
#include "PanicLogger.h"
#include "../pipe/Pipe.h"

namespace pmon::util::log
{
	namespace vi = std::views;

	class NamedPipe
	{
	public:
		NamedPipe(const std::string& pipeSuffix)
			:
			pipeAddress_{ R"(\\.\pipe\)" + pipeSuffix },
			entryEvent_{ ioctx_, win::Event{ false }.Release() }
		{}
		~NamedPipe()
		{
			using namespace std::chrono_literals;
			deactivated_ = true;
			// give 50ms max grace period to finish queued work
			pmquell(win::WaitAnyEventFor(50ms, emptyEvent_));
			pmquell(ioctx_.stop());
		}
		template<class T>
		void Send(T&& packetable)
		{
			if (!deactivated_) {
				emptyEvent_.Reset();
				entryQueue_.enqueue(std::forward<T>(packetable));
				if (!SetEvent(entryEvent_.native_handle())) {
					pmlog_error("failed setting packet queue event").raise<Exception>();
				}
			}
		}
		bool WaitForConnection(std::chrono::duration<float> timeout)
		{
			if (timeout.count() > 0.f) {
				return connectionSema_.try_acquire_for(timeout);
			}
			else {
				connectionSema_.acquire();
				return true;
			}
		}
		void Run()
		{
			try {
				pipe::as::co_spawn(ioctx_, ConnectionAcceptor_(), pipe::as::detached);
				pipe::as::co_spawn(ioctx_, EventHandler_(), pipe::as::detached);
				ioctx_.run();
			}
			catch (...) {
				deactivated_ = true;
				pmlog_error(ReportException());
			}
		}
	private:
		// function
		pipe::as::awaitable<void> EventHandler_()
		{
			try {
				MarshallPacket packet = Entry{};
				while (true) {
					co_await entryEvent_.async_wait(pipe::as::use_awaitable);
					while (entryQueue_.try_dequeue(packet)) {
						// TODO: add ability to pre-serialize and re-use buffer
						// for all active pipe connections, set transmission sequence and add to action list
						for (auto pPipe : pipePtrs_) {
							try {
								co_await pPipe->WritePacket(NamedPipeMarshallSender::EmptyHeader{}, packet);
							}
							catch (const pipe::PipeBroken&) {
								pmlog_dbg("Pipe disconnected by client");
								pipePtrs_.erase(pPipe);
							}
							catch (...) {
								pmlog_error(ReportException());
								pipePtrs_.erase(pPipe);
							}
						}
					}
					emptyEvent_.Set();
				}
			}
			catch (...) {
				deactivated_ = true;
				pmlog_error(ReportException());
			}
		}
		pipe::as::awaitable<void> ConnectionAcceptor_()
		{
			try {
				// create pipe instance
				std::shared_ptr pPipe = pipe::DuplexPipe::MakeAsPtr(pipeAddress_, ioctx_);
				// wait for a client to connect
				co_await pPipe->Accept();
				connectionSema_.release(16);
				// fork to replace this instance with a new  acceptor to take its place
				pipe::as::co_spawn(ioctx_, ConnectionAcceptor_(), pipe::as::detached);
				// transmit bulk context data
				MarshallPacket packet = IdentificationTable::GetBulk();
				co_await pPipe->WritePacket(NamedPipeMarshallSender::EmptyHeader{}, packet);
				// add pipe to the set of pipes available to broadcast to
				pipePtrs_.insert(pPipe);
			}
			catch (...) {
				pmlog_error("Log sender pipe failed in accepting");
			}
		}
		// data
		std::string pipeAddress_;
		pipe::as::io_context ioctx_;
		std::counting_semaphore<> connectionSema_{ 0 };
		std::atomic<bool> deactivated_ = false;
		moodycamel::ConcurrentQueue<MarshallPacket> entryQueue_;
		std::unordered_set<std::shared_ptr<pipe::DuplexPipe>> pipePtrs_;
		pipe::as::windows::object_handle entryEvent_;
		win::Event emptyEvent_{ false };
	};

    NamedPipeMarshallSender::NamedPipeMarshallSender(const std::string& pipeName, size_t nInstances)
	{
		// there are issues with calling .stop() and subsequently destroying ioctx
		// for now we will detach the thread and allow it to run independently until process exit,
		// owning NamedPipe in the detached thread so that ioctx dtor is not called
		// 
		mt::Thread{ "log-snd", [&, this] {
			pNamedPipe_ = std::make_shared<NamedPipe>(pipeName);
			constructionSema_.release();
			std::static_pointer_cast<NamedPipe>(pNamedPipe_)->Run();
		} }.detach();
		constructionSema_.acquire();
	}

    NamedPipeMarshallSender::~NamedPipeMarshallSender() = default;

    void NamedPipeMarshallSender::Push(const Entry& entry)
    {
		std::static_pointer_cast<NamedPipe>(pNamedPipe_)->Send(entry);
    }
	void NamedPipeMarshallSender::AddThread(uint32_t tid, uint32_t pid, std::string name)
	{
		std::static_pointer_cast<NamedPipe>(pNamedPipe_)->Send(
			IdentificationTable::Bulk{ .threads = { { tid, pid, std::move(name) } } }
		);
	}
	void NamedPipeMarshallSender::AddProcess(uint32_t pid, std::string name)
	{
		std::static_pointer_cast<NamedPipe>(pNamedPipe_)->Send(
			IdentificationTable::Bulk{ .processes = { { pid, std::move(name) } } }
		);
	}
	bool NamedPipeMarshallSender::WaitForConnection(std::chrono::duration<float> timeout) noexcept
	{
		try {
			return std::static_pointer_cast<NamedPipe>(pNamedPipe_)->WaitForConnection(timeout);
		}
		catch (...) {
			pmlog_panic_("failed to wait for connection");
			return false;
		}
	}

}
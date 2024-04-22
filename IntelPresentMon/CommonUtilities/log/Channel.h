#pragma once
#include "IChannel.h"
#include <span>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace pmon::util::log
{
	namespace
	{
		// internal channel implementation, has functions that are called only from the worker thread
		// in response to entries / packets placed in the queue
		class ChannelInternal_
		{
			friend struct QueueAccessor_;
		public:
			~ChannelInternal_();
			void Flush();
			void SignalExit();
			void DisableTraceResolution();
			void AttachDriver(std::shared_ptr<IDriver>);
			void AttachPolicy(std::shared_ptr<IPolicy>);
			void EnqueueEntry(Entry&&);
			template<class P, typename...Args>
			void EnqueuePacketWait(Args&&...args);
			template<class P, typename...Args>
			void EnqueuePacketAsync(Args&&...args);
		protected:
			ChannelInternal_(std::vector<std::shared_ptr<IDriver>> driverPtrs);
		private:
			bool resolvingTraces_ = true;
			bool exiting_ = false;
			std::vector<std::shared_ptr<IDriver>> driverPtrs_;
			std::vector<std::shared_ptr<IPolicy>> policyPtrs_;
			std::shared_ptr<void> pEntryQueue_;
			std::jthread worker_;
		};
	}

	// Channel: standard free-threaded lockfree-queued implementation of IChannel
	// external channel implementation, has functions that are only called from outside of the channel
	// worker thread and which only place entries / packets in the queue
	class Channel : public IChannel, private ChannelInternal_
	{
	public:
		Channel(std::vector<std::shared_ptr<IDriver>> driverPtrs = {});
		~Channel();
		void Submit(Entry&&) noexcept override;
		void Flush() override;
		void AttachDriver(std::shared_ptr<IDriver>) override;
		void AttachPolicy(std::shared_ptr<IPolicy>) override;
		void FlushEntryPointExit() override;
	};
}
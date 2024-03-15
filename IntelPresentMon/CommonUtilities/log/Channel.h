#pragma once
#include "IChannel.h"
#include <span>
#include <vector>
#include <memory>

namespace pmon::util::log
{
	class Channel : public IChannel
	{
	public:
		Channel(std::vector<std::shared_ptr<IDriver>> driverPtrs = {});
		~Channel();
		void Submit(Entry&) override;
		void Flush() override;
		void AttachDriver(std::shared_ptr<IDriver>) override;
		void AttachPolicy(std::shared_ptr<IPolicy>) override;
	private:
		std::vector<std::shared_ptr<IDriver>> driverPtrs_;
		std::vector<std::shared_ptr<IPolicy>> policyPtrs_;
	};
}
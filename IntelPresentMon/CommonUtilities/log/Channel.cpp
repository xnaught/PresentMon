#include "Channel.h"
#include "IPolicy.h"
#include "IDriver.h"

namespace pmon::util::log
{
	Channel::Channel(std::vector<std::shared_ptr<IDriver>> driverPtrs)
		:
		driverPtrs_{ std::move(driverPtrs) }
	{}
	Channel::~Channel()
	{}
	void Channel::Submit(Entry& e)
	{
		// process all policies, tranforming entry in-place
		for (auto& pPolicy : policyPtrs_) {
			// if any policy returns false, drop entry
			if (!pPolicy->TransformFilter(e)) {
				return;
			}
		}
		// submit entry to all drivers (by copy)
		for (auto& pDriver : driverPtrs_) {
			pDriver->Submit(e);
		}
		// TODO: log case when there are no drivers?
	}
	void Channel::Flush()
	{
		for (auto& pDriver : driverPtrs_) {
			pDriver->Flush();
		}
	}
	void Channel::AttachDriver(std::shared_ptr<IDriver> pDriver)
	{
		driverPtrs_.push_back(std::move(pDriver));
	}
	void Channel::AttachPolicy(std::shared_ptr<IPolicy> pPolicy)
	{
		policyPtrs_.push_back(std::move(pPolicy));
	}
}
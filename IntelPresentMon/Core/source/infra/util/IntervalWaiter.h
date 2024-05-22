// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "ChiliTimer.h"
#include <chrono>

namespace p2c::infra::util
{
	class IntervalWaiter
	{
	public:
		IntervalWaiter(double intervalSeconds);
		IntervalWaiter(const IntervalWaiter&) = delete;
		IntervalWaiter& operator=(const IntervalWaiter&) = delete;
		void SetInterval(double intervalSeconds);
		void SetInterval(std::chrono::nanoseconds interval);
		void Wait();
		~IntervalWaiter();
	private:
		static constexpr double waitBuffer_ = 0.000'25;
		ChiliTimer timer_;
		double intervalSeconds_;
		double lastTargetTime_ = 0.;
		void* waitableTimerHandle_ = nullptr;
	};
}
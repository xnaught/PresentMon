// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace pmon::util
{
	uint64_t GetCurrentTimestamp() noexcept;
	double GetTimestampPeriodSeconds() noexcept;
	void SpinWaitUntilTimestamp(uint64_t timestamp) noexcept;
	double TimestampDeltaToSeconds(uint64_t start, uint64_t end, double period) noexcept;

	class QpcTimer
	{
	public:
		QpcTimer() noexcept;
		double Mark() noexcept;
		double Peek() const noexcept;
		uint64_t GetStartTimestamp() const noexcept;
		void SpinWaitUntil(double seconds) const noexcept;
	private:
		double performanceCounterPeriod_;
		uint64_t startTimestamp_ = 0;
	};
}
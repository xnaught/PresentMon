#include "IntervalWaiter.h"
#include <Core/source/win/WinAPI.h>
#include <Core/source/infra/Logging.h>
#include <CommonUtilities/Exception.h>
#include <thread>

namespace p2c::infra::util
{
	using namespace ::pmon::util;

	IntervalWaiter::IntervalWaiter(double intervalSeconds)
		:
		intervalSeconds_{ intervalSeconds }
	{
		waitableTimerHandle_ = CreateWaitableTimerExW(
			nullptr,
			nullptr,
			CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
			TIMER_ALL_ACCESS);
		if (!waitableTimerHandle_) {
			pmlog_error(L"Failed creating high resolution timer").hr();
			throw Except<Exception>();
		}
	}

	void IntervalWaiter::SetInterval(double intervalSeconds)
	{
		intervalSeconds_ = intervalSeconds;
	}

	void IntervalWaiter::SetInterval(std::chrono::nanoseconds interval)
	{
		intervalSeconds_ = double(interval.count()) / 1'000'000'000.;
	}

	void IntervalWaiter::Wait()
	{
		const auto waitTimeSeconds = [=, this] {
			const auto t = timer_.Peek();
			auto targetCandidate = lastTargetTime_ + intervalSeconds_;
			// if we are on-time
			if (t <= targetCandidate) {
				lastTargetTime_ = targetCandidate;
				return targetCandidate - t;
			}
			// if we are late, reset target to NOW and do not wait
			lastTargetTime_ = t;
			return 0.;
		}();

		if (waitTimeSeconds == 0.) return;

		// wait slightly (waitBuffer_ seconds) shorter than required to compensate for overwait error
		const LARGE_INTEGER waitTime100ns{
			.QuadPart = -LONGLONG(double(waitTimeSeconds - waitBuffer_) * 10'000'000.)
		};
		if (!SetWaitableTimerEx(
			waitableTimerHandle_,
			&waitTime100ns,
			0,
			nullptr,
			nullptr,
			nullptr,
			0
		)) {
			pmlog_error(L"Failed setting high resolution timer").hr();
			throw Except<Exception>();
		}
		if (auto code = WaitForSingleObject(waitableTimerHandle_, INFINITE)) {
			pmlog_error(std::format(L"Failed waiting on high resolution timer CODE:{:8x}", code)).hr();
			throw Except<Exception>();
		}
		// spin wait remaining time to obtain more accurate wait
		while (timer_.Peek() < lastTargetTime_) {
			std::this_thread::yield();
		}
	}

	IntervalWaiter::~IntervalWaiter()
	{
		if (!CloseHandle(waitableTimerHandle_)) {
			pmlog_warn(L"Failed closing high resolution timer").hr();
		}
	}
}

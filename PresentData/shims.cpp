#include "shims.h"

#define NOMINMAX
#include <windows.h>

#include <thread>

#pragma comment(lib, "tdh.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace shims;

size_t shims::HashCombine(size_t lhs, size_t rhs) noexcept
{
    constexpr size_t kHashConstant = 0x517c'c1b7'2722'0a95ull;
    return lhs ^ (rhs + kHashConstant + (lhs << 6) + (lhs >> 2));
}

// ::-------------------------------------------------------------------------::

int64_t shims::GetCurrentTimestamp() noexcept
{
    LARGE_INTEGER timestamp;
    QueryPerformanceCounter(&timestamp);
    return timestamp.QuadPart;
}

double shims::GetTimestampPeriodSeconds() noexcept
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return 1.0 / static_cast<double>(freq.QuadPart);
}

double shims::TimestampDeltaToSeconds(int64_t start, int64_t end, double period) noexcept
{
    return double(end - start) * period;
}

PrecisionWaiter::PrecisionWaiter(double defaultWaitBuffer)
    : defaultWaitBuffer_(defaultWaitBuffer)
{
    performanceCounterPeriod_ = GetTimestampPeriodSeconds();
    MarkTimer();

    auto hnd = CreateWaitableTimerExW(
        nullptr,
        nullptr,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
        TIMER_ALL_ACCESS
    );

    if (hnd)
    {
        timerHandle_ = hnd;
    }
}

PrecisionWaiter::~PrecisionWaiter()
{
    if (timerHandle_)
    {
        CloseHandle(timerHandle_);
    }
}

void PrecisionWaiter::Wait(double seconds) noexcept
{
    if (timerHandle_)
    {
        // mark the start time of the wait if we're doing buffer with precision spin
        if (defaultWaitBuffer_ > 0.)
        {
            MarkTimer();
        }
        // wait slightly (buffer seconds) shorter than required to compensate for overwait error
        const LARGE_INTEGER waitTime100ns{
            .QuadPart = -LONGLONG(double(seconds - defaultWaitBuffer_) * 10'000'000.)
        };
        // set the timer deadline
        SetWaitableTimerEx(
            timerHandle_,
            &waitTime100ns,
            0,
            nullptr,
            nullptr,
            nullptr,
            0
        );
        // wait on the timer
        WaitForSingleObject(timerHandle_, INFINITE);
        // spin wait for the remainder buffer time
        if (defaultWaitBuffer_ > 0.)
        {
            SpinWaitUntil(seconds);
        }
    }
    else
    {
        // fallback if we fail high performance timing
        Sleep(DWORD(seconds * 1000.f));
    }
}

double PrecisionWaiter::PeekTimer() const noexcept
{
    const auto newTimestamp = GetCurrentTimestamp();
    const auto delta = TimestampDeltaToSeconds(startTimestamp_, newTimestamp, performanceCounterPeriod_);
    return delta;
}

double PrecisionWaiter::MarkTimer() noexcept
{
    const auto newTimestamp = GetCurrentTimestamp();
    const auto delta = TimestampDeltaToSeconds(startTimestamp_, newTimestamp, performanceCounterPeriod_);
    startTimestamp_ = newTimestamp;
    return delta;
}

void PrecisionWaiter::SpinWaitUntil(double seconds) const noexcept
{
    while (PeekTimer() < seconds)
    {
        std::this_thread::yield();
    }
}

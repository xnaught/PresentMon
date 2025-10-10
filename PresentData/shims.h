#pragma once

#include <cstdint>
#include <utility>

// ::-- Log.h ----------------------------------------------------------------::
#define pmlog_dbg(m) (void)0
#define pmlog_debug(m) (void)0
#define pmlog_info(m) (void)0
#define pmlog_warn(m) (void)0
#define pmlog_error(m) (void)0

// ::-- Exception.h ----------------------------------------------------------::
template <class E, typename... R>
auto Except(R&&... args)
{
    E exception{std::forward<R>(args)...};
    return exception;
}

namespace shims {
// ::-- Meta.h ---------------------------------------------------------------::
template <typename T>
struct dependent_false : std::false_type
{
};

size_t HashCombine(size_t lhs, size_t rhs) noexcept;

template <typename T, typename S>
size_t DualHash(const T& t, const S& s) noexcept
{
    return HashCombine(std::hash<T>{}(t), std::hash<S>{}(s));
}

// trait to deduce/extract the signature details of a function by pointer
namespace impl {
    template <typename T>
    struct FunctionPtrTraitsImpl_;

    template <typename R, typename... Args>
    struct FunctionPtrTraitsImpl_<R(*)(Args...)>
    {
        using ReturnType = R;
        using ParameterTypes = std::tuple<Args...>;
        template <size_t N>
        using ParameterType = std::tuple_element_t<N, ParameterTypes>;
        static constexpr size_t ParameterCount = sizeof...(Args);
    };
}

template <typename T>
struct FunctionPtrTraits : impl::FunctionPtrTraitsImpl_<std::remove_cvref_t<T>>
{
};

// ::-- PrecisionWaiter.h/Qpc.h/Handle.h -------------------------------------::
int64_t GetCurrentTimestamp() noexcept;
double GetTimestampPeriodSeconds() noexcept;
double TimestampDeltaToSeconds(int64_t start, int64_t end, double period) noexcept;

class PrecisionWaiter
{
public:
    explicit PrecisionWaiter(double defaultWaitBuffer);
    ~PrecisionWaiter();

    void Wait(double seconds) noexcept;

private:
    double PeekTimer() const noexcept;
    double MarkTimer() noexcept;
    void SpinWaitUntil(double seconds) const noexcept;

    double defaultWaitBuffer_;
    double performanceCounterPeriod_;
    int64_t startTimestamp_ = 0;
    void* timerHandle_ = nullptr;
};
} // namespace shims

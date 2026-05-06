#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <type_traits>
#include <vector>
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
struct DependentFalseT : std::false_type {};

template<typename T>
inline constexpr bool DependentFalse = DependentFalseT<T>::value;

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

// ::-- SampleStatistic.h -------------------------------------::
template <typename Sample>
class SampleStatistics {
  static_assert(std::is_arithmetic_v<Sample>, "SampleStatistics requires arithmetic sample types.");

public:
  void AddSample(Sample sample) {
    samples_.push_back(sample);
    prepared_ = false;
  }

  void Reset() noexcept {
    samples_.clear();
    prepared_ = false;
  }

  void Prepare() { EnsurePrepared_(); }

  size_t GetSampleCount() const noexcept { return samples_.size(); }

  double GetMean() const {
    if (samples_.empty()) {
      return 0.0;
    }
    const auto sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
    return sum / static_cast<double>(samples_.size());
  }

  Sample GetPercentile(double percentile) const {
    if (samples_.empty()) {
      return Sample{};
    }
    EnsurePrepared_();
    percentile = std::clamp(percentile, 0.0, 1.0);
    if (samples_.size() == 1 || percentile == 0.0) {
      return samples_.front();
    }
    if (percentile == 1.0) {
      return samples_.back();
    }

    const auto scaledIndex =
        percentile * static_cast<double>(samples_.size() - 1);
    const auto lower = static_cast<size_t>(scaledIndex);
    const auto upper = (std::min)(lower + 1, samples_.size() - 1);
    const auto weight = scaledIndex - static_cast<double>(lower);
    const auto low = static_cast<double>(samples_[lower]);
    const auto high = static_cast<double>(samples_[upper]);

    return static_cast<Sample>(low + (high - low) * weight);
  }

private:
  void EnsurePrepared_() const {
    if (!prepared_) {
      std::sort(samples_.begin(), samples_.end());
      prepared_ = true;
    }
  }

  mutable std::vector<Sample> samples_;
  mutable bool prepared_ = false;
};

} // namespace shims

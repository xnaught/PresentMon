#pragma once
#include <atomic>
#include "Level.h"

namespace pmon::util::log
{
	enum class ExceptionTracePolicy
	{
		Default,
		OverrideOn,
		OverrideOff,
	};

	class GlobalPolicy
	{
	public:
		Level GetLogLevel() const noexcept;
		void SetLogLevel(Level level) noexcept;
		Level GetTraceLevel() const noexcept;
		void SetTraceLevel(Level level) noexcept;
		bool GetResolveTraceInClientThread() const noexcept;
		void SetResolveTraceInClientThread(bool setting) noexcept;
		ExceptionTracePolicy GetExceptionTrace() const noexcept;
		void SetExceptionTrace(ExceptionTracePolicy policy) noexcept;
		bool GetSehTracing() const noexcept;
		void SetSehTracing(bool on) noexcept;
		static GlobalPolicy& Get() noexcept;
	private:
		// functions
		GlobalPolicy() noexcept;
		// data
		std::atomic<Level> logLevel_;
		std::atomic<bool> resolveTraceInClientThread_ = false;
		std::atomic<Level> traceLevel_ = Level::Error;
		std::atomic<ExceptionTracePolicy> exceptionTracePolicy_ = ExceptionTracePolicy::Default;
		std::atomic<bool> sehTraceOn_ = false;
	};
}
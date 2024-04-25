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
		static Level GetLogLevel() noexcept;
		static void SetLogLevel(Level level) noexcept;
		static Level GetTraceLevel() noexcept;
		static void SetTraceLevel(Level level) noexcept;
		static bool GetResolveTraceInClientThread() noexcept;
		static void SetResolveTraceInClientThread(bool setting) noexcept;
		static ExceptionTracePolicy GetExceptionTrace() noexcept;
		static void SetExceptionTrace(ExceptionTracePolicy policy) noexcept;
		static bool TracingSehExceptions() noexcept;
		static void SetSehTracing(bool on) noexcept;
	private:
		// functions
		GlobalPolicy() noexcept;
		static GlobalPolicy& Get_() noexcept;
		// data
		std::atomic<Level> logLevel_;
		std::atomic<bool> resolveTraceInClientThread_ = false;
		std::atomic<Level> traceLevel_ = Level::Error;
		std::atomic<ExceptionTracePolicy> exceptionTracePolicy_ = ExceptionTracePolicy::Default;
		std::atomic<bool> sehTraceOn_ = false;
	};
}
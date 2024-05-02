#pragma once
#include <atomic>
#include "Level.h"

namespace pmon::util::log
{
	class GlobalPolicy
	{
	public:
		Level GetLogLevel() const noexcept;
		void SetLogLevel(Level level) noexcept;
		Level GetTraceLevel() const noexcept;
		void SetTraceLevel(Level level) noexcept;
		bool GetResolveTraceInClientThread() const noexcept;
		void SetResolveTraceInClientThread(bool setting) noexcept;
		bool GetExceptionTrace() const noexcept;
		void SetExceptionTrace(bool policy) noexcept;
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
		std::atomic<bool> exceptionTracePolicy_ = false;
		std::atomic<bool> sehTraceOn_ = false;
	};
}
#include "GlobalPolicy.h"

#ifndef NDEBUG
#define PMLOG_GPOL_DEFAULT_LEVEL Level::Info
#else
#define PMLOG_GPOL_DEFAULT_LEVEL Level::Error
#endif

namespace pmon::util::log
{
	GlobalPolicy::GlobalPolicy() noexcept
		:
		logLevel_{ PMLOG_GPOL_DEFAULT_LEVEL }
	{}
	GlobalPolicy& GlobalPolicy::Get() noexcept
	{
		// @SINGLETON
		static GlobalPolicy policy;
		return policy;
	}

	Level GlobalPolicy::GetLogLevel() const noexcept
	{
		return logLevel_;
	}
	void GlobalPolicy::SetLogLevel(Level level) noexcept
	{
		logLevel_ = level;
	}
	Level GlobalPolicy::GetTraceLevel() const noexcept
	{
		return traceLevel_;
	}
	void GlobalPolicy::SetTraceLevel(Level level) noexcept
	{
		traceLevel_ = level;
	}
	bool GlobalPolicy::GetResolveTraceInClientThread() const noexcept
	{
		return resolveTraceInClientThread_;
	}
	void GlobalPolicy::SetResolveTraceInClientThread(bool setting) noexcept
	{
		resolveTraceInClientThread_ = setting;
	}
	ExceptionTracePolicy GlobalPolicy::GetExceptionTrace() const noexcept
	{
		return exceptionTracePolicy_;
	}
	void GlobalPolicy::SetExceptionTrace(ExceptionTracePolicy policy) noexcept
	{
		exceptionTracePolicy_ = policy;
	}
	bool GlobalPolicy::GetSehTracing() const noexcept
	{
		return sehTraceOn_;
	}
	void GlobalPolicy::SetSehTracing(bool on) noexcept
	{
		sehTraceOn_ = on;
	}
}
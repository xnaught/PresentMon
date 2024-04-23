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
	GlobalPolicy& GlobalPolicy::Get_() noexcept
	{
		// @SINGLETON
		static GlobalPolicy policy;
		return policy;
	}

	Level GlobalPolicy::GetLogLevel() noexcept
	{
		return Get_().logLevel_;
	}
	void GlobalPolicy::SetLogLevel(Level level) noexcept
	{
		Get_().logLevel_ = level;
	}
	Level GlobalPolicy::GetTraceLevel() noexcept
	{
		return Get_().traceLevel_;
	}
	void GlobalPolicy::SetTraceLevel(Level level) noexcept
	{
		Get_().traceLevel_ = level;
	}
	bool GlobalPolicy::GetResolveTraceInClientThread() noexcept
	{
		return Get_().resolveTraceInClientThread_;
	}
	void GlobalPolicy::SetResolveTraceInClientThread(bool setting) noexcept
	{
		Get_().resolveTraceInClientThread_ = setting;
	}
	ExceptionTracePolicy GlobalPolicy::GetExceptionTracePolicy() noexcept
	{
		return Get_().exceptionTracePolicy_;
	}
	void GlobalPolicy::SetExceptionTracePolicy(ExceptionTracePolicy policy) noexcept
	{
		Get_().exceptionTracePolicy_ = policy;
	}
}
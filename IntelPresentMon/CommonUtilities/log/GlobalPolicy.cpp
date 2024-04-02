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
	Level GlobalPolicy::GetLogLevel_() const noexcept
	{
		return logLevel_.load();
	}
	void GlobalPolicy::SetLogLevel_(Level level) noexcept
	{
		logLevel_.store(level);
	}
	bool GlobalPolicy::GetResolveTraceInClientThread_() const noexcept
	{
		return resolveTraceInClientThread_.load();
	}
	void GlobalPolicy::SetResolveTraceInClientThread_(bool setting) noexcept
	{
		resolveTraceInClientThread_.store(setting);
	}

	GlobalPolicy& GlobalPolicy::Get_() noexcept
	{
		// @SINGLETON
		static GlobalPolicy policy;
		return policy;
	}

	Level GlobalPolicy::GetLogLevel() noexcept
	{
		return Get_().GetLogLevel_();
	}
	void GlobalPolicy::SetLogLevel(Level level) noexcept
	{
		Get_().SetLogLevel_(level);
	}
	bool GlobalPolicy::GetResolveTraceInClientThread() noexcept
	{
		return Get_().GetResolveTraceInClientThread_();
	}
	void GlobalPolicy::SetResolveTraceInClientThread(bool setting) noexcept
	{
		Get_().SetResolveTraceInClientThread_(setting);
	}
}
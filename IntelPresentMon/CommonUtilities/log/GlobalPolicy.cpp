#include "GlobalPolicy.h"

namespace pmon::util::log
{
	GlobalPolicy globalPolicy;

	GlobalPolicy::GlobalPolicy() noexcept
	{
		// default to info level in debug build, error level in release
#ifndef NDEBUG
		SetLogLevel(Level::Info);
#else
		SetLogLevel(Level::Error);
#endif
	}

	Level GlobalPolicy::GetLogLevel() const noexcept
	{
		return logLevel_.load();
	}

	void GlobalPolicy::SetLogLevel(Level level) noexcept
	{
		logLevel_.store(level);
	}
}
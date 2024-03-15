#pragma once
#include <string>

namespace pmon::util::log
{
	enum class Level
	{
		None,
		Fatal,
		Error,
		Warn,
		Info,
		Debug,
		Verbose,
	};

	std::wstring GetLevelName(Level) noexcept;
}

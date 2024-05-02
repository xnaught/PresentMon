#pragma once
#include <string>
#include <map>

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
	std::map<std::string, Level> GetLevelMapNarrow() noexcept;
}

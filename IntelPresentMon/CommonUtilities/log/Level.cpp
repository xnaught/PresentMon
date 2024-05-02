#include "Level.h"
#include "../str/String.h"

namespace pmon::util::log
{
	std::wstring GetLevelName(Level lv) noexcept
	{
		try {
			switch (lv) {
			case Level::Verbose: return L"Verbose";
			case Level::Debug: return L"Debug";
			case Level::Info: return L"Info";
			case Level::Warn: return L"Warning";
			case Level::Error: return L"Error";
			case Level::Fatal: return L"Fatal";
			}
		} catch (...) {}
		return L"Unknown";
	}

	std::map<std::string, Level> GetLevelMapNarrow() noexcept
	{
		using namespace pmon::util::str;
		std::map<std::string, Level> map;
		for (int n = (int)Level::Fatal; n <= (int)Level::Verbose; n++) {
			const auto lvl = Level(n);
			auto key = ToLower(ToNarrow(GetLevelName(lvl)));
			map[std::move(key)] = lvl;
		}
		return map;
	}
}
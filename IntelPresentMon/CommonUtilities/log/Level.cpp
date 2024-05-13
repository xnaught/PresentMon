#include "Level.h"
#include "../str/String.h"

namespace pmon::util::log
{
	std::wstring GetLevelName(Level lv) noexcept
	{
		try {
			switch (lv) {
			case Level::None: return L"None";
			case Level::Fatal: return L"Fatal";
			case Level::Error: return L"Error";
			case Level::Warning: return L"Warning";
			case Level::Info: return L"Info";
			case Level::Performance: return L"Performance";
			case Level::Debug: return L"Debug";
			case Level::Verbose: return L"Verbose";
			default: return L"Unknown";
			}
		} catch (...) {}
		return {};
	}

	std::map<std::string, Level> GetLevelMapNarrow() noexcept
	{
		using namespace pmon::util::str;
		std::map<std::string, Level> map;
		for (int n = (int)Level::Fatal; n <= (int)Level::Verbose; n++) {
			const auto lvl = Level(n);
			auto key = ToLower(ToNarrow(GetLevelName(lvl)));
			if (key != "unknown") {
				map[std::move(key)] = lvl;
			}
		}
		return map;
	}
}
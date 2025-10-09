#include "Verbose.h"
#include "../str/String.h"

namespace pmon::util::log
{
	std::string GetVerboseModuleName(V mod) noexcept
	{
		try {
			switch (mod) {
			case V::v8async: return "v8async";
			case V::procwatch: return "procwatch";
			case V::tele_gpu: return "tele_gpu";
			case V::core_metric: return "core_metric";
			case V::core_hotkey: return "core_hotkey";
			case V::core_window: return "core_window";
			case V::etwq: return "etwq";
			default: return "Unknown";
			}
		}
		catch (...) {}
		return {};
	}

	std::map<std::string, V> GetVerboseModuleMapNarrow() noexcept
	{
		using namespace pmon::util::str;
		std::map<std::string, V> map;
		for (int n = 0; n <= (int)V::Count; n++) {
			const auto lvl = V(n);
			auto key = ToLower(GetVerboseModuleName(lvl));
			if (key != "Unknown") {
				map[std::move(key)] = lvl;
			}
		}
		return map;
	}
}
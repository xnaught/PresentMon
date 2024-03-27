#include "Level.h"

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
}
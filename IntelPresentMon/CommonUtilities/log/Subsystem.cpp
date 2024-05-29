#include "Subsystem.h"
#include "../str/String.h"

namespace pmon::util::log
{
	std::wstring GetSubsystemName(Subsystem lv) noexcept
	{
		try {
			switch (lv) {
			case Subsystem::None: return L"None";
			case Subsystem::Middleware: return L"Middleware";
			case Subsystem::Server: return L"Server";
			case Subsystem::Wrapper: return L"Wrapper";
			case Subsystem::IntelPresentmon: return L"IntelPresentmon";
			default:
				if (int(lv) < int(Subsystem::User)) {
					return L"Unknown";
				}
				else {
					return L"User" + std::to_wstring(int(lv) - int(Subsystem::User));
				}
			}
		}
		catch (...) {}
		return {};
	}
}
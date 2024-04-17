#pragma once 
#include "Utilities.h"
#include "../log/Log.h"

namespace pmon::util::win
{
	std::wstring GetErrorDescription(HRESULT hr) noexcept
	{
		try {
			wchar_t* descriptionWinalloc = nullptr;
			const auto result = FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&descriptionWinalloc), 0, nullptr
			);

			std::wstring description;
			if (!result) {
				pmlog_warn(L"Failed formatting windows error");
			}
			else {
				description = descriptionWinalloc;
				if (LocalFree(descriptionWinalloc)) {
					pmlog_warn(L"Failed freeing memory for windows error formatting");
				}
				if (description.ends_with(L"\r\n")) {
					description.resize(description.size() - 2);
				}
			}
			return description;
		}
		catch (...) {
			pmlog_warn(L"Exception thrown in windows error GetErrorDescription");
			return {};
		}
	}
}
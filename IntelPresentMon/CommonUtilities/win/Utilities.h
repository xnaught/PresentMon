#pragma once
#include <string>
#include "WinAPI.h"

namespace pmon::util::win
{
	std::wstring GetErrorDescription(HRESULT hr) noexcept;
	std::wstring GetSEHSymbol(DWORD sehCode) noexcept;
}
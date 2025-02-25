#pragma once
#include <string>
#include <filesystem>
#include <optional>
#include "WinAPI.h"

namespace pmon::util::win
{
	// convert WinAPI HRESULT code to human-readable string
	std::string GetErrorDescription(HRESULT hr) noexcept;
	// convert Structured Exception Handling error code to human-readable string
	std::string GetSEHSymbol(DWORD sehCode) noexcept;
	// returns true of the named pipe is available for connection
	bool WaitForNamedPipe(const std::string& fullname, int timeoutMs);
	// gets the full path to an process's module
	std::filesystem::path GetExecutableModulePath();
}
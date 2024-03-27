#include "PanicLogger.h"
#include <fstream>
#include <iostream>
#include <array>
#include <filesystem>
#include "../win/WinAPI.h"

namespace pmon::util::log
{
	class PanicLogger_
	{
	public:
		PanicLogger_() noexcept
		{
			try {
				file_.open("pmlog-panic.txt", file_.out|file_.app);
			}
			catch (...) {
				LogLastChance_(L"Could not open pmlog-panic.txt in working directory, reverting to temp directory");
			}
			if (!file_) {
				try {
					file_.open(std::filesystem::temp_directory_path() / "pmlog-panic.txt", file_.out | file_.app);
				}
				catch (...) {
					LogLastChance_(L"Could not open pmlog-panic.txt in temp directory, file panic logging will not be used");
				}
			}
		}
		void Log(const std::wstring& msg) noexcept
		{
			LogWinDbg_(msg);
			LogStdErr_(msg);
			LogFile_(msg);
		}
	private:
		// functions
		void LogStdErr_(const std::wstring& msg) const noexcept
		{
			try {
				std::wcerr << L"[@Error|log_panic] " << msg << std::endl;
			}
			catch (...) {
				LogLastChance_(L"Failed panic logging to stderr");
			}
		}
		void LogWinDbg_(const std::wstring& msg) const noexcept
		{
			OutputDebugStringW(L"[@Error|log_panic] ");
			OutputDebugStringW(msg.c_str());
			OutputDebugStringW(L"\n");
		}
		void LogFile_(const std::wstring& msg) noexcept
		{
			if (file_) {
				try {
					file_ << L"[@Error|log_panic] " << msg << std::endl;
					if (!file_) {
						LogLastChance_(L"Error occurred when panic logging to file");
					}
				}
				catch (...) {
					LogLastChance_(L"Failed panic logging to file");
				}
			}
		}
		void LogLastChance_(const std::wstring& msg) const noexcept
		{
			LogWinDbg_(msg);
		}
		static std::array<wchar_t, 250> GetTimestamp_() noexcept;
		// data
		std::wofstream file_;
	};

	void PMLogPanic_(const std::wstring& msg) noexcept
	{
		static PanicLogger_ panic_;
		panic_.Log(msg);
	}
}
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
				file_.open("logger-panic.txt", file_.out|file_.app);
			}
			catch (...) {
				LogLastChance_(L"Could not open logger-panic.txt in working directory, reverting to temp directory");
			}
			if (!file_) {
				try {
					file_.open(std::filesystem::temp_directory_path(), file_.out | file_.app);
				}
				catch (...) {
					LogLastChance_(L"Could not open logger-panic.txt in temp directory, file panic logging will not be used");
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
				std::wcerr << msg << std::endl;
			}
			catch (...) {
				LogLastChance_(L"Failed panic logging to stderr");
			}
		}
		void LogWinDbg_(const std::wstring& msg) const noexcept
		{
			OutputDebugStringW(msg.c_str());
		}
		void LogFile_(const std::wstring& msg) noexcept
		{
			if (file_) {
				try {
					file_ << msg << std::endl;
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

	void Panic(const std::wstring& msg) noexcept
	{
		static PanicLogger_ panic;
		panic.Log(msg);
	}
}
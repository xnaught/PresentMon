#pragma once
#include "Level.h"
#include <chrono>
#include <optional>
#include <memory>
#include <variant>

namespace pmon::util::log
{
	class StackTrace;

	struct Entry
	{
		struct StaticSourceStrings
		{
			const wchar_t* file_ = nullptr;
			const wchar_t* functionName_ = nullptr;
		};
		struct HeapedSourceStrings
		{
			std::wstring file_;
			std::wstring functionName_;
		};
		// data fields 
		Level level_ = Level::Error;
		std::wstring note_;
		std::variant<StaticSourceStrings, HeapedSourceStrings> sourceStrings_;
		int sourceLine_ = -1;
		std::chrono::system_clock::time_point timestamp_;
		std::shared_ptr<StackTrace> pTrace_;
		std::optional<unsigned int> hResult_;
		uint32_t pid_;
		uint32_t tid_;
		// behavior override flags 
		std::optional<bool> captureTrace_;
		std::optional<bool> showSourceLine_;
	};
}

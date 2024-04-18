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
		struct RateControl
		{
			enum class Type : int
			{
				None,
				Every,
				EveryAndFirst,
				First,
				After,
				Hitcount,
			} type = Type::None;
			int parameter = -1;
		};
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
		RateControl rateControl_;
		int hitCount_ = -1;
		// behavior override flags 
		std::optional<bool> captureTrace_;
		std::optional<bool> showSourceLine_;
		// accessors
		std::wstring GetSourceFileName() const
		{
			if (auto p = std::get_if<StaticSourceStrings>(&sourceStrings_)) {
				if (p->file_) {
					return p->file_;
				}
				else {
					return {};
				}
			}
			return std::get<HeapedSourceStrings>(sourceStrings_).file_;
		}
	};
}

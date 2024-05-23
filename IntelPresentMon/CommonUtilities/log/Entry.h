#pragma once
#include "Level.h"
#include "Subsystem.h"
#include <chrono>
#include <optional>
#include <memory>
#include <variant>
#include "StackTrace.h"
#include "ErrorCode.h"
#include "../Memory.h"

namespace pmon::util::log
{
	struct Entry
	{
		// type
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
		Subsystem subsystem_ = Subsystem::None;
		std::wstring note_;
		std::variant<StaticSourceStrings, HeapedSourceStrings> sourceStrings_;
		int sourceLine_ = -1;
		std::chrono::system_clock::time_point timestamp_;
		CloningUptr<StackTrace> pTrace_;
		ErrorCode errorCode_;
		uint32_t pid_;
		uint32_t tid_;
		RateControl rateControl_;
		int hitCount_ = -1;
		bool diagnosticLayer_ = false;
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

#pragma once
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/chrono.hpp>
#include <cereal/types/variant.hpp>
#include "Entry.h"
#include "StackTraceCereal.h"

namespace pmon::util::log
{
	template<class Archive>
	void serialize(Archive& ar, Entry::HeapedSourceStrings& s)
	{
		ar(s.file_, s.functionName_);
	}
	template<class Archive>
	void serialize(Archive& ar, Entry& e)
	{
		ar(e.level_, e.note_, e.sourceLine_, e.timestamp_,
			e.pTrace_, e.hResult_, e.pid_, e.tid_);

		// special handling for source strings as they might contain raw pointer alternatives
		// we always serialize as wstring container regardless of source
		if constexpr (Archive::is_saving::value) {
			// raw pointer sources should convert to wstring
			if (auto pStrings = std::get_if<Entry::StaticSourceStrings>(&e.sourceStrings_)) {
				// Convert to HeapedSourceStrings for serialization
				auto file = pStrings->file_ ? std::wstring(pStrings->file_) : std::wstring{};
				auto functionName = pStrings->functionName_ ? std::wstring(pStrings->functionName_) : std::wstring{};
				Entry::HeapedSourceStrings tempHeapedStrings{ std::move(file), std::move(functionName) };
				ar(tempHeapedStrings);
			}
			// wstring source needs no extra work
			else if (auto pStrings = std::get_if<Entry::HeapedSourceStrings>(&e.sourceStrings_)) {
				ar(*pStrings);
			}
			// we should never get here, but if we do just serialize empty wstring source
			else {
				Entry::HeapedSourceStrings tempHeapedStrings{};
				ar(tempHeapedStrings);
			}
		}
		// we always deserialize as wstring container
		else {
			Entry::HeapedSourceStrings strings;
			ar(strings);
			e.sourceStrings_ = std::move(strings);
		}
	}
}
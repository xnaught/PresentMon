#pragma once
#include "Entry.h"

namespace pmon::util::log
{
	class IEntrySink;

	class EntryBuilder : private Entry
	{
	public:
		EntryBuilder(Level lvl, const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine) noexcept;
		EntryBuilder& note(std::wstring note) noexcept;
		EntryBuilder& to(IEntrySink*) noexcept;
		//EntryBuilder& trace_skip(int depth);
		//EntryBuilder& no_trace();
		//EntryBuilder& trace();
		EntryBuilder& no_line() noexcept;
		EntryBuilder& line() noexcept;
		EntryBuilder& hr() noexcept;
		EntryBuilder& hr(unsigned int) noexcept;
		~EntryBuilder();
		EntryBuilder& operator<<(std::wstring note) noexcept;
	private:
		IEntrySink* pDest_ = nullptr;
		int traceSkipDepth_ = 6;
	};

}

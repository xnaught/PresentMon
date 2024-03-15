#pragma once
#include "Entry.h"

namespace pmon::util::log
{
	class IEntrySink;

	class EntryBuilder : private Entry
	{
	public:
		EntryBuilder(const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine);
		EntryBuilder& note(std::wstring note);
		EntryBuilder& level(Level);
		EntryBuilder& to(IEntrySink*);
		//EntryBuilder& trace_skip(int depth);
		//EntryBuilder& no_trace();
		//EntryBuilder& trace();
		EntryBuilder& no_line();
		EntryBuilder& line();
		EntryBuilder& hr();
		EntryBuilder& hr(unsigned int);
		~EntryBuilder();
		EntryBuilder& operator<<(std::wstring note);
	private:
		IEntrySink* pDest_ = nullptr;
		int traceSkipDepth_ = 6;
	};

}

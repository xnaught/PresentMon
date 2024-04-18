#include "EntryBuilder.h"
#include "IChannel.h"
#include "../win/WinAPI.h"
#include "PanicLogger.h"
#include "StackTrace.h"
#include "GlobalPolicy.h"
#include "LineTable.h"

#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>
#include <sstream>
#include <cassert>


namespace pmon::util::log
{
	EntryBuilder::EntryBuilder(Level lvl, const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine) noexcept
		:
		Entry{
			.level_ = lvl,
			.sourceStrings_ = Entry::StaticSourceStrings{
				.file_ = sourceFile,
				.functionName_ = sourceFunctionName,
			},
			.sourceLine_ = sourceLine,
			.timestamp_ = std::chrono::system_clock::now(),
			.pid_ = GetCurrentProcessId(),
			.tid_ = GetCurrentThreadId(),
		}
	{}
	EntryBuilder& EntryBuilder::note(std::wstring note) noexcept
	{
		note_ = std::move(note);
		return *this;
	}
	EntryBuilder& EntryBuilder::to(IEntrySink* pSink) noexcept
	{
		pDest_ = pSink;
		return *this;
	}
	//EntryBuilder& EntryBuilder::trace_skip(int depth)
	//{
	//	traceSkipDepth_ = depth;
	//	return *this;
	//}
	//EntryBuilder& EntryBuilder::no_trace()
	//{
	//	captureTrace_ = false;
	//	return *this;
	//}
	//EntryBuilder& EntryBuilder::trace()
	//{
	//	captureTrace_ = true;
	//	return *this;
	//}
	EntryBuilder& EntryBuilder::no_line() noexcept
	{
		showSourceLine_ = false;
		return *this;
	}
	EntryBuilder& EntryBuilder::line() noexcept
	{
		showSourceLine_ = true;
		return *this;
	}
	EntryBuilder& EntryBuilder::hr() noexcept
	{
		hResult_ = GetLastError();
		return *this;
	}
	EntryBuilder& EntryBuilder::hr(unsigned int hr) noexcept
	{
		hResult_ = hr;
		return *this;
	}
	EntryBuilder& EntryBuilder::every(int n, bool includeFirst) noexcept
	{
		assert(n > 1);
		rateControl_ = {
			.type = includeFirst ? RateControl::Type::EveryAndFirst : RateControl::Type::Every,
			.parameter = n,
		};
		return *this;
	}
	EntryBuilder& EntryBuilder::first(int n) noexcept
	{
		assert(n > 0);
		rateControl_ = {
			.type = RateControl::Type::First,
			.parameter = n,
		};
		return *this;
	}
	EntryBuilder& EntryBuilder::after(int n) noexcept
	{
		assert(n > 0);
		rateControl_ = {
			.type = RateControl::Type::After,
			.parameter = n,
		};
		return *this;
	}
	EntryBuilder& EntryBuilder::hitcount() noexcept
	{
		rateControl_ = { .type = RateControl::Type::Hitcount };
		return *this;
	}
	EntryBuilder::~EntryBuilder()
	{
		if (pDest_) {
			auto tracing = captureTrace_.value_or((int)level_ <= (int)GlobalPolicy::GetTraceLevel());
			// do line override check
			if (!tracing && LineTable::TraceOverrideActive()) {
				if (auto pEntry = LineTable::TryLookup(GetSourceFileName(), sourceLine_)) {
					tracing = pEntry->traceOverride_;
				}
			}
			if (tracing) {
				try {
					pTrace_ = StackTrace::Here();
					if (GlobalPolicy::GetResolveTraceInClientThread()) {
						pTrace_->Resolve();
					}
				}
				catch (...) {
					pmlog_panic_(L"Failed to get current stacktrace");
				}
			}
			pDest_->Submit(std::move(*this));
		}
		else {
			pmlog_panic_(L"Log entry completed with no destination channel set");
		}
	}
	EntryBuilder& EntryBuilder::operator<<(std::wstring noteString) noexcept
	{
		note(std::move(noteString));
		return *this;
	}
}
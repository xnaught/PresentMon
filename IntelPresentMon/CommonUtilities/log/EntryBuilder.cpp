#include "EntryBuilder.h"
#include "IChannel.h"
#include "../str/String.h"
#include "../win/WinAPI.h"
#include "PanicLogger.h"
#include "StackTrace.h"
#include "GlobalPolicy.h"
#include "LineTable.h"
#include "../win/HrErrorCodeProvider.h"

#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>
#include <sstream>
#include <cassert>


#ifndef NDEBUG
#define PM_LOG_DEFAULT_TRACE_SKIP 3
#else
#define PM_LOG_DEFAULT_TRACE_SKIP 1
#endif


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
		},
		traceSkipDepth_{ PM_LOG_DEFAULT_TRACE_SKIP }
	{}
	EntryBuilder& EntryBuilder::note(std::wstring note) noexcept
	{
		note_ = std::move(note);
		return *this;
	}
	EntryBuilder& EntryBuilder::note(const std::string& note) noexcept
	{
		return this->note(str::ToWide(note));
	}
	EntryBuilder& EntryBuilder::to(std::shared_ptr<IEntrySink> pSink) noexcept
	{
		pDest_ = std::move(pSink);
		return *this;
	}
	EntryBuilder& EntryBuilder::trace_skip(int depth) noexcept
	{
		traceSkipDepth_ = depth;
		return *this;
	}
	EntryBuilder& EntryBuilder::no_trace() noexcept
	{
		captureTrace_ = false;
		return *this;
	}
	EntryBuilder& EntryBuilder::trace() noexcept
	{
		captureTrace_ = true;
		return *this;
	}
	EntryBuilder& EntryBuilder::hr() noexcept
	{
		return hr(GetLastError());
	}
	EntryBuilder& EntryBuilder::hr(uint32_t hres) noexcept
	{
		return code(win::hr_wrap{ hres });
	}
	EntryBuilder& EntryBuilder::every(int n, bool includeFirst) noexcept
	{
		assert(n > 0);
		if (n > 1) {
			rateControl_ = {
				.type = includeFirst ? RateControl::Type::EveryAndFirst : RateControl::Type::Every,
				.parameter = n,
			};
		}
		else {
			rateControl_ = {};
		}
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
			auto tracing = captureTrace_.value_or((int)level_ <= (int)GlobalPolicy::Get().GetTraceLevel());
			// do line override check
			if (LineTable::GetTraceOverride()) {
				if (auto pEntry = LineTable::TryLookup(GetSourceFileName(), sourceLine_)) {
					switch (pEntry->traceOverride_) {
					case LineTable::TraceOverride::ForceOn: tracing = true; break;
					case LineTable::TraceOverride::ForceOff: tracing = false; break;
					}
				}
			}
			if (tracing) {
				try {
					pTrace_ = StackTrace::Here(traceSkipDepth_);
					if (GlobalPolicy::Get().GetResolveTraceInClientThread()) {
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
}
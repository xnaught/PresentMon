#include "EntryBuilder.h"
#include "IChannel.h"
#include "../win/WinAPI.h"
#include "PanicLogger.h"
#include "StackTrace.h"


namespace pmon::util::log
{
	EntryBuilder::EntryBuilder(Level lvl, const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine) noexcept
		:
		Entry{
			.level_ = lvl,
			.sourceFile_ = sourceFile,
			.sourceFunctionName_ = sourceFunctionName,
			.sourceLine_ = sourceLine,
			.timestamp_ = std::chrono::system_clock::now(),
		}
	{
		static uint32_t pid = GetProcessId(GetCurrentProcess());;
		pid_ = pid;
		thread_local uint32_t tid = GetCurrentThreadId();;
		tid_ = tid;
	}
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
	EntryBuilder::~EntryBuilder()
	{
		if (pDest_) {
			if (captureTrace_.value_or((int)level_ <= (int)Level::Error)) {
				try {
					pTrace_ = StackTrace::Here();
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
#include "EntryBuilder.h"
#include "IChannel.h"
#include "../win/WinAPI.h"
#include "PanicLogger.h"


namespace pmon::util::log
{
	EntryBuilder::EntryBuilder(const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine) noexcept
		:
		Entry{
			.sourceFile_ = sourceFile,
			.sourceFunctionName_ = sourceFunctionName,
			.sourceLine_ = sourceLine,
			.timestamp_ = std::chrono::system_clock::now(),
		}
	{}
	EntryBuilder& EntryBuilder::note(std::wstring note) noexcept
	{
		note_ = std::move(note);
		return *this;
	}
	EntryBuilder& EntryBuilder::level(Level level) noexcept
	{
		level_ = level;
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
					pTrace_ = std::make_unique<std::stacktrace>(std::stacktrace::current());
				}
				catch (...) {
					Panic(L"Failed to get current stacktrace");
				}
			}
			pDest_->Submit(std::move(*this));
		}
		else {
			Panic(L"Log entry completed with no destination channel set");
		}
	}
	EntryBuilder& EntryBuilder::operator<<(std::wstring noteString) noexcept
	{
		note(std::move(noteString));
		return *this;
	}
}
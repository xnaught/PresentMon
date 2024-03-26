#include "EntryBuilder.h"
#include "IChannel.h"
#include "../win/WinAPI.h"


namespace pmon::util::log
{
	EntryBuilder::EntryBuilder(const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine)
		:
		Entry{
			.sourceFile_ = sourceFile,
			.sourceFunctionName_ = sourceFunctionName,
			.sourceLine_ = sourceLine,
			.timestamp_ = std::chrono::system_clock::now(),
		}
	{}
	EntryBuilder& EntryBuilder::note(std::wstring note)
	{
		note_ = std::move(note);
		return *this;
	}
	EntryBuilder& EntryBuilder::level(Level level)
	{
		level_ = level;
		return *this;
	}
	EntryBuilder& EntryBuilder::to(IEntrySink* pSink)
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
	EntryBuilder& EntryBuilder::no_line()
	{
		showSourceLine_ = false;
		return *this;
	}
	EntryBuilder& EntryBuilder::line()
	{
		showSourceLine_ = true;
		return *this;
	}
	EntryBuilder& EntryBuilder::hr()
	{
		hResult_ = GetLastError();
		return *this;
	}
	EntryBuilder& EntryBuilder::hr(unsigned int hr)
	{
		hResult_ = hr;
		return *this;
	}
	EntryBuilder::~EntryBuilder()
	{
		if (pDest_) {
			if (captureTrace_.value_or((int)level_ <= (int)Level::Error)) {
				pTrace_ = std::make_unique<std::stacktrace>(std::stacktrace::current());
			}
			pDest_->Submit(std::move(*this));
		}
	}
	EntryBuilder& EntryBuilder::operator<<(std::wstring noteString)
	{
		note(std::move(noteString));
		return *this;
	}
}
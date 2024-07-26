#pragma once
#include "Entry.h"
#include <format>
#include <memory>
#include "TimePoint.h"
#include "PanicLogger.h"

namespace pmon::util::log
{
	class IEntrySink;

	class EntryBuilder : private Entry
	{
	public:
		EntryBuilder(Level lvl, const char* sourceFile, const char* sourceFunctionName, int sourceLine) noexcept;
		EntryBuilder(Level lvl, std::string sourceFile, std::string, int sourceLine) noexcept;

		EntryBuilder(const EntryBuilder&) = delete;
		EntryBuilder & operator=(const EntryBuilder&) = delete;

		~EntryBuilder();
		template<typename T>
		EntryBuilder& watch(const char* symbol, const T& value) noexcept
		{
			try {
				if (note_.empty()) {
					note_ += std::format("   {} => {}", symbol, value);
				}
				else {
					note_ += std::format("\n     {} => {}", symbol, value);
				}
			}
			catch (...) { pmlog_panic_("Failed to format watch in EntryBuilder"); }
			return *this;
		}
		EntryBuilder& mark(const TimePoint& tp) noexcept;
		EntryBuilder& note(std::string note = "") noexcept;
		EntryBuilder& note(const std::string& note) noexcept;
		EntryBuilder& to(std::shared_ptr<IEntrySink>) noexcept;
		EntryBuilder& trace_skip(int depth) noexcept;
		EntryBuilder& no_trace() noexcept;
		EntryBuilder& trace() noexcept;
		EntryBuilder& hr() noexcept;
		EntryBuilder& hr(uint32_t) noexcept;
		EntryBuilder& every(int n, bool includeFirst = true) noexcept;
		EntryBuilder& first(int n) noexcept;
		EntryBuilder& after(int n) noexcept;
		EntryBuilder& hitcount() noexcept;
		EntryBuilder& diag() noexcept;
		EntryBuilder& subsys(Subsystem sys) noexcept;
		template<typename T>
		EntryBuilder& code(const T& code) noexcept
		{
			errorCode_ = { code };
			return *this;
		} 
	private:
		std::shared_ptr<IEntrySink> pDest_;
		int traceSkipDepth_;
		std::optional<bool> captureTrace_;
	};

}

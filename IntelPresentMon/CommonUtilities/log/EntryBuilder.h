#pragma once
#include "Entry.h"
#include <format>
#include <memory>

namespace pmon::util::log
{
	class IEntrySink;

	class EntryBuilder : private Entry
	{
	public:
		EntryBuilder(Level lvl, const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine) noexcept;
		EntryBuilder(Level lvl, std::wstring sourceFile, std::wstring, int sourceLine) noexcept;
		template<typename T>
		EntryBuilder& watch(const wchar_t* symbol, const T& value) noexcept
		{
			if (note_.empty()) {
				note_ += std::format(L"   {} => {}", symbol, value);
			}
			else {
				note_ += std::format(L"\n     {} => {}", symbol, value);
			}
			return *this;
		}
		EntryBuilder& note(std::wstring note = L"") noexcept;
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
		template<typename T>
		EntryBuilder& code(const T& code) noexcept
		{
			errorCode_ = { code };
			return *this;
		} 
		~EntryBuilder();
	private:
		std::shared_ptr<IEntrySink> pDest_;
		int traceSkipDepth_;
		std::optional<bool> captureTrace_;
	};

}

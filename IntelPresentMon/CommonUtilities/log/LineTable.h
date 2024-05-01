#pragma once
#include <chrono>
#include <atomic>
#include <shared_mutex>
#include <unordered_map>

namespace pmon::util::log
{
	class LineTable
	{
	public:
		enum class ListMode
		{
			None,
			Black,
			White,
		};
		struct Entry
		{
			uint32_t NextHit()
			{
				return ++hitCount_;
			}
			uint32_t PeekHit() const
			{
				return hitCount_;
			}
			std::atomic<uint32_t> hitCount_ = 0;
			std::atomic<bool> isListed_ = false;
			std::atomic<bool> traceOverride_ = false;
		};
		static Entry* TryLookup(const std::wstring& file, int line) noexcept;
		static Entry& Lookup(const std::wstring& file, int line) noexcept;
		// blacklist, whitelist, or none
		static ListMode GetListMode() noexcept;
		static void SetListMode(ListMode mode) noexcept;
		// dictates whether line lookups necessary to check for per-line trace overrides
		static bool GetTraceOverride() noexcept;
		static void SetTraceOverride(bool) noexcept;
		static void RegisterListItem(const std::wstring& file, int line, bool isTrace) noexcept;
		// returns true if there were any trace override lines
		static bool IngestList(const std::wstring& path);

		// implementation functions
		static LineTable& Get_();
		LineTable::Entry* TryLookup_(const std::wstring& file, int line);
		Entry& Lookup_(const std::wstring& file, int line);
		void RegisterListItem_(const std::wstring& file, int line, bool isTrace);
		bool GetTraceOverride_() const noexcept;
		void SetTraceOverride_(bool) noexcept;
		ListMode GetListMode_() const noexcept;
		void SetListMode_(ListMode mode) noexcept;
	private:
		// functions
		void RegisterListItem_(const std::wstring& key, bool isTrace);
		static std::wstring MakeKey_(const std::wstring& file, int line);
		// data
		mutable std::shared_mutex mtx_;
		std::unordered_map<std::wstring, Entry> lines_;
		std::atomic<ListMode> listMode_ = ListMode::None;
		std::atomic<bool> traceOverride_ = false;
		static Entry dummyEntry_;
	};
}
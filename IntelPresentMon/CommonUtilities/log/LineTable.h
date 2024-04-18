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
			bool isListed_ = false;
		};
		static Entry* TryLookup(const std::wstring& file, int line) noexcept;
		static Entry& Lookup(const std::wstring& file, int line) noexcept;
		static ListMode GetListMode() noexcept;
		static void SetListMode(ListMode mode) noexcept;
		static void RegisterListItem(const std::wstring& file, int line) noexcept;
	private:
		// function
		static LineTable& Get_();
		LineTable::Entry* TryLookup_(const std::wstring& file, int line);
		Entry& Lookup_(const std::wstring& file, int line);
		ListMode GetListMode_() const;
		void SetListMode_(ListMode mode);
		void RegisterListItem_(const std::wstring& file, int line);
		static std::wstring MakeKey_(const std::wstring& file, int line);
		// data
		mutable std::shared_mutex mtx_;
		std::unordered_map<std::wstring, Entry> lines_;
		std::atomic<ListMode> listMode_ = ListMode::None;
		static Entry dummyEntry_;
	};
}
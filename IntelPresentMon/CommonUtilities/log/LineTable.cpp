#include "LineTable.h"

namespace pmon::util::log
{
	LineTable::Entry LineTable::dummyEntry_ = {};

	LineTable::Entry* LineTable::TryLookup(const std::wstring& file, int line) noexcept
	{
		try {
			return Get_().TryLookup_(file, line);
		}
		catch (...) { return nullptr; }
	}
	LineTable::Entry& LineTable::Lookup(const std::wstring& file, int line) noexcept
	{
		try {
			return Get_().Lookup_(file, line);
		}
		catch (...) { return dummyEntry_; }
	}
	LineTable::ListMode LineTable::GetListMode() noexcept
	{
		return Get_().GetListMode_();
	}
	void LineTable::SetListMode(ListMode mode) noexcept
	{
		Get_().SetListMode_(mode);
	}
	void LineTable::RegisterListItem(const std::wstring& file, int line) noexcept
	{
		Get_().RegisterListItem_(file, line);
	}

	LineTable& LineTable::Get_()
	{
		static LineTable table;
		return table;
	}
	LineTable::Entry* LineTable::TryLookup_(const std::wstring& file, int line)
	{
		std::shared_lock lk{ mtx_ };
		if (auto i = lines_.find(MakeKey_(file, line)); i != lines_.end()) {
			return &i->second;
		}
		return nullptr;
	}
	LineTable::Entry& LineTable::Lookup_(const std::wstring& file, int line)
	{
		return lines_[MakeKey_(file, line)];
	}
	LineTable::ListMode LineTable::GetListMode_() const
	{
		return listMode_;
	}
	void LineTable::SetListMode_(ListMode mode)
	{
		listMode_ = mode;
	}
	void LineTable::RegisterListItem_(const std::wstring& file, int line)
	{
		lines_[MakeKey_(file, line)];
	}
	std::wstring LineTable::MakeKey_(const std::wstring& file, int line)
	{
		return file + L"+" + std::to_wstring(line);
	}
}
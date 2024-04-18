#include "LineTable.h"
#include <fstream>
#include <regex>
#include "../str/String.h"
#include "Log.h"

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

	void LineTable::IngestList(const std::wstring& path)
	{
		if (std::wifstream listFile{ path }) {
			std::wregex patternRaw(LR"(.*\?.+\d+$)");
			std::wstring line;
			while (std::getline(listFile, line)) {
				line = str::TrimWhitespace(line);
				if (line.empty()) {
					continue;
				}
				// try to match a line in the table key format +42
				if (std::regex_match(line, patternRaw)) {
					Get_().RegisterListItem_(line);
				}
				else {
					std::wregex patternLogged(LR"((.+)\((\d+)\)$)");
					std::wsmatch matches;
					// try to match a line in the log line format (42)
					if (std::regex_search(line, matches, patternLogged)) {
						if (matches.size() == 3) {  // matches[0] is the whole string, [1] is the path, [2] is the number
							RegisterListItem(matches[1], std::stoi(matches[2]));
						}
					}
				}
			}
		}
		else {
			pmlog_error();
			throw std::runtime_error{ "Failed to open listfile {}" };
		}
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
		RegisterListItem_(MakeKey_(file, line));
	}
	void LineTable::RegisterListItem_(const std::wstring& key)
	{
		lines_[key].isListed_ = true;
	}
	std::wstring LineTable::MakeKey_(const std::wstring& file, int line)
	{
		return file + L"?" + std::to_wstring(line);
	}
}
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
		try {
			return Get_().listMode_;
		}
		catch (...) { return ListMode::None; }
	}
	void LineTable::SetListMode(ListMode mode) noexcept
	{
		try {
			Get_().listMode_ = mode;
		}
		catch (...) {}
	}
	bool LineTable::TraceOverrideActive() noexcept
	{
		try {
			return Get_().traceOverride_;
		}
		catch (...) { return false; }
	}
	void LineTable::SetTraceOverride(bool ov) noexcept
	{
		try {
			Get_().traceOverride_ = ov;
		}
		catch (...) {}
	}
	void LineTable::RegisterListItem(const std::wstring& file, int line, bool isTrace) noexcept
	{
		try {
			Get_().RegisterListItem_(file, line, isTrace);
		}
		catch (...) {}
	}
		
	bool LineTable::IngestList(const std::wstring& path)
	{
		if (std::wifstream listFile{ path }) {
			std::wstring line;
			bool traceover = false;
			while (std::getline(listFile, line)) {
				line = str::TrimWhitespace(line);
				if (line.empty()) {
					continue;
				}
				// detect marker for trace control line and remove
				const auto traceControl = line.back() == L'%';
				if (traceControl) {
					line.pop_back();
					traceover = true;
				}
				// check if pattern matches line identifier
				std::wregex patternLogged(LR"((.+)\((\d+)\)$)");
				std::wsmatch matches;
				// try to match a line in the log line format (42)
				if (std::regex_search(line, matches, patternLogged)) {
					if (matches.size() == 3) {  // matches[0] is the whole string, [1] is the path, [2] is the number
						RegisterListItem(matches[1], std::stoi(matches[2]), traceControl);
					}
				}
			}
			return traceover;
		}
		else {
			pmlog_error();
			throw std::runtime_error{ "Failed to open listfile {}" };
		}
	}

	LineTable& LineTable::Get_()
	{
		// @SINGLETON
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
	void LineTable::RegisterListItem_(const std::wstring& file, int line, bool isTrace)
	{
		RegisterListItem_(MakeKey_(file, line), isTrace);
	}
	void LineTable::RegisterListItem_(const std::wstring& key, bool isTrace)
	{
		auto& e = lines_[key];
		if (isTrace) {
			e.traceOverride_ = true;
		}
		else {
			e.isListed_ = true;
		}
	}
	std::wstring LineTable::MakeKey_(const std::wstring& file, int line)
	{
		return file + L"?" + std::to_wstring(line);
	}
}
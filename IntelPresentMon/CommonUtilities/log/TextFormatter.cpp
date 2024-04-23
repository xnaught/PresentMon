#include "TextFormatter.h"
#include <chrono>
#include <format>
#include <sstream>
#include "Entry.h"
#include "../win/Utilities.h"
#include "../str/String.h"
#include <ranges>
#include "PanicLogger.h"
#include "StackTrace.h"
#include "IdentificationTable.h"
#include "IErrorCodeResolver.h"

namespace pmon::util::log
{
	std::wstring MakeProc_(const Entry& e)
	{
		std::wstring text;
		if (auto proc = IdentificationTable::LookupProcess(e.pid_)) {
			text = std::format(L"{}({})", proc->name, proc->pid);
		}
		else {
			text = std::to_wstring(e.pid_);
		}
		return text;
	}
	std::wstring MakeThread_(const Entry& e)
	{
		std::wstring text;
		if (auto thread = IdentificationTable::LookupThread(e.tid_)) {
			text = std::format(L"{}({})", thread->name, thread->tid);
		}
		else {
			text = std::to_wstring(e.tid_);
		}
		return text;
	}


	std::wstring TextFormatter::Format(const Entry& e) const
	{
		try {
			std::wostringstream oss;
			oss << std::format(L"[@{}] <{}:{}> {{{}}}\n  {}",
				GetLevelName(e.level_),
				MakeProc_(e),
				MakeThread_(e),
				std::chrono::zoned_time{ std::chrono::current_zone(), e.timestamp_ },
				e.note_
			);
			if (e.errorCode_) {
				auto& ec = e.errorCode_;
				if (ec.IsResolvedNontrivial()) {
					auto pStrings = ec.GetStrings();
					oss << std::format(L"\n  !{} [{}]:{}) {}", pStrings->type, ec.AsHex(), pStrings->name, pStrings->description);
				}
				else {
					oss << std::format(L"\n  !UNKNOWN [{}]", ec.AsHex());
				}
			}
			// display of source line info could be controlled here
			// if so, hitcount would need to be separately handled
			if (true) {
				std::visit([&](auto& strings) {
					oss << std::format(L"\n  >> at {} {}\n     {}({})\n",
						strings.functionName_,
						[&] { return e.hitCount_ == -1 ? std::wstring{} : std::format(L"[Hits: {}]", e.hitCount_); }(),
						strings.file_,
						e.sourceLine_
					);
				}, e.sourceStrings_);
			}
			if (e.pTrace_) {
				try {
					oss << L" ====== STACK TRACE (newest on top) ======\n";
					oss << e.pTrace_->ToString();
					oss << L" =========================================\n";
				}
				catch (...) {
					pmlog_panic_(L"Failed printing stack trace in TextFormatter::Format");
				}
			}
			return oss.str();
		}
		catch (...) {
			pmlog_panic_(L"Exception in TextFormatter::Format");
			return {};
		}
	}
}
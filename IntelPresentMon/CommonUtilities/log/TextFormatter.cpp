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
			if (e.hResult_) {
				oss << std::format(L"\n  !HRESULT [{:#010x}]: {}", *e.hResult_,
					win::GetErrorDescription(*e.hResult_));
			}
			if (e.showSourceLine_.value_or(true)) {
				std::visit([&](auto& strings) {
					oss << std::format(L"\n  >> at {}\n     {}({})\n",
						strings.functionName_,
						strings.file_,
						e.sourceLine_
					);
				}, e.sourceStrings_);
			}
			else {
				oss << L"\n";
			}
			if (e.pTrace_) {
				try {
					oss << L" ====== STACK TRACE (newest on top) ======\n";
					if (e.pTrace_->Resolved()) {
						for (auto& f : e.pTrace_->GetFrames()) {
							oss << L"  [" << f.index << L"] " << f.description << L"\n";
							if (f.line != 0 || !f.file.empty()) {
								oss << L"    > " << f.file << L'(' << f.line << L")\n";
							}
						}
					}
					else {
						oss << L"\n   !! UNRESOLVED STACK TRACE !!\n\n";
					}
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
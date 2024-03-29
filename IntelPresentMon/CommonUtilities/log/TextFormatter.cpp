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

namespace pmon::util::log
{
	std::wstring TextFormatter::Format(const Entry& e) const
	{
		try {
			std::wostringstream oss;
			oss << std::format(L"[@{}] <{}:{}> {{{}}} {}",
				GetLevelName(e.level_),
				e.pid_,
				e.tid_,
				std::chrono::zoned_time{ std::chrono::current_zone(), e.timestamp_ },
				e.note_
			);
			if (e.hResult_) {
				oss << std::format(L"\n  !HRESULT [{:#010x}]: {}", *e.hResult_,
					win::GetErrorDescription(*e.hResult_));
			}
			if (e.showSourceLine_.value_or(true)) {
				oss << std::format(L"\n  >> at {}\n     {}({})\n",
					e.sourceFunctionName_,
					e.sourceFile_,
					e.sourceLine_
				);
			}
			else {
				oss << L"\n";
			}
			if (e.pTrace_) {
				try {
					oss << L" ====== STACK TRACE (newest on top) ======\n";
					for (auto& f : e.pTrace_->GetFrames()) {
						oss << L"  [" << f.index << L"] " << f.description << L"\n";
						if (f.line != 0 || !f.file.empty()) {
							oss << L"    > " << f.file << L'(' << f.line << L")\n";
						}
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
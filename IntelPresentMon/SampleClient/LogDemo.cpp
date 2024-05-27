#include "LogDemo.h"
#include <iostream>
#include <format>
#include <map>
#include <conio.h>
#include "CliOptions.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../CommonUtilities/win/WinAPI.h"
// #define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Verbose
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/NamedPipeMarshallReceiver.h"
#include "../CommonUtilities/log/NamedPipeMarshallSender.h"
#include "../CommonUtilities/log/MarshallDriver.h"
#include "../CommonUtilities/log/EntryMarshallInjector.h"
#include "../CommonUtilities/log/IdentificationTable.h"
#include "../CommonUtilities/log/LineTable.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/win/Utilities.h"
// #define VVV_LOGDEMO
#include "Verbose.h"
#include "LogSetup.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPIWrapper/DiagnosticHandler.h"

using namespace std::chrono_literals;
using namespace pmon::util;

PM_DEFINE_EX(LogDemoException);

const char* GetLevelName(PM_DIAGNOSTIC_LEVEL lvl) {
	switch (lvl) {
	case PM_DIAGNOSTIC_LEVEL_NONE: return "None";
	case PM_DIAGNOSTIC_LEVEL_FATAL: return "Fatal";
	case PM_DIAGNOSTIC_LEVEL_ERROR: return "Error";
	case PM_DIAGNOSTIC_LEVEL_WARNING: return "Warning";
	case PM_DIAGNOSTIC_LEVEL_INFO: return "Info";
	case PM_DIAGNOSTIC_LEVEL_PERFORMANCE: return "Performance";
	case PM_DIAGNOSTIC_LEVEL_DEBUG: return "Debug";
	case PM_DIAGNOSTIC_LEVEL_VERBOSE: return "Verbose";
	default: return "Unknown";
	}
}

void f2() {
	pmlog_error();
}
void f1() {
	f2();
}

void g2() {
	throw Except<LogDemoException>("something bad happened here");
}
void g1() {
	g2();
}

void RunLogDemo(int mode)
{
	p2sam::LogChannelManager zLogMan_;
	p2sam::ConfigureLogging();

	// example of setting up diagnostic layer and custom diagnostic message handling
	pmapi::DiagnosticHandler dh{
		PM_DIAGNOSTIC_LEVEL_INFO,
		PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER | PM_DIAGNOSTIC_OUTPUT_FLAGS_QUEUE,
		[](const PM_DIAGNOSTIC_MESSAGE& msg) { std::cout <<
			std::format("[PMON {}] <{}> {}\n", GetLevelName(msg.level),
				msg.pTimestamp ? msg.pTimestamp : "", msg.pText);
		}
	};

	pmapi::Session sesh;

	for (auto&& o : clio::Options::Get().GetForwardedOptions()) {
		std::cout << std::format("({}, {})", o.first, o.second) << std::endl;
	}
	
	// basic log info w/ message
	if (mode == 0) {
		pmlog_info(L"information goes here");
		PM_DIAGNOSTIC_MESSAGE dm{
			.level = PM_DIAGNOSTIC_LEVEL_WARNING,
			.system = PM_DIAGNOSTIC_SUBSYSTEM(PM_DIAGNOSTIC_SUBSYSTEM_USER + 2),
			.pText = "@#$ test test test",
		};
		pmDiagnosticEnqueueMessage(&dm);
	}
	// basic warn w/ format message
	else if (mode == 1) {
		const int x = 4224;
		pmlog_warn(std::format(L"warning with variable x = {}", x));
	}
	// error log has trace (no message)
	else if (mode == 2) {
		f1();
	}
	// disable info at runtime
	else if (mode == 3) {
		log::GlobalPolicy::Get().SetLogLevel(log::Level::Warning);
		pmlog_info(L"info hidden");
		pmlog_warn(L"warn gets through");
	}
	// show build release disable (info/warn)
	else if (mode == 4) {
		pmlog_info(L"info hidden in release, shows in debug");
		pmlog_error(L"error always shows");
	}
	// set trace level
	else if (mode == 5) {
		log::GlobalPolicy::Get().SetTraceLevel(log::Level::Warning);
		pmlog_warn(L"enable stack trace for warning");
	}
	// error code logging hr
	else if (mode == 6) {
		CreateFileA("this-doesnt-exist.fake", 0, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		pmlog_error().hr();
	}
	// error code logging pmon
	else if (mode == 7) {
		pmlog_error().code(PM_STATUS_INVALID_ADAPTER_ID);
	}
	// watch for variables
	else if (mode == 8) {
		const int x = 2, y = 10;
		pmlog_info().pmwatch(x).pmwatch(y).pmwatch(std::pow(x, y));
	}
	// loop frequency control 3 variants
	else if (mode == 9) {
		for (int i = 0; i < 9; i++) {
			pmlog_warn(L"every 3").every(3);
			pmlog_warn(L"first 2").first(2);
			pmlog_warn(L"after 6").after(6);
		}
	}
	// verbose logging in a loop
	else if (mode == 10) {
		log::GlobalPolicy::Get().SetLogLevel(log::Level::Verbose);
		pmlog_info(L"starting loop");
		for (int i = 0; i < 20; i++) {
			pmlog_verb(v::logdemo)(L"in loop").pmwatch(i);
		}
		pmlog_info(L"loop complete");
	}
	// define, throw, catch custom exception
	else if (mode == 11) {
		try {
			g1();
		}
		catch (...) {
			std::cout << ReportException();
		}
	}
	// throw exception with trace (nested)
	else if (mode == 12) {
		log::GlobalPolicy::Get().SetExceptionTrace(true);
		try {
			g1();
		}
		catch (...) {
			std::cout << ReportException();
		}
	}
	// throw std exception
	else if (mode == 13) {
		try {
			auto x = std::map<int, int>{}.at(0);
		}
		catch (...) {
			std::cout << ReportException();
		}
	}
	// throw unknown exception
	else if (mode == 14) {
		try {
			throw std::vector<int>{};
		}
		catch (...) {
			std::cout << ReportException();
		}
	}
	// Seh exception catching with trace (nested)
	else if (mode == 15) {
		log::GlobalPolicy::Get().SetExceptionTrace(true);
		try {
			auto h = CreateEventA(nullptr, 0, 0, nullptr);
			CloseHandle(h);
			CloseHandle(h);
		}
		catch (...) {
			std::cout << ReportException();
		}
	}
	// named process / thread
	else if (mode == 16) {
		log::IdentificationTable::AddThisProcess(L"myprocy");
		log::IdentificationTable::AddThisThread(L"threadx");
		pmlog_info(L"rich point-of-origin context");
	}
	// ipc server setup (server always setup), set thread name special command
	else if (mode == 17) {
		auto pSender = std::make_shared<log::NamedPipeMarshallSender>(L"pml_demopipe");
		{
			log::IdentificationTable::RegisterSink(pSender);
			auto pDriver = std::make_shared<log::MarshallDriver>(pSender);
			log::GetDefaultChannel()->AttachComponent(std::move(pDriver));
		}
		std::wstring note;
		while (true) {
			int x = 3;
			std::cout << "SAY> ";
			std::getline(std::wcin, note);
			if (note.empty()) break;
			if (note.front() == L'%') {
				log::IdentificationTable::AddThisThread(note);
			}
			else if (note.front() == L'$') {
				log::IdentificationTable::AddThisProcess(note);
			}
			else if (note.front() == L'#') {
				std::cout << "Waiting for connection..." << std::endl;
				pSender->WaitForConnection();
				std::cout << "We in there!" << std::endl;
			}
			else {
				pmlog_info(note);
			}
		}
	}
	// ipc client
	else if (mode == 18) {
		log::GetDefaultChannel()->AttachComponent(std::make_shared<log::EntryMarshallInjector>(log::GetDefaultChannel(),
			std::make_shared<log::NamedPipeMarshallReceiver>(L"pml_demopipe", log::IdentificationTable::GetPtr())));
		while (!_kbhit());
	}
	// blacklist to disable and force trace
	else if (mode == 19) {
		try {
			pmon::util::log::LineTable::IngestList(L"black.txt", true);
			pmon::util::log::LineTable::SetListMode(pmon::util::log::LineTable::ListMode::Black);
			pmon::util::log::LineTable::SetTraceOverride(true);
		}
		catch (...) {
			std::cout << "Couldn't process black.txt\n";
			return;
		}
		pmlog_info(L"b1");
		pmlog_info(L"b2");
		pmlog_info(L"b3");
	}
	else {
		std::cout << "unknown mode for log demo" << std::endl;
	}
}
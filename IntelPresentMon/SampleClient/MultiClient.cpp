#include "MultiClient.h"
#include "CliOptions.h"
#include <PresentMonAPIWrapperCommon/EnumMap.h>
#include <chrono>
#include <iostream>

using namespace std::literals;

int MultiClientTest(std::unique_ptr<pmapi::Session> pSession)
{
	auto& opt = clio::Options::Get();

	std::optional<PM_STATUS> errorStatus;
	try {
		if (opt.telemetryPeriodMs) {
			pSession->SetTelemetryPollingPeriod(0, *opt.telemetryPeriodMs);
		}
		if (opt.etwFlushPeriodMs) {
			pSession->SetEtwFlushPeriod(*opt.etwFlushPeriodMs);
		}
	}
	catch (const pmapi::ApiErrorException& e) {
		if (!opt.testExpectError) {
			throw;
		}
		errorStatus = e.GetCode();
	}

	std::string line;

	// ping gate to sync on init finished
	std::getline(std::cin, line);
	if (line != "%ping") {
		std::cout << "%%{ping-error}%%" << std::endl;
		return -1;
	}
	std::cout << "%%{ping-ok}%%" << std::endl;

	// if we captured an error, wait here for error ack
	if (errorStatus) {
		std::getline(std::cin, line);
		if (line != "%err-check") {
			std::cout << "%%{err-check-error}%%" << std::endl;
			return -1;
		}
		auto&& err = pmapi::EnumMap::GetKeyMap(PM_ENUM_STATUS)->at(*errorStatus).narrowSymbol;
		std::cout << "%%{err-check-ok:" << err << "}%%" << std::endl;
	}

	// wait for command
	while (std::getline(std::cin, line)) {
		if (line == "%quit") {
			std::cout << "%%{quit-ok}%%" << std::endl;
			return 0;
		}
		else {
			std::cout << "%%{err-bad-command}%%" << std::endl;
		}
	}

	return -1;
}
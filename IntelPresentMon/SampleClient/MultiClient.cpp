#include "MultiClient.h"
#include "CliOptions.h"
#include <chrono>
#include <iostream>

using namespace std::literals;

int MultiClientTest(std::unique_ptr<pmapi::Session> pSession)
{
	auto& opt = clio::Options::Get();

	if (opt.telemetryPeriodMs) {
		pSession->SetTelemetryPollingPeriod(0, *opt.telemetryPeriodMs);
	}
	if (opt.etwFlushPeriodMs) {
		pSession->SetEtwFlushPeriod(*opt.etwFlushPeriodMs);
	}

	std::string line;

	// ping gate to sync on init finished
	std::getline(std::cin, line);
	if (line != "%ping") {
		std::cout << "%%{ping-error}%%" << std::endl;
		return -1;
	}
	std::cout << "%%{ping-ok}%%" << std::endl;

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
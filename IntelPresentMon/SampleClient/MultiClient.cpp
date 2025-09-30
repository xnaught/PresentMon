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

	std::string line;

	// ping to signal init finished
	std::getline(std::cin, line);
	if (line != "%ping") {
		std::cout << "%%{ping-error}%%" << std::endl;
		return -1;
	}
	std::cout << "%%{ping-ok}%%" << std::endl;

	// wait for quit
	std::getline(std::cin, line);
	if (line != "%quit") {
		std::cout << "%%{quit-error}%%" << std::endl;
		return -1;
	}
	std::cout << "%%{quit-ok}%%" << std::endl;

	return 0;
}
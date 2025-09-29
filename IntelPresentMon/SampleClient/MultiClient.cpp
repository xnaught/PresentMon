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

	//std::string line;

	//// ping to signal init finished
	//std::getline(std::cin, line);
	//if (line != "%ping") {
	//	std::cout << "error-ping%%}" << std::endl;
	//	return -1;
	//}
	//std::cout << "ping-ok%%}" << std::endl;

	//// wait for quit
	//std::getline(std::cin, line);
	//if (line != "%quit") {
	//	std::cout << "error-quit%%}" << std::endl;
	//	return -1;
	//}
	//std::cout << "quit-ok%%}" << std::endl;


	std::this_thread::sleep_for(1.5s);

	return 0;
}
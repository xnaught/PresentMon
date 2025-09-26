#include "TestControl.h"
#include "../PresentMon.h"
#include <iostream>
#include <sstream>
#include <cereal/archives/json.hpp>

namespace pmon::svc::testing
{
	TestControlModule::TestControlModule(const PresentMon* pSession)
		:
		pPresentMon_{ pSession },
		worker_{ &TestControlModule::Run_, this }
	{}
	void TestControlModule::Run_()
	{
		std::string line;
		while (std::getline(std::cin, line)) {
			if (line == "%quit") {
				std::cout << "{%QUIT_OK%}" << std::endl;
				break;
			}
			else if (line == "%status") {
				std::ostringstream oss;
				cereal::JSONOutputArchive{ oss }(pPresentMon_->GetTestingStatus());
				std::cout << "{%" << oss.str() << "%}" << std::endl;
			}
			else {
				std::cout << "{%BAD_COMMAND%}" << std::endl;
			}
		}
	}
}
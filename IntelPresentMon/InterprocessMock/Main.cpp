#include <iostream>
#include "../Interprocess/source/ExperimentalInterprocess.h"
#include "Options.h"

int main(int argc, char** argv)
{
	using namespace pmon::ipc;
	using namespace pmon::ipc::mock;

	// parse command line options
	try {
		opt::init(argc, argv);
	}
	catch (const CLI::ParseError& e) {
		return e.get_exit_code();
	};

	// shortcut for command line options
	const auto& opts = opt::get();

	if (opts.testF) {
		std::cout << experimental::f() << std::endl;
	}
	else if (opts.basicMessage) {
		std::string buffer;
		std::cin >> buffer;

		auto pServer = experimental::IServer::Make(std::move(buffer));

		// send goahead signal to client
		std::cout << "go" << std::endl;
		
		// wait until client has finished receiving
		std::cin >> buffer;
	}
	else {
		std::cout << "default-output" << std::endl;
	}

	return 0;
}
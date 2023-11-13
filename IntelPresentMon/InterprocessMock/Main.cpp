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

		auto pServer = experimental::IServer::Make(buffer);
		pServer->MakeUptrToMessage(buffer);

		// send goahead signal to client
		std::cout << "go" << std::endl;
		
		// wait until client has finished receiving
		std::cin >> buffer;
	}
	else if (opts.destroyUptr) {
		std::string buffer;

		// waiting for client that contains input code
		std::cin >> buffer;

		auto pServer = experimental::IServer::Make(buffer);
		pServer->MakeUptrToMessage(buffer);

		// send goahead signal to client, checks the free memory
		std::cout << "go" << std::endl;

		// wait until client has finishing checking free memory
		std::cin >> buffer;

		pServer->FreeUptrToMessage();

		// send goahead signal to client to check free memory again
		std::cout << "go" << std::endl;

		// wait until client has finishing checking free memory
		std::cin >> buffer;
	}
	else if (opts.makeDestroyUptr) {
		std::string buffer;

		auto pServer = experimental::IServer::Make("dummy");

		// signal to client that shm has been created
		std::cout << "go" << std::endl;

		// wait for client signal that free memory has been checked, contains code string
		std::cin >> buffer;

		// create the uptr and string in memory
		pServer->MakeUptrToMessage(buffer);

		// send goahead signal to client, checks the free memory
		std::cout << "go" << std::endl;

		// wait until client has finishing checking free memory
		std::cin >> buffer;

		pServer->FreeUptrToMessage();

		// send goahead signal to client to check free memory again
		std::cout << "go" << std::endl;

		// wait until client has finishing checking free memory
		std::cin >> buffer;
	}
	else {
		std::cout << "default-output" << std::endl;
	}

	return 0;
}
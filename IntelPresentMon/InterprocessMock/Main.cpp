#include <iostream>
#include "../Interprocess/source/Interprocess.h"
#include "Options.h"

int main(int argc, char** argv)
{
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
		std::cout << pmon::ipc::f() << std::endl;
	}
	else if (opts.basicMessage) {

	}
	else {
		std::cout << "default-output" << std::endl;
	}

	return 0;
}
#pragma once
#include "../CommonUtilities/source/cli/CliFramework.h"

namespace clio
{
	using namespace pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
		Option<std::string> controlPipe{ this, "--control-pipe", "", "Name of the named pipe to use for the client-service control channel" };
		Option<std::string> introNsm{ this, "--intro-nsm", "", "Name of the NSM used for introspection data" };
		static constexpr const char* description = "Minimal Sample Client for Intel PresentMon service";
		static constexpr const char* name = "SampleClient.exe";
	};
}
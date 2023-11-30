#pragma once
#include "../CommonUtilities/source/cli/CliFramework.h"

namespace clio
{
	using namespace pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
		Option<int> testIntOption{ this, "--test-int-option", -1, "Test passing info via the command line" };
		static constexpr const char* description = "Intel PresentMon service for frame and system performance measurement";
		static constexpr const char* name = "PresentMonService.exe";
	};
}
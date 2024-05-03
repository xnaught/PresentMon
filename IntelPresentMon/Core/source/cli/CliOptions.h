#pragma once
#include <CommonUtilities/cli/CliFramework.h>
#include <CommonUtilities/log/Level.h>
#include <format>

namespace p2c::cli
{
	using namespace ::pmon::util;
	using namespace ::pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
		// add options and switches here to augment the CLI
		Flag verbose{ this, "--p2c-verbose", "Enable verbose logging in release" };
		Flag filesWorking{ this, "--p2c-files-working", "Use the working directory for file storage" };
		Flag logBlack{ this, "--p2c-log-black", "Use blacklist to exclude source lines from logging" };
		Flag noNetFail{ this, "--p2c-no-net-fail", "Disable error modal for bad url accesses" };
		Flag allowTearing{ this, "--p2c-allow-tearing", "Allow tearing presents for overlay (optional, might affect VRR)" };
		Option<std::string> url{ this, "--p2c-url", "", "URL to load instead of app files" };
		Option<std::string> controlPipe{ this, "--p2c-control-pipe", "", "Named pipe to connect to the service with" };
		Option<std::string> shmName{ this, "--p2c-shm-name", "", "Shared memory to connect to the service with" };
		Option<std::string> cefType{ this, "--type", "", "Type of the current chromium process" };

		static constexpr const char* description = "PresentMon performance overlay and trace capture application";
		static constexpr const char* name = "PresentMon.exe";

	private:
		NoForward noForward_{ cefType };
		AllowExtras ext_{ this };
	};
}
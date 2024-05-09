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
	private:
		CLI::CheckedTransformer logLevelTf_{ log::GetLevelMapNarrow(), CLI::ignore_case };

	public:
		// add options and switches here to augment the CLI
		Option<std::string> controlPipe{ this, "--p2c-control-pipe", "", "Named pipe to connect to the service with" };
		Option<std::string> shmName{ this, "--p2c-shm-name", "", "Shared memory to connect to the service with" };
		Flag allowTearing{ this, "--p2c-allow-tearing", "Allow tearing presents for overlay (optional, might affect VRR)" };
		Option<std::string> cefType{ this, "--type", "", "Type of the current chromium process" };

		Option<std::string> url{ this, "--p2c-url", "", "URL to load instead of app files" };
		Flag noNetFail{ this, "--p2c-no-net-fail", "Disable error modal for bad url accesses" };
		Flag debugWaitRender{ this, "--p2c-debug-wait-render", "Force all render child processes wait for debugger connection" };
		Flag filesWorking{ this, "--p2c-files-working", "Use the working directory for file storage" };
		Option<std::string> logPipeName{ this, "--p2c-log-pipe-name", "", "The postfix used to create the named pipe for logging source server" };
		Option<log::Level> logLevel{ this, "--p2c-log-level", log::Level::Error, "Severity to log at", logLevelTf_ };
		Option<log::Level> logTraceLevel{ this, "--p2c-log-trace-level", log::Level::Error, "Severity to print stacktrace at", logLevelTf_ };
		Flag traceExceptions{ this, "--p2c-trace-exceptions", "Add stack trace to all thrown exceptions (including SEH exceptions)" };
		Option<std::string> logDenyList{ this, "--p2c-log-deny-list", "", "Path to log deny list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logAllowList{ this, "--p2c-log-allow-list", "", "Path to log allow list (with trace overrides)", CLI::ExistingFile };

		static constexpr const char* description = "PresentMon performance overlay and trace capture application";
		static constexpr const char* name = "PresentMon.exe";

	private:
		MutualExclusion excl_{ logDenyList, logAllowList };
		NoForward noForward_{ cefType, logPipeName };
		AllowExtras ext_{ this };
	};
}
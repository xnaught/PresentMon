#pragma once
#include "../CommonUtilities/source/cli/CliFramework.h"

namespace clio
{
	using namespace pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
		Option<std::string> controlPipe{ this, "--control-pipe", "", "Name of the named pipe to use for the client-service control channel" };
		Option<std::string> introNsm{ this, "--intro-nsm", "", "Name of the NSM used for introspection data" };
		Option<std::string> viewMetrics{ this, "--view-metrics", "", "View available system metrics via introspection" };
		Option<std::string> pollMetrics{ this, "--poll-metrics", "", "Poll frame metrics" };
		Option<std::string> recordFrames{ this, "--record-frames", "", "Record raw frame data frames" };
		Option<double> metricOffset{ this, "--record-frames", 200., "Record raw frame data frames" };
		Option<double> windowSize{ this, "--window-size", 2000., "Record raw frame data frames" };
		Option<int> processId{ this, "--process-id", 0, "Process Id to use for polling or frame data capture" };
		Option<std::string> appName{ this, "--app-name", "", "Name of application to use for polling or frame data capture" };
		static constexpr const char* description = "Minimal Sample Client for Intel PresentMon service";
		static constexpr const char* name = "SampleClient.exe";
	};
}
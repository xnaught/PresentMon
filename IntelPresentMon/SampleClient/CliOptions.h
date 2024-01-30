#pragma once
#include "../CommonUtilities/source/cli/CliFramework.h"

namespace clio
{
	using namespace pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
		Option<std::string> controlPipe{ this, "--control-pipe", "", "Name of the named pipe to use for the client-service control channel" };
		Option<std::string> introNsm{ this, "--intro-nsm", "", "Name of the NSM used for introspection data" };
		Flag viewAvailableMetrics{ this, "--view-available-metrics", "Example of how to view available system metrics via introspection" };
		Flag pollMetrics{ this, "--poll-metrics", "Example of how to poll frame and gpu metrics" };
		Flag recordFrames{ this, "--record-frames", "Example of how to record raw frame data frames" };
		Option<double> metricOffset{ this, "--metric-offset", 1000., "Record raw frame data frames" };
		Option<double> windowSize{ this, "--window-size", 2000., "Record raw frame data frames" };
		Option<unsigned int> processId{ this, "--process-id", 0, "Process Id to use for polling or frame data capture" };
		Option<std::string> processName{ this, "--process-name", "", "Name of process to use for polling or frame data capture" };
		static constexpr const char* description = "Minimal Sample Client for Intel PresentMon service";
		static constexpr const char* name = "SampleClient.exe";
	};
}
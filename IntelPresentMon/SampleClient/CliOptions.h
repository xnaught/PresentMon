#pragma once
#include "../CommonUtilities/source/cli/CliFramework.h"

namespace clio
{
	using namespace pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
		Option<std::string> controlPipe{ this, "--control-pipe", "", "Name of the named pipe to use for the client-service control channel" };
		Option<std::string> introNsm{ this, "--intro-nsm", "", "Name of the NSM used for introspection data" };
		Flag wrapper{ this, "--wrapper", "Execute wrapper code" };
		Flag dynamic{ this, "--dynamic", "Execute dynamic polling test in wrapper (otherwise do frame consume)" };
		Flag introspectionSample{ this, "--introspection-sample", "Example of how to view available system metrics via introspection" };
		Flag dynamicQuerySample{ this, "--dynamic-query-sample", "Example of how to use a dynamic query to poll metric data" };
		Flag frameQuerySample{ this, "--frame-query-sample", "Example of how to use a frame query to consume raw frame data" };
		Option<double> metricOffset{ this, "--metric-offset", 1000., "Offset from top for frame data. Used in --dynamic-query-sample" };
		Option<double> windowSize{ this, "--window-size", 2000., "Window size used for metrics calculation. Used in --dynamic-query-sample" };
		Option<unsigned int> processId{ this, "--process-id", 0, "Process Id to use for polling or frame data capture" };
		Option<std::string> processName{ this, "--process-name", "", "Name of process to use for polling or frame data capture" };
		Option<std::string> checkMetric{ this, "--check-metric", "", "Check if passed in PM_METRIC is available on the system" };
		static constexpr const char* description = "Minimal Sample Client for Intel PresentMon service";
		static constexpr const char* name = "SampleClient.exe";
	};
}
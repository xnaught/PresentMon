#pragma once
#include "../IntelPresentMon/CommonUtilities/cli/CliFramework.h"

namespace clio
{
	using namespace pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
		Option<std::string> inputFile{ this, "--input-file", "", "Path to input file to use" };
		Option<std::string> outputFile{ this, "--output-file", "", "Path for the trimmed output file" };
		Flag provider{ this, "--provider,-p", "Enable pruning by provider (uses same provider set as PresentMon)" };
		Flag event{ this, "--event,-e", "Enable pruning by event (uses same event set as PresentMon)" };
		Flag keyword{ this, "--keyword,-k", "Enable pruning by keyword (uses same keyword set as PresentMon)" };
		Flag level{ this, "--level,-l", "Enable pruning by level (uses same level set as PresentMon)" };
		Option<std::pair<uint64_t, uint64_t>> trimRange{ this, "--trim-range", {}, "Range of timestamps outside of which to trim" };

		static constexpr const char* description = "Postprocessing tool for trimming and pruning ETL files";
		static constexpr const char* name = "ETLTrimmer.exe";
	};
}
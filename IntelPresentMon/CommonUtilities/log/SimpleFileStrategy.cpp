#include "SimpleFileStrategy.h"

namespace pmon::util::log
{
	SimpleFileStrategy::SimpleFileStrategy(std::filesystem::path path)
	{
		// create any directories in the path that don't yet exist
		if (const auto dirs = path.parent_path(); !dirs.empty()) {
			std::filesystem::create_directories(dirs);
		}
		// open file, append if already exists
		pFileStream_ = std::make_shared<std::wofstream>(path, std::ios::out | std::ios::app);
	}
	std::shared_ptr<std::wostream> SimpleFileStrategy::AddLine()
	{
		return pFileStream_;
	}
	std::shared_ptr<std::wostream> SimpleFileStrategy::GetFileStream() const
	{
		return pFileStream_;
	}
	void SimpleFileStrategy::Cleanup()
	{}
}
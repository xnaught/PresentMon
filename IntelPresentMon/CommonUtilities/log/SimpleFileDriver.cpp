#include "SimpleFileDriver.h"
#include "ITextFormatter.h"

namespace pmon::util::log
{
	SimpleFileDriver::SimpleFileDriver(std::filesystem::path path, std::shared_ptr<ITextFormatter> pFormatter)
		:
		pFormatter_{ std::move(pFormatter) }
	{
		// create any directories in the path that don't yet exist
		if (const auto dirs = path.parent_path(); !dirs.empty()) {
			std::filesystem::create_directories(dirs);
		}
		// open file, append if already exists 
		file_.open(path, file_.out | file_.app);
	}
	void SimpleFileDriver::Submit(const Entry& e)
	{
		if (pFormatter_) {
			file_ << pFormatter_->Format(e).c_str();
		}
		// TODO: how to log stuff from log system 
	}
	void SimpleFileDriver::SetFormatter(std::shared_ptr<ITextFormatter> pFormatter)
	{
		pFormatter_ = std::move(pFormatter);
	}
	void SimpleFileDriver::Flush()
	{
		file_.flush();
	}
}
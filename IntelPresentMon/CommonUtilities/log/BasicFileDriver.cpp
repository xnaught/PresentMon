#include "BasicFileDriver.h"
#include "ITextFormatter.h"
#include <cassert>

namespace pmon::util::log
{
	BasicFileDriver::BasicFileDriver(
		std::shared_ptr<ITextFormatter> pFormatter,
		std::shared_ptr<IFileStrategy> pFileStrategy)
		:
		pFormatter_{ std::move(pFormatter) },
		pFileStrategy_{ std::move(pFileStrategy) }
	{}
	void BasicFileDriver::Submit(const Entry& e)
	{
		if (pFormatter_ && pFileStrategy_) {
			auto pFile = pFileStrategy_->AddLine();
			*pFile << pFormatter_->Format(e);
		}
		// TODO: how to log stuff from log system 
	}
	void BasicFileDriver::SetFormatter(std::shared_ptr<ITextFormatter> pFormatter)
	{
		pFormatter_ = std::move(pFormatter);
	}
	void BasicFileDriver::SetFileStrategy(std::shared_ptr<IFileStrategy> pFileStrategy)
	{
		pFileStrategy_ = pFileStrategy;
	}
	void BasicFileDriver::Flush()
	{
		if (pFileStrategy_) {
			pFileStrategy_->GetFileStream()->flush();
		}
	}
}
#pragma once
#include "IDriver.h"
#include <filesystem>
#include <memory>
#include <fstream>

namespace pmon::util::log
{
	class SimpleFileDriver : public ITextDriver
	{
	public:
		SimpleFileDriver(std::filesystem::path path, std::shared_ptr<ITextFormatter> pFormatter = {});
		void Submit(const Entry&) override;
		void SetFormatter(std::shared_ptr<ITextFormatter> pFormatter) override;
		void Flush() override;
	private:
		std::wofstream file_;
		std::shared_ptr<ITextFormatter> pFormatter_;
	};
}


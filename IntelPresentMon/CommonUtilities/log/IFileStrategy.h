#pragma once
#include <filesystem>
#include <ostream>
#include <memory>

namespace pmon::util::log
{
	class IFileStrategy
	{
	public:
		virtual std::shared_ptr<std::wostream> AddLine() = 0;
		virtual std::shared_ptr<std::wostream> GetFileStream() const = 0;
		virtual void Cleanup() = 0;
	};
}


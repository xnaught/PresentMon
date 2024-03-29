#pragma once
#include <string>
#include <vector>
#include <stacktrace>
#include <span>

namespace pmon::util::log
{
	class StackTrace
	{
	public:
		// types
		struct FrameInfo
		{
			std::wstring description;
			std::wstring file;
			int line;
			int index;
		};
		// functions
		StackTrace(std::stacktrace = std::stacktrace::current());
		void Resolve();
		std::span<const FrameInfo> GetFrames() const;
	private:
		std::stacktrace trace_;
		std::vector<FrameInfo> frames_;
	};
}
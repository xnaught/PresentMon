#pragma once
#include <string>
#include <vector>
#include <stacktrace>
#include <span>
#include <memory>

namespace pmon::util::log
{
	class StackTrace
	{
		friend struct StackTraceCereal;
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
		StackTrace() = default;
		StackTrace(std::stacktrace trace);
		void Resolve();
		std::span<const FrameInfo> GetFrames() const;
		static std::unique_ptr<StackTrace> Here();
	private:
		std::stacktrace trace_;
		std::vector<FrameInfo> frames_;
	};
}
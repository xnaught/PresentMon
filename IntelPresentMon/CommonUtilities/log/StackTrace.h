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
		StackTrace(const StackTrace& other);

		StackTrace& operator=(const StackTrace&) = delete;
		~StackTrace() = default;

		void Resolve();
		std::span<const FrameInfo> GetFrames() const;
		bool Empty() const;
		bool Resolved() const;
		std::wstring ToString() const;
		static std::unique_ptr<StackTrace> Here(size_t skip = 0);
	private:
		std::stacktrace trace_;
		std::vector<FrameInfo> frames_;
	};
}
#pragma once
#include <atomic>
#include "Level.h"

namespace pmon::util::log
{
	class GlobalPolicy
	{
	public:
		static Level GetLogLevel() noexcept;
		static void SetLogLevel(Level level) noexcept;
		static bool GetResolveTraceInClientThread() noexcept;
		static void SetResolveTraceInClientThread(bool setting) noexcept;
	private:
		// functions
		GlobalPolicy() noexcept;
		static GlobalPolicy& Get_() noexcept;
		Level GetLogLevel_() const noexcept;
		void SetLogLevel_(Level level) noexcept;
		bool GetResolveTraceInClientThread_() const noexcept;
		void SetResolveTraceInClientThread_(bool setting) noexcept;
		// data
		std::atomic<Level> logLevel_;
		std::atomic<bool> resolveTraceInClientThread_ = false;
	};
}
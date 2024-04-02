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
	private:
		// functions
		GlobalPolicy() noexcept;
		static GlobalPolicy& Get_() noexcept;
		Level GetLogLevel_() const noexcept;
		void SetLogLevel_(Level level) noexcept;
		// data
		std::atomic<Level> logLevel_;
	};
}
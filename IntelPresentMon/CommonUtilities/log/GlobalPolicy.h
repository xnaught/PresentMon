#pragma once
#include <atomic>
#include "Level.h"

namespace pmon::util::log
{
	class GlobalPolicy
	{
	public:
		GlobalPolicy() noexcept;
		Level GetLogLevel() const noexcept;
		void SetLogLevel(Level level) noexcept;
	private:
		std::atomic<Level> logLevel_;
	};
	extern GlobalPolicy globalPolicy;
}
#pragma once
#include <string>

#ifndef NDEBUG
#define pmlog_panic_ PMLogPanic_
#else
#define pmlog_panic_
#endif

namespace pmon::util::log
{
	void PMLogPanic_(const std::string& msg) noexcept;
}


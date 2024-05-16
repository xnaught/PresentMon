#pragma once
#include <cstdint>
#include "Log.h"

namespace pmon::util::log
{
	class HrLogger
	{
	public:
		HrLogger(const wchar_t*, const wchar_t*, int) noexcept;
		void operator<<(uint32_t hr) const;
	private:
		const wchar_t* sourceFile_ = nullptr;
		const wchar_t* sourceFunctionName_ = nullptr;
		int sourceLine_ = 0;
	};
}

#define pmlog_hr ::pmon::util::log::HrLogger{ __FILEW__, __FUNCTIONW__, __LINE__ }
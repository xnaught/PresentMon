#pragma once
#include "str/String.h"
#include <sstream>
#include "log/GlobalPolicy.h"
#include "log/StackTrace.h"

#ifndef PM_THROW_SKIP
#ifndef NDEBUG
#define PM_THROW_SKIP 3
#else
#define PM_THROW_SKIP 2
#endif
#endif

namespace pmon::util
{
	class Exception : public std::exception
	{
	public:
		Exception(std::string msg) : note_{ std::move(msg) } {}
		void CaptureStackTrace()
		{
			pTrace_ = log::StackTrace::Here(PM_THROW_SKIP);
		}
		const char* what() const override
		{
			buffer_ = GetNote_();
			if (pTrace_) {
				buffer_ += "\n" + GetTraceString_();
			}
			return buffer_.c_str();
		}
	protected:
		const std::string& GetNote_() const
		{
			return note_;
		}
		std::string GetTraceString_() const
		{
			if (pTrace_) {
				pTrace_->Resolve();
				std::wostringstream oss;
				oss << L" ====== STACK TRACE (newest on top) ======\n";
				oss << pTrace_->ToString();
				oss << L" =========================================\n";
				return str::ToNarrow(oss.str());
			}
			return {};
		}
	private:
		std::string note_;
		mutable std::string buffer_;
		std::unique_ptr<log::StackTrace> pTrace_;
	};

	template<class E, typename...R>
	auto Except(R&&...args)
	{
		E exception{ std::forward<R>(args)... };
		if (log::GlobalPolicy::GetExceptionTracePolicy() == log::ExceptionTracePolicy::OverrideOn) {
			exception.CaptureStackTrace();
		}
		return exception;
	}
}
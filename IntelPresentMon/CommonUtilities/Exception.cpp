#include "Exception.h"


namespace pmon::util
{
	Exception::Exception(std::string msg) noexcept : note_{ std::move(msg) } {}
	void Exception::CaptureStackTrace()
	{
		pTrace_ = log::StackTrace::Here(PM_THROW_SKIP);
	}
	const char* Exception::what() const noexcept
	{
		if (buffer_.empty()) {
			buffer_ = ComposeWhatString_();
		}
		return buffer_.c_str();
	}
	std::string Exception::ComposeWhatString_() const noexcept
	{
		try {
			std::ostringstream oss;
			oss << GetNote_();
			if (pTrace_) {
				oss << "\n" << GetTraceString_();
			}
			return oss.str();
		}
		catch (...) {}
		return {};
	}
	const std::string& Exception::GetNote_() const
	{
		return note_;
	}
	std::string Exception::GetTraceString_() const
	{
		if (HasTrace_()) {
			pTrace_->Resolve();
			std::wostringstream oss;
			oss << L" ====== STACK TRACE (newest on top) ======\n";
			oss << pTrace_->ToString();
			oss << L" =========================================\n";
			return str::ToNarrow(oss.str());
		}
		return {};
	}
	bool Exception::HasTrace_() const noexcept
	{
		return bool(pTrace_);
	}

	void DoCapture_(Exception& e)
	{
		if (log::GlobalPolicy::GetExceptionTracePolicy() == log::ExceptionTracePolicy::OverrideOn) {
			e.CaptureStackTrace();
		}
	}

	std::string ReportException(std::exception_ptr pEx) noexcept
	{
		if (!pEx) {
			pEx = std::current_exception();
		}
		if (pEx) {
			try {
				std::rethrow_exception(pEx);
			}
			catch (const std::exception& e) {
				return std::format("[{}]\n{}", typeid(e).name(), e.what());
			}
			catch (...) {
				return "Unrecognized exception";
			}
		}
		return "No exception in flight";
	}
}
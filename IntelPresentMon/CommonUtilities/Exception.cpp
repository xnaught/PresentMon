#include "Exception.h"
#include "../CommonUtilities/win/Utilities.h"
#include <sstream>
#include "log/GlobalPolicy.h"
#include "log/StackTrace.h"
#include <format>


#ifndef PM_THROW_SKIP
#ifndef NDEBUG
#define PM_THROW_SKIP 3
#else
#define PM_THROW_SKIP 2
#endif
#endif

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
			oss << GetNote();
			if (pTrace_) {
				oss << "\n" << GetTraceString();
			}
			return oss.str();
		}
		catch (...) {}
		return {};
	}
	const std::string& Exception::GetNote() const
	{
		return note_;
	}
	std::string Exception::GetTraceString() const
	{
		if (HasTrace()) {
			pTrace_->Resolve();
			std::wostringstream oss;
			oss << L" ====== STACK TRACE (newest on top) ======\n";
			oss << pTrace_->ToString();
			oss << L" =========================================\n";
			return str::ToNarrow(oss.str());
		}
		return {};
	}
	bool Exception::HasTrace() const noexcept
	{
		return bool(pTrace_);
	}

	void DoCapture_(Exception& e)
	{
		if (log::GlobalPolicy::Get().GetExceptionTrace()) {
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
				try { return std::format("[{}] {}", typeid(e).name(), e.what()); }
				catch (...) { return {}; }
			}
			catch (...) {
				return "Unrecognized exception";
			}
		}
		return "No exception in flight";
	}

	std::wstring ReportExceptionWide(std::exception_ptr pEx) noexcept
	{
		return str::ToWide(ReportException(std::move(pEx)));
	}

	namespace {
		void seh_trans_func(unsigned int u, EXCEPTION_POINTERS*)
		{
			SehException ex{ u };
			if (log::GlobalPolicy::Get().GetSehTracing()) {
				ex.CaptureStackTrace();
			}
			throw ex;
		}
	}

	void InstallSehTranslator() noexcept
	{
		_set_se_translator(seh_trans_func);
	}

	SehException::SehException(unsigned int code) noexcept : sehCode{ code } {}

	unsigned int SehException::GetSehCode() const noexcept { return sehCode; }

	std::string SehException::ComposeWhatString_() const noexcept
	{
		try {
			std::ostringstream oss;
			oss << std::format("Error Code [0x{:08X}]: {}\n", GetSehCode(),
				str::ToNarrow(win::GetSEHSymbol(GetSehCode())));
			if (HasTrace()) {
				oss << "\n" << GetTraceString();
			}
			return oss.str();
		}
		catch (...) {}
		return {};
	}
}
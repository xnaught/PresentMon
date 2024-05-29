#pragma once
#include "str/String.h"
#include "log/StackTrace.h"
#include <exception>

namespace pmon::util
{
	class Exception : public std::exception
	{
	public:
		Exception() noexcept = default;
		Exception(std::string msg) noexcept;
		void CaptureStackTrace();
		const char* what() const noexcept override;
		const std::string& GetNote() const;
		std::string GetTraceString() const;
		bool HasTrace() const noexcept;
	protected:
		virtual std::string ComposeWhatString_() const noexcept;
	private:
		std::string note_;
		mutable std::string buffer_;
		std::shared_ptr<log::StackTrace> pTrace_;
	};

	void DoCapture_(Exception& e);

	template<class E, typename...R>
	auto Except(R&&...args)
	{
		E exception{ std::forward<R>(args)... };
		DoCapture_(exception);
		return exception;
	}

	std::string ReportException(std::exception_ptr pEx = {}) noexcept;
	std::wstring ReportExceptionWide(std::exception_ptr pEx = {}) noexcept;

	class SehException : public Exception
	{
	public:
		SehException() noexcept = default;
		SehException(unsigned int code) noexcept;
		unsigned int GetSehCode() const noexcept;
	protected:
		std::string ComposeWhatString_() const noexcept override;
	private:
		unsigned int sehCode = 0;
	};

	void InstallSehTranslator() noexcept;

#define PM_DEFINE_EX(name) class name : public ::pmon::util::Exception { public: using Exception::Exception; }

// prevent any exceptions from leaking from this statement
#define pmquell(stat) try { stat; } catch (...) {}

#define pmcatch_report catch (...) { pmlog_error(::pmon::util::ReportExceptionWide()); }
}
#pragma once
#include "str/String.h"
#include <sstream>
#include "log/GlobalPolicy.h"
#include "log/StackTrace.h"
#include <exception>
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
	class Exception : public std::exception
	{
	public:
		Exception() noexcept = default;
		Exception(std::string msg) noexcept;
		void CaptureStackTrace();
		const char* what() const noexcept override;
	protected:
		virtual std::string ComposeWhatString_() const noexcept;
		const std::string& GetNote_() const;
		std::string GetTraceString_() const;
		bool HasTrace_() const noexcept;
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

#define PM_DEFINE_EX(name) class name : public Exception { public: using Exception::Exception; }
}
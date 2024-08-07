#pragma once
#include "../../../CommonUtilities/Exception.h"
#include "../../../PresentMonAPI2/PresentMonAPI.h"
#include <sstream>


namespace pmon::ipc::act
{
	class ActionResponseError : public ::pmon::util::Exception {
	public:
		ActionResponseError(PM_STATUS code) : Exception{ "Error response to dispatched Action" }, code_{ code } {}
		std::string ComposeWhatString_() const noexcept override
		{
			try {
				std::ostringstream oss;
				oss << GetNote() << " CODE:[ " << code_ << "]";
				if (HasTrace()) {
					oss << "\n" << GetTraceString();
				}
				return oss.str();
			}
			catch (...) {}
			return {};
		}
		PM_STATUS GetCode() const noexcept { return code_; }
	private:
		PM_STATUS code_;
	};
}
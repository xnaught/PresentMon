#include "PresentMonAPI.h"
#include <memory>
#include "../../PresentMonMiddleware/source/MockMiddleware.h"
#include "../../PresentMonMiddleware/source/ConcreteMiddleware.h"


using namespace pmid;

// global state
bool useMockedMiddleware_ = false;
std::unique_ptr<Middleware> pMiddleware_;


PRESENTMON_API_EXPORT void pmSetMiddlewareAsMock_(bool mocked)
{
	useMockedMiddleware_ = mocked;
}

PRESENTMON_API_EXPORT PM_STATUS pmMiddlewareSpeak_(char* buffer)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		pMiddleware_->Speak(buffer);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API_EXPORT PM_STATUS pmOpenSession()
{
	try {
		if (useMockedMiddleware_) {
			pMiddleware_ = std::make_unique<MockMiddleware>();
		}
		else {
			pMiddleware_ = std::make_unique<ConcreteMiddleware>();
		}
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API_EXPORT PM_STATUS pmCloseSession()
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		pMiddleware_.reset();
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}
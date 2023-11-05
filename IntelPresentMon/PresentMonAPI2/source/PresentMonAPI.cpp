#include "PresentMonAPI.h"
#include <memory>
#include <crtdbg.h>
#include "../../PresentMonMiddleware/source/MockMiddleware.h"
#include "../../PresentMonMiddleware/source/ConcreteMiddleware.h"


using namespace pmid;

// global state
bool useMockedMiddleware_ = false;
bool useCrtHeapDebug_ = false;
std::unique_ptr<Middleware> pMiddleware_;

// private implementation functions


// private endpoints
PRESENTMON_API_EXPORT void pmSetMiddlewareAsMock_(bool mocked, bool activateCrtHeapDebug)
{
	useMockedMiddleware_ = mocked;
	useCrtHeapDebug_ = activateCrtHeapDebug;
	if (useCrtHeapDebug_) {
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
	}
	else {
		_CrtSetDbgFlag(0);
	}
}

PRESENTMON_API_EXPORT _CrtMemState pmCreateHeapCheckpoint_()
{
	_CrtMemState s;
	_CrtMemCheckpoint(&s);
	return s;
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

// public endpoints
PRESENTMON_API_EXPORT PM_STATUS pmOpenSession()
{
	if (pMiddleware_) {
		return PM_STATUS_FAILURE;
	}
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

PRESENTMON_API_EXPORT PM_STATUS pmEnumerateInterface(const PM_INTROSPECTION_ROOT** ppInterface)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		if (!ppInterface) {
			return PM_STATUS_FAILURE;
		}
		*ppInterface = pMiddleware_->GetIntrospectionData();
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API_EXPORT PM_STATUS pmFreeInterface(const PM_INTROSPECTION_ROOT* pInterface)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		if (!pInterface) {
			return PM_STATUS_FAILURE;
		}
		pMiddleware_->FreeIntrospectionData(pInterface);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

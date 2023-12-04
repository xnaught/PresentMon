#include "../../PresentMonAPI/PresentMonAPI.h"
#include <memory>
#include <crtdbg.h>
#include "../../PresentMonMiddleware/source/MockMiddleware.h"
#include "../../PresentMonMiddleware/source/ConcreteMiddleware.h"
#include "Internal.h"
#include "PresentMonAPI.h"


using namespace pmon::mid;

// global state
bool useMockedMiddleware_ = false;
bool useCrtHeapDebug_ = false;
bool useLocalShmServer_ = false;
std::unique_ptr<Middleware> pMiddleware_;

// private implementation functions


// private endpoints
PRESENTMON_API2_EXPORT void pmSetMiddlewareAsMock_(bool mocked, bool activateCrtHeapDebug, bool useLocalShmServer)
{
	useMockedMiddleware_ = mocked;
	useCrtHeapDebug_ = activateCrtHeapDebug;
	useLocalShmServer_ = useLocalShmServer;
	if (useCrtHeapDebug_) {
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
	}
	else {
		_CrtSetDbgFlag(0);
	}
}

PRESENTMON_API2_EXPORT _CrtMemState pmCreateHeapCheckpoint_()
{
	_CrtMemState s;
	_CrtMemCheckpoint(&s);
	return s;
}

PRESENTMON_API2_EXPORT PM_STATUS pmMiddlewareSpeak_(char* buffer)
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

PRESENTMON_API2_EXPORT PM_STATUS pmMiddlewareAdvanceTime_(uint32_t milliseconds)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		if (auto pMock = dynamic_cast<MockMiddleware*>(pMiddleware_.get())) {
			pMock->AdvanceTime(milliseconds);
		}
		else {
			return PM_STATUS_FAILURE;
		}
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API2_EXPORT PM_STATUS pmOpenSession_(const char* pipeNameOverride, const char* introNsmOverride)
{
	if (pMiddleware_) {
		return PM_STATUS_FAILURE;
	}
	try {
		if (useMockedMiddleware_) {
			pMiddleware_ = std::make_unique<MockMiddleware>(useLocalShmServer_);
		}
		else {
			std::optional<std::string> pipeName;
			std::optional<std::string> introNsm;
			if (pipeNameOverride) {
				pipeName = std::string(pipeNameOverride);
			}
			if (introNsmOverride) {
				introNsm = std::string(introNsmOverride);
			}
			pMiddleware_ = std::make_unique<ConcreteMiddleware>(std::move(pipeName), std::move(introNsm));
		}
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

// public endpoints
PRESENTMON_API2_EXPORT PM_STATUS pmOpenSession()
{
	return pmOpenSession_(nullptr, nullptr);
}

PRESENTMON_API2_EXPORT PM_STATUS pmCloseSession()
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

PRESENTMON_API2_EXPORT PM_STATUS pmStartStreaming(uint32_t processId)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		pMiddleware_->StartStreaming(processId);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API2_EXPORT PM_STATUS pmStopStreaming(uint32_t processId)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		pMiddleware_->StopStreaming(processId);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API2_EXPORT PM_STATUS pmEnumerateInterface(const PM_INTROSPECTION_ROOT** ppInterface)
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

PRESENTMON_API2_EXPORT PM_STATUS pmFreeInterface(const PM_INTROSPECTION_ROOT* pInterface)
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

PRESENTMON_API2_EXPORT PM_STATUS pmRegisterDynamicQuery(PM_DYNAMIC_QUERY_HANDLE* pHandle, PM_QUERY_ELEMENT* pElements, uint64_t numElements, uint32_t processId, double windowSizeMs, double metricOffsetMs)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		if (!pHandle || !pElements || !numElements) {
			return PM_STATUS_FAILURE;
		}
		*pHandle = pMiddleware_->RegisterDynamicQuery({pElements, numElements}, processId, windowSizeMs, metricOffsetMs);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API2_EXPORT PM_STATUS pmFreeDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		if (!handle) {
			return PM_STATUS_FAILURE;
		}
		pMiddleware_->FreeDynamicQuery(handle);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API2_EXPORT PM_STATUS pmPollDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle, uint8_t* pBlob, uint32_t* numSwapChains)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		if (!handle || !pBlob) {
			return PM_STATUS_FAILURE;
		}
		pMiddleware_->PollDynamicQuery(handle, pBlob);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

PRESENTMON_API2_EXPORT PM_STATUS pmPollStaticQuery(const PM_QUERY_ELEMENT* pElement, uint8_t* pBlob)
{
	try {
		if (!pMiddleware_) {
			return PM_STATUS_SESSION_NOT_OPEN;
		}
		if (!pElement || !pBlob) {
			return PM_STATUS_FAILURE;
		}
		pMiddleware_->PollStaticQuery(*pElement, pBlob);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		return PM_STATUS_FAILURE;
	}
}

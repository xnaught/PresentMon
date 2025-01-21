#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPI2/PresentMonDiagnostics.h"
#include "../CommonUtilities/win/WinAPI.h"
#include <functional>
#include <vector>
#include <cassert>
#include <type_traits>


// pointers to runtime-resolved functions
PM_STATUS(*pFunc_pmOpenSession_)(PM_SESSION_HANDLE*) = nullptr;
PM_STATUS(*pFunc_pmCloseSession_)(PM_SESSION_HANDLE) = nullptr;
PM_STATUS(*pFunc_pmStartTrackingProcess_)(PM_SESSION_HANDLE, uint32_t) = nullptr;
PM_STATUS(*pFunc_pmStopTrackingProcess_)(PM_SESSION_HANDLE, uint32_t) = nullptr;
PM_STATUS(*pFunc_pmGetIntrospectionRoot_)(PM_SESSION_HANDLE, const PM_INTROSPECTION_ROOT**) = nullptr;
PM_STATUS(*pFunc_pmFreeIntrospectionRoot_)(const PM_INTROSPECTION_ROOT*) = nullptr;
PM_STATUS(*pFunc_pmSetTelemetryPollingPeriod_)(PM_SESSION_HANDLE, uint32_t, uint32_t) = nullptr;
PM_STATUS(*pFunc_pmSetEtwFlushPeriod_)(PM_SESSION_HANDLE, uint32_t) = nullptr;
PM_STATUS(*pFunc_pmRegisterDynamicQuery_)(PM_SESSION_HANDLE, PM_DYNAMIC_QUERY_HANDLE*, PM_QUERY_ELEMENT*, uint64_t, double, double) = nullptr;
PM_STATUS(*pFunc_pmFreeDynamicQuery_)(PM_DYNAMIC_QUERY_HANDLE) = nullptr;
PM_STATUS(*pFunc_pmPollDynamicQuery_)(PM_DYNAMIC_QUERY_HANDLE, uint32_t, uint8_t*, uint32_t*) = nullptr;
PM_STATUS(*pFunc_pmPollStaticQuery_)(PM_SESSION_HANDLE, const PM_QUERY_ELEMENT*, uint32_t, uint8_t*) = nullptr;
PM_STATUS(*pFunc_pmRegisterFrameQuery_)(PM_SESSION_HANDLE, PM_FRAME_QUERY_HANDLE*, PM_QUERY_ELEMENT*, uint64_t, uint32_t*) = nullptr;
PM_STATUS(*pFunc_pmConsumeFrames_)(PM_FRAME_QUERY_HANDLE, uint32_t, uint8_t*, uint32_t*) = nullptr;
PM_STATUS(*pFunc_pmFreeFrameQuery_)(PM_FRAME_QUERY_HANDLE) = nullptr;
// pointers to runtime-resolved diagnostic functions
PM_STATUS(*pFunc_pmDiagnosticSetup_)(const PM_DIAGNOSTIC_CONFIGURATION*) = nullptr;
uint32_t(*pFunc_pmDiagnosticGetQueuedMessageCount_)() = nullptr;
PM_STATUS(*pFunc_pmDiagnosticSetMaxQueuedMessages_)(uint32_t) = nullptr;
PM_STATUS(*pFunc_pmDiagnosticGetMaxQueuedMessages_)() = nullptr;
uint32_t(*pFunc_pmDiagnosticGetDiscardedMessageCount_)() = nullptr;
PM_STATUS(*pFunc_pmDiagnosticDequeueMessage_)(PM_DIAGNOSTIC_MESSAGE**) = nullptr;
PM_STATUS(*pFunc_pmDiagnosticEnqueueMessage_)(const PM_DIAGNOSTIC_MESSAGE*) = nullptr;
PM_STATUS(*pFunc_pmDiagnosticFreeMessage_)(PM_DIAGNOSTIC_MESSAGE*) = nullptr;
PM_DIAGNOSTIC_WAKE_REASON(*pFunc_pmDiagnosticWaitForMessage_)(uint32_t) = nullptr;
PM_STATUS(*pFunc_pmDiagnosticUnblockWaitingThread_)() = nullptr;
// pointers to runtime-resolved internal functions
PM_STATUS(*pFunc_pmOpenSession__)(PM_SESSION_HANDLE* pHandle, const char*, const char*) = nullptr;
void(*pFunc_pmSetMiddlewareAsMock__)(bool, bool, bool) = nullptr;
_CrtMemState(*pFunc_pmCreateHeapCheckpoint__)() = nullptr;
PM_STATUS(*pFunc_pmMiddlewareSpeak__)(PM_SESSION_HANDLE, char*) = nullptr;
PM_STATUS(*pFunc_pmMiddlewareAdvanceTime__)(PM_SESSION_HANDLE, uint32_t) = nullptr;
LoggingSingletons(*pFunc_pmLinkLogging__)(std::shared_ptr<pmon::util::log::IChannel>,
	std::function<pmon::util::log::IdentificationTable&()>) = nullptr;
void(*pFunc_pmFlushEntryPoint__)() = nullptr;


void* GetCppProcAddress_Impl_(HMODULE h,const char* name, const char* mangledEmbeddedSignature)
{
	// don't bother including receiving template function name
	std::string Signature = mangledEmbeddedSignature + 22;
	// The signature of F appears twice (return type and template parameter type)
	size_t len = Signature.find("@@YA");
	std::string templateParam = Signature.substr(0, len);
	std::string returnType = Signature.substr(len + 4);
	returnType.resize(templateParam.size()); // Strip off our own arguments (HMODULE and const char*)
	// templateParam and returnType are _pointers_ to functions (P6), so adjust to function type (Y)
	std::string funName = "?" + std::string(name) + "@@Y" + templateParam.substr(2);
	return ::GetProcAddress(h, funName.c_str());
}

template<typename F>
F GetCppProcAddress_(HMODULE h, const char* name)
{
	return reinterpret_cast<F>(GetCppProcAddress_Impl_(h, name, __FUNCDNAME__));
}


PRESENTMON_API2_EXPORT PM_STATUS LoadLibrary_()
{
	HMODULE hMod = LoadLibraryA("PresentMonAPI2.dll");
	if (!hMod) {
		return PM_STATUS_FAILURE;
	}

#define RESOLVE(f) pFunc_##f##_ = reinterpret_cast<decltype(pFunc_##f##_)>(GetProcAddress(hMod, #f))
#define RESOLVE_CPP(f) pFunc_##f##_ = GetCppProcAddress_<decltype(pFunc_##f##_)>(hMod, #f)
	// core
	RESOLVE(pmOpenSession);
	RESOLVE(pmCloseSession);
	RESOLVE(pmStartTrackingProcess);
	RESOLVE(pmStopTrackingProcess);
	RESOLVE(pmGetIntrospectionRoot);
	RESOLVE(pmFreeIntrospectionRoot);
	RESOLVE(pmSetTelemetryPollingPeriod);
	RESOLVE(pmSetEtwFlushPeriod);
	RESOLVE(pmRegisterDynamicQuery);
	RESOLVE(pmFreeDynamicQuery);
	RESOLVE(pmPollDynamicQuery);
	RESOLVE(pmPollStaticQuery);
	RESOLVE(pmRegisterFrameQuery);
	RESOLVE(pmConsumeFrames);
	RESOLVE(pmFreeFrameQuery);
	// diagnostics
	RESOLVE(pmDiagnosticSetup);
	RESOLVE(pmDiagnosticGetQueuedMessageCount);
	RESOLVE(pmDiagnosticGetMaxQueuedMessages);
	RESOLVE(pmDiagnosticSetMaxQueuedMessages);
	RESOLVE(pmDiagnosticGetDiscardedMessageCount);
	RESOLVE(pmDiagnosticDequeueMessage);
	RESOLVE(pmDiagnosticEnqueueMessage);
	RESOLVE(pmDiagnosticFreeMessage);
	RESOLVE(pmDiagnosticWaitForMessage);
	RESOLVE(pmDiagnosticUnblockWaitingThread);
	// internal
	RESOLVE(pmOpenSession_); // !!
	RESOLVE_CPP(pmSetMiddlewareAsMock_); //
	RESOLVE_CPP(pmCreateHeapCheckpoint_); // ??
	RESOLVE_CPP(pmMiddlewareSpeak_); //
	RESOLVE_CPP(pmMiddlewareAdvanceTime_); //
	RESOLVE_CPP(pmLinkLogging_);
	RESOLVE_CPP(pmFlushEntryPoint_);

	return PM_STATUS_SUCCESS;
}

PRESENTMON_API2_EXPORT PM_STATUS pmOpenSession(PM_SESSION_HANDLE* pHandle)
{
	if (!pFunc_pmOpenSession_) {
		if (auto status = LoadLibrary_(); status != PM_STATUS_SUCCESS) {
			return status;
		}
	}
	return pFunc_pmOpenSession_(pHandle);
}
PRESENTMON_API2_EXPORT PM_STATUS pmCloseSession(PM_SESSION_HANDLE handle)
{
	assert(pFunc_pmCloseSession_);
	return pFunc_pmCloseSession_(handle);
}
PRESENTMON_API2_EXPORT PM_STATUS pmStartTrackingProcess(PM_SESSION_HANDLE handle, uint32_t process_id)
{
	assert(pFunc_pmStartTrackingProcess_);
	return pFunc_pmStartTrackingProcess_(handle, process_id);
}
PRESENTMON_API2_EXPORT PM_STATUS pmStopTrackingProcess(PM_SESSION_HANDLE handle, uint32_t process_id)
{
	assert(pFunc_pmStartTrackingProcess_);
	return pFunc_pmStartTrackingProcess_(handle, process_id);
}
PRESENTMON_API2_EXPORT PM_STATUS pmGetIntrospectionRoot(PM_SESSION_HANDLE handle, const PM_INTROSPECTION_ROOT** ppRoot)
{
	assert(pFunc_pmGetIntrospectionRoot_);
	return pFunc_pmGetIntrospectionRoot_(handle, ppRoot);
}
PRESENTMON_API2_EXPORT PM_STATUS pmFreeIntrospectionRoot(const PM_INTROSPECTION_ROOT* pRoot)
{
	assert(pFunc_pmFreeIntrospectionRoot_);
	return pFunc_pmFreeIntrospectionRoot_(pRoot);
}
PRESENTMON_API2_EXPORT PM_STATUS pmSetTelemetryPollingPeriod(PM_SESSION_HANDLE handle, uint32_t deviceId, uint32_t timeMs)
{
	assert(pFunc_pmSetTelemetryPollingPeriod_);
	return pFunc_pmSetTelemetryPollingPeriod_(handle, deviceId, timeMs);
}
PRESENTMON_API2_EXPORT PM_STATUS pmSetEtwFlushPeriod(PM_SESSION_HANDLE handle, uint32_t periodMs)
{
	assert(pFunc_pmSetEtwFlushPeriod_);
	return pFunc_pmSetEtwFlushPeriod_(handle, periodMs);
}
PRESENTMON_API2_EXPORT PM_STATUS pmRegisterDynamicQuery(PM_SESSION_HANDLE sessionHandle, PM_DYNAMIC_QUERY_HANDLE* pHandle, PM_QUERY_ELEMENT* pElements, uint64_t numElements, double windowSizeMs, double metricOffsetMs)
{
	assert(pFunc_pmRegisterDynamicQuery_);
	return pFunc_pmRegisterDynamicQuery_(sessionHandle, pHandle, pElements, numElements, windowSizeMs, metricOffsetMs);
}
PRESENTMON_API2_EXPORT PM_STATUS pmFreeDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle)
{
	assert(pFunc_pmFreeDynamicQuery_);
	return pFunc_pmFreeDynamicQuery_(handle);
}
PRESENTMON_API2_EXPORT PM_STATUS pmPollDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains)
{
	assert(pFunc_pmPollDynamicQuery_);
	return pFunc_pmPollDynamicQuery_(handle, processId, pBlob, numSwapChains);
}
PRESENTMON_API2_EXPORT PM_STATUS pmPollStaticQuery(PM_SESSION_HANDLE sessionHandle, const PM_QUERY_ELEMENT* pElement, uint32_t processId, uint8_t* pBlob)
{
	assert(pFunc_pmPollStaticQuery_);
	return pFunc_pmPollStaticQuery_(sessionHandle, pElement, processId, pBlob);
}
PRESENTMON_API2_EXPORT PM_STATUS pmRegisterFrameQuery(PM_SESSION_HANDLE sessionHandle, PM_FRAME_QUERY_HANDLE* pHandle, PM_QUERY_ELEMENT* pElements, uint64_t numElements, uint32_t* pBlobSize)
{
	assert(pFunc_pmRegisterFrameQuery_);
	return pFunc_pmRegisterFrameQuery_(sessionHandle, pHandle, pElements, numElements, pBlobSize);
}
PRESENTMON_API2_EXPORT PM_STATUS pmConsumeFrames(PM_FRAME_QUERY_HANDLE handle, uint32_t processId, uint8_t* pBlobs, uint32_t* pNumFramesToRead)
{
	assert(pFunc_pmConsumeFrames_);
	return pFunc_pmConsumeFrames_(handle, processId, pBlobs, pNumFramesToRead);
}
PRESENTMON_API2_EXPORT PM_STATUS pmFreeFrameQuery(PM_FRAME_QUERY_HANDLE handle)
{
	assert(pFunc_pmFreeFrameQuery_);
	return pFunc_pmFreeFrameQuery_(handle);
}
PRESENTMON_API2_EXPORT PM_STATUS pmOpenSession_(PM_SESSION_HANDLE* pHandle, const char* pipeNameOverride, const char* introNsmOverride)
{
	if (!pFunc_pmOpenSession__) {
		if (auto status = LoadLibrary_(); status != PM_STATUS_SUCCESS) {
			return status;
		}
	}
	return pFunc_pmOpenSession__(pHandle, pipeNameOverride, introNsmOverride);
}
PRESENTMON_API2_EXPORT void pmSetMiddlewareAsMock_(bool mocked, bool useCrtHeapDebug, bool useLocalShmServer)
{
	assert(pFunc_pmSetMiddlewareAsMock__);
	pFunc_pmSetMiddlewareAsMock__(mocked, useCrtHeapDebug, useLocalShmServer);
}
PRESENTMON_API2_EXPORT _CrtMemState pmCreateHeapCheckpoint_()
{
	assert(pFunc_pmCreateHeapCheckpoint__);
	return pFunc_pmCreateHeapCheckpoint__();
}
PRESENTMON_API2_EXPORT PM_STATUS pmMiddlewareSpeak_(PM_SESSION_HANDLE handle, char* buffer)
{
	assert(pFunc_pmMiddlewareSpeak__);
	return pFunc_pmMiddlewareSpeak__(handle, buffer);
}
PRESENTMON_API2_EXPORT PM_STATUS pmMiddlewareAdvanceTime_(PM_SESSION_HANDLE handle, uint32_t milliseconds)
{
	assert(pFunc_pmMiddlewareAdvanceTime__);
	return pFunc_pmMiddlewareAdvanceTime__(handle, milliseconds);
}
PRESENTMON_API2_EXPORT LoggingSingletons pmLinkLogging_(std::shared_ptr<pmon::util::log::IChannel> pChannel,
	std::function<pmon::util::log::IdentificationTable& ()> getIdTable)
{
	if (!pFunc_pmOpenSession__) {
		if (auto status = LoadLibrary_(); status != PM_STATUS_SUCCESS) {
			return {};
		}
	}
	assert(pFunc_pmLinkLogging__);
	return pFunc_pmLinkLogging__(pChannel, getIdTable);
}
PRESENTMON_API2_EXPORT void pmFlushEntryPoint_() noexcept
{
	// flush is called even in cases where the dll hasn't been loaded
	// allow it to be elided in this case since it has no effect without
	// other functions being called first anyways
	if (pFunc_pmFlushEntryPoint__) {
		pFunc_pmFlushEntryPoint__();
	}
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetup(const PM_DIAGNOSTIC_CONFIGURATION* pConfig)
{
	assert(pFunc_pmDiagnosticSetup_);
	return pFunc_pmDiagnosticSetup_(pConfig);
}
PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetQueuedMessageCount()
{
	assert(pFunc_pmDiagnosticGetQueuedMessageCount_);
	return pFunc_pmDiagnosticGetQueuedMessageCount_();
}
PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetMaxQueuedMessages()
{
	assert(pFunc_pmDiagnosticGetMaxQueuedMessages_);
	return pFunc_pmDiagnosticGetMaxQueuedMessages_();
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetMaxQueuedMessages(uint32_t max)
{
	assert(pFunc_pmDiagnosticSetMaxQueuedMessages_);
	return pFunc_pmDiagnosticSetMaxQueuedMessages_(max);
}
PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetDiscardedMessageCount()
{
	assert(pFunc_pmDiagnosticGetDiscardedMessageCount_);
	return pFunc_pmDiagnosticGetDiscardedMessageCount_();
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticDequeueMessage(PM_DIAGNOSTIC_MESSAGE** ppMessage)
{
	assert(pFunc_pmDiagnosticDequeueMessage_);
	return pFunc_pmDiagnosticDequeueMessage_(ppMessage);
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticEnqueueMessage(const PM_DIAGNOSTIC_MESSAGE* pMessage)
{
	assert(pFunc_pmDiagnosticEnqueueMessage_);
	return pFunc_pmDiagnosticEnqueueMessage_(pMessage);
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticFreeMessage(PM_DIAGNOSTIC_MESSAGE* pMessage)
{
	assert(pFunc_pmDiagnosticFreeMessage_);
	return pFunc_pmDiagnosticFreeMessage_(pMessage);
}
PRESENTMON_API2_EXPORT PM_DIAGNOSTIC_WAKE_REASON pmDiagnosticWaitForMessage(uint32_t timeoutMs)
{
	assert(pFunc_pmDiagnosticWaitForMessage_);
	return pFunc_pmDiagnosticWaitForMessage_(timeoutMs);
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticUnblockWaitingThread()
{
	assert(pFunc_pmDiagnosticUnblockWaitingThread_);
	return pFunc_pmDiagnosticUnblockWaitingThread_();
}
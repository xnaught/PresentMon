#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPI2/PresentMonDiagnostics.h"
#include "../CommonUtilities/win/WinAPI.h"
#include "../Versioning/PresentMonAPIVersion.h"
#include <functional>
#include <vector>
#include <cassert>
#include <type_traits>
#include <optional>
#include <mutex>
#include "Registry.h"
#include "Loader.h"


// pointers to runtime-resolved core API functions
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
PM_STATUS(*pFunc_pmGetApiVersion_)(PM_VERSION*) = nullptr;
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
_CrtMemState(*pFunc_pmCreateHeapCheckpoint__)() = nullptr;
LoggingSingletons(*pFunc_pmLinkLogging__)(std::shared_ptr<pmon::util::log::IChannel>,
	std::function<pmon::util::log::IdentificationTable&()>) = nullptr;
void(*pFunc_pmFlushEntryPoint__)() = nullptr;
void(*pFunc_pmConfigureStandaloneLogging__)() = nullptr;


// internal loader state globals
std::string middlewareDllPath_;
std::mutex middlewareLoadMtx_;
std::optional<PM_STATUS> middlewareLoadResult_;


// exception for use in the loader routine
class LoadException_ : public std::exception
{
public:
	LoadException_(PM_STATUS code_in) : code{ code_in } {}
	PM_STATUS code;
	const char* what() const noexcept override
	{
		return "Error occurred while loading the PresentMon Middleware DLL";
	}
};


// loader-internal implementation functions
// inner impl of method for deriving the mangled name of a cpp-linkage function and loading it
void* GetCppProcAddress_Impl_(HMODULE h,const char* name, const char* mangledEmbeddedSignature)
{
	// don't bother including receiving template function name
	std::string Signature = mangledEmbeddedSignature + 22;
	// The signature of F appears twice (return type and template parameter type)
	size_t len = Signature.find("@@YA");
	std::string templateParam = Signature.substr(0, len);
	// templateParam is a _pointer_ to a function (P6), so adjust to function type (Y)
	std::string funName = "?" + std::string(name) + "@@Y" + templateParam.substr(2);
	// find the address of the function in the dll
	if (auto pFunc = GetProcAddress(h, funName.c_str())) {
		return pFunc;
	}
	throw LoadException_{ PM_STATUS_MIDDLEWARE_MISSING_ENDPOINT };
}
// load a cpp linkage function from a dll module
template<typename F>
F GetCppProcAddress_(HMODULE h, const char* name)
{
	return reinterpret_cast<F>(GetCppProcAddress_Impl_(h, name, __FUNCDNAME__));
}
// load a c-linkage function
FARPROC GetProcAddress_(HMODULE h, const char* name)
{
	if (auto pFunc = GetProcAddress(h, name)) {
		return pFunc;
	}
	throw LoadException_{ PM_STATUS_MIDDLEWARE_MISSING_ENDPOINT };
}
// routine to load the middleware dll and resolve addresses of all the API endpoint functions
PRESENTMON_API2_EXPORT PM_STATUS LoadLibrary_()
{
	// ensure following routine is only run once
	// concurrent calls will block until the first one completes
	std::lock_guard lk{ middlewareLoadMtx_ };
	if (middlewareLoadResult_) {
		// we were not first, return the same result as the first call
		return *middlewareLoadResult_;
	}
	try {
		// get path to middleware dll from the registry if not already overridden
		if (middlewareDllPath_.empty()) {
			// discover the full path of the middleware dll using registry
			Reg::SetReadonly();
			middlewareDllPath_ = Reg::Get().middlewarePath;
		}
		// attempt to load the dll
		const HMODULE hMod = LoadLibraryA(middlewareDllPath_.c_str());
		if (!hMod) {
			throw LoadException_{ PM_STATUS_MIDDLEWARE_MISSING_DLL };
		}

#define RESOLVE(f) pFunc_##f##_ = reinterpret_cast<decltype(pFunc_##f##_)>(GetProcAddress_(hMod, #f))
#define RESOLVE_CPP(f) pFunc_##f##_ = GetCppProcAddress_<decltype(pFunc_##f##_)>(hMod, #f)
		// first check version before attempting to resolve all endpoints
		RESOLVE(pmGetApiVersion);
		{
			PM_VERSION dllVersion{};
			if (pmGetApiVersion(&dllVersion) != PM_STATUS_SUCCESS) {
				throw LoadException_{ PM_STATUS_FAILURE };
			}
			const auto buildVersion = pmon::bid::GetApiVersion();
			if (dllVersion.major > buildVersion.major) {
				throw LoadException_{ PM_STATUS_MIDDLEWARE_VERSION_HIGH };
			}
			if (dllVersion.major < buildVersion.major) {
				throw LoadException_{ PM_STATUS_MIDDLEWARE_VERSION_LOW };
			}
		}
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
		RESOLVE_CPP(pmCreateHeapCheckpoint_); // ??
		RESOLVE_CPP(pmLinkLogging_);
		RESOLVE_CPP(pmFlushEntryPoint_);
		RESOLVE_CPP(pmConfigureStandaloneLogging_);
		// if we make it here then we have succeeded
		middlewareLoadResult_ = PM_STATUS_SUCCESS;
	}
	catch (const LoadException_& ex) {
		middlewareLoadResult_ = ex.code;
	}
	catch (const winreg::RegException&) {
		middlewareLoadResult_ = PM_STATUS_MIDDLEWARE_MISSING_PATH;
	}
	catch (...) {
		middlewareLoadResult_ = PM_STATUS_FAILURE;
	}

	return *middlewareLoadResult_;
}


// helper macro to lazy-load DLL on-demand; exits function returning error code on failure
#define LoadEndpointIfEmpty_(pFunc) if (!pFunc) if (auto sta = LoadLibrary_()) return sta;
// satisfying exports from middleware by proxying them
PRESENTMON_API2_EXPORT PM_STATUS pmOpenSession(PM_SESSION_HANDLE* pHandle)
{
	LoadEndpointIfEmpty_(pFunc_pmOpenSession_);
	return pFunc_pmOpenSession_(pHandle);
}
PRESENTMON_API2_EXPORT PM_STATUS pmCloseSession(PM_SESSION_HANDLE handle)
{
	LoadEndpointIfEmpty_(pFunc_pmCloseSession_);
	return pFunc_pmCloseSession_(handle);
}
PRESENTMON_API2_EXPORT PM_STATUS pmStartTrackingProcess(PM_SESSION_HANDLE handle, uint32_t process_id)
{
	LoadEndpointIfEmpty_(pFunc_pmStartTrackingProcess_);
	return pFunc_pmStartTrackingProcess_(handle, process_id);
}
PRESENTMON_API2_EXPORT PM_STATUS pmStopTrackingProcess(PM_SESSION_HANDLE handle, uint32_t process_id)
{
	LoadEndpointIfEmpty_(pFunc_pmStartTrackingProcess_);
	return pFunc_pmStartTrackingProcess_(handle, process_id);
}
PRESENTMON_API2_EXPORT PM_STATUS pmGetIntrospectionRoot(PM_SESSION_HANDLE handle, const PM_INTROSPECTION_ROOT** ppRoot)
{
	LoadEndpointIfEmpty_(pFunc_pmGetIntrospectionRoot_);
	return pFunc_pmGetIntrospectionRoot_(handle, ppRoot);
}
PRESENTMON_API2_EXPORT PM_STATUS pmFreeIntrospectionRoot(const PM_INTROSPECTION_ROOT* pRoot)
{
	LoadEndpointIfEmpty_(pFunc_pmFreeIntrospectionRoot_);
	return pFunc_pmFreeIntrospectionRoot_(pRoot);
}
PRESENTMON_API2_EXPORT PM_STATUS pmSetTelemetryPollingPeriod(PM_SESSION_HANDLE handle, uint32_t deviceId, uint32_t timeMs)
{
	LoadEndpointIfEmpty_(pFunc_pmSetTelemetryPollingPeriod_);
	return pFunc_pmSetTelemetryPollingPeriod_(handle, deviceId, timeMs);
}
PRESENTMON_API2_EXPORT PM_STATUS pmSetEtwFlushPeriod(PM_SESSION_HANDLE handle, uint32_t periodMs)
{
	LoadEndpointIfEmpty_(pFunc_pmSetEtwFlushPeriod_);
	return pFunc_pmSetEtwFlushPeriod_(handle, periodMs);
}
PRESENTMON_API2_EXPORT PM_STATUS pmRegisterDynamicQuery(PM_SESSION_HANDLE sessionHandle, PM_DYNAMIC_QUERY_HANDLE* pHandle, PM_QUERY_ELEMENT* pElements, uint64_t numElements, double windowSizeMs, double metricOffsetMs)
{
	LoadEndpointIfEmpty_(pFunc_pmRegisterDynamicQuery_);
	return pFunc_pmRegisterDynamicQuery_(sessionHandle, pHandle, pElements, numElements, windowSizeMs, metricOffsetMs);
}
PRESENTMON_API2_EXPORT PM_STATUS pmFreeDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle)
{
	LoadEndpointIfEmpty_(pFunc_pmFreeDynamicQuery_);
	return pFunc_pmFreeDynamicQuery_(handle);
}
PRESENTMON_API2_EXPORT PM_STATUS pmPollDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains)
{
	LoadEndpointIfEmpty_(pFunc_pmPollDynamicQuery_);
	return pFunc_pmPollDynamicQuery_(handle, processId, pBlob, numSwapChains);
}
PRESENTMON_API2_EXPORT PM_STATUS pmPollStaticQuery(PM_SESSION_HANDLE sessionHandle, const PM_QUERY_ELEMENT* pElement, uint32_t processId, uint8_t* pBlob)
{
	LoadEndpointIfEmpty_(pFunc_pmPollStaticQuery_);
	return pFunc_pmPollStaticQuery_(sessionHandle, pElement, processId, pBlob);
}
PRESENTMON_API2_EXPORT PM_STATUS pmRegisterFrameQuery(PM_SESSION_HANDLE sessionHandle, PM_FRAME_QUERY_HANDLE* pHandle, PM_QUERY_ELEMENT* pElements, uint64_t numElements, uint32_t* pBlobSize)
{
	LoadEndpointIfEmpty_(pFunc_pmRegisterFrameQuery_);
	return pFunc_pmRegisterFrameQuery_(sessionHandle, pHandle, pElements, numElements, pBlobSize);
}
PRESENTMON_API2_EXPORT PM_STATUS pmConsumeFrames(PM_FRAME_QUERY_HANDLE handle, uint32_t processId, uint8_t* pBlobs, uint32_t* pNumFramesToRead)
{
	LoadEndpointIfEmpty_(pFunc_pmConsumeFrames_);
	return pFunc_pmConsumeFrames_(handle, processId, pBlobs, pNumFramesToRead);
}
PRESENTMON_API2_EXPORT PM_STATUS pmFreeFrameQuery(PM_FRAME_QUERY_HANDLE handle)
{
	LoadEndpointIfEmpty_(pFunc_pmFreeFrameQuery_);
	return pFunc_pmFreeFrameQuery_(handle);
}
PRESENTMON_API2_EXPORT PM_STATUS pmGetApiVersion(PM_VERSION* pVersion)
{
	LoadEndpointIfEmpty_(pFunc_pmGetApiVersion_);
	return pFunc_pmGetApiVersion_(pVersion);
}
// expose
PRESENTMON_API2_EXPORT PM_STATUS pmOpenSession_(PM_SESSION_HANDLE* pHandle, const char* pipeNameOverride, const char* introNsmOverride)
{
	LoadEndpointIfEmpty_(pFunc_pmOpenSession__);
	return pFunc_pmOpenSession__(pHandle, pipeNameOverride, introNsmOverride);
}
// deprecate?
PRESENTMON_API2_EXPORT _CrtMemState pmCreateHeapCheckpoint_()
{
	if (!pFunc_pmCreateHeapCheckpoint__) {
		if (auto status = LoadLibrary_(); status != PM_STATUS_SUCCESS) {
			throw LoadException_{ status };
		}
	}
	return pFunc_pmCreateHeapCheckpoint__();
}
PRESENTMON_API2_EXPORT LoggingSingletons pmLinkLogging_(std::shared_ptr<pmon::util::log::IChannel> pChannel,
	std::function<pmon::util::log::IdentificationTable& ()> getIdTable)
{
	if (!pFunc_pmLinkLogging__) {
		if (auto status = LoadLibrary_(); status != PM_STATUS_SUCCESS) {
			throw LoadException_{ status };
		}
	}
	assert(pFunc_pmLinkLogging__);
	return pFunc_pmLinkLogging__(pChannel, getIdTable);
}
// expose C / rethink?
PRESENTMON_API2_EXPORT void pmFlushEntryPoint_() noexcept
{
	// flush is called even in cases where the dll hasn't been loaded
	// allow it to be elided in this case since it has no effect without
	// other functions being called previously anyways
	if (pFunc_pmFlushEntryPoint__) {
		pFunc_pmFlushEntryPoint__();
	}
}
PRESENTMON_API2_EXPORT void pmConfigureStandaloneLogging_()
{
	if (!pFunc_pmConfigureStandaloneLogging__) {
		if (auto status = LoadLibrary_(); status != PM_STATUS_SUCCESS) {
			throw LoadException_{ status };
		}
	}
	pFunc_pmConfigureStandaloneLogging__();
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetup(const PM_DIAGNOSTIC_CONFIGURATION* pConfig)
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticSetup_);
	return pFunc_pmDiagnosticSetup_(pConfig);
}
PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetQueuedMessageCount()
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticGetQueuedMessageCount_);
	return pFunc_pmDiagnosticGetQueuedMessageCount_();
}
PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetMaxQueuedMessages()
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticGetMaxQueuedMessages_);
	return pFunc_pmDiagnosticGetMaxQueuedMessages_();
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetMaxQueuedMessages(uint32_t max)
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticSetMaxQueuedMessages_);
	return pFunc_pmDiagnosticSetMaxQueuedMessages_(max);
}
PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetDiscardedMessageCount()
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticGetDiscardedMessageCount_);
	return pFunc_pmDiagnosticGetDiscardedMessageCount_();
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticDequeueMessage(PM_DIAGNOSTIC_MESSAGE** ppMessage)
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticDequeueMessage_);
	return pFunc_pmDiagnosticDequeueMessage_(ppMessage);
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticEnqueueMessage(const PM_DIAGNOSTIC_MESSAGE* pMessage)
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticEnqueueMessage_);
	return pFunc_pmDiagnosticEnqueueMessage_(pMessage);
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticFreeMessage(PM_DIAGNOSTIC_MESSAGE* pMessage)
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticFreeMessage_);
	return pFunc_pmDiagnosticFreeMessage_(pMessage);
}
PRESENTMON_API2_EXPORT PM_DIAGNOSTIC_WAKE_REASON pmDiagnosticWaitForMessage(uint32_t timeoutMs)
{
	// don't lazy load here since A) it make no sense to ever call this function first and B)
	// we have no convenient mechanism to relay a loader error
	// instead, ignore the call and return a wake_reason error when the pointer is empty
	if (!pFunc_pmDiagnosticWaitForMessage_) {
		return PM_DIAGNOSTIC_WAKE_REASON_ERROR;
	}
	return pFunc_pmDiagnosticWaitForMessage_(timeoutMs);
}
PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticUnblockWaitingThread()
{
	LoadEndpointIfEmpty_(pFunc_pmDiagnosticUnblockWaitingThread_);
	return pFunc_pmDiagnosticUnblockWaitingThread_();
}

// exports unique to loader
PRESENTMON_API2_EXPORT void pmLoaderSetPathToMiddlewareDll_(const char* path)
{
	middlewareDllPath_ = path;
}


// null logger implementation to satisfy linker (logging from loader not currently supported)
namespace pmon::util::log
{
	std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
	{
		return {};
	}
}
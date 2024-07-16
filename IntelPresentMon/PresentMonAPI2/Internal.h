#pragma once
#include "PresentMonAPI.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/LineTable.h"
#include "../CommonUtilities/log/GlobalPolicy.h"
#include "../CommonUtilities/log/IdentificationTable.h"
#include <memory>
#include <functional>

#ifdef __cplusplus
extern "C" {
#endif
	PRESENTMON_API2_EXPORT PM_STATUS pmOpenSession_(PM_SESSION_HANDLE* pHandle, const char* pipeNameOverride, const char* introNsmOverride);
#ifdef __cplusplus
}
#endif

// testing support functions
PRESENTMON_API2_EXPORT void pmSetMiddlewareAsMock_(bool mocked, bool useCrtHeapDebug = false, bool useLocalShmServer = true);
PRESENTMON_API2_EXPORT _CrtMemState pmCreateHeapCheckpoint_();
PRESENTMON_API2_EXPORT PM_STATUS pmMiddlewareSpeak_(PM_SESSION_HANDLE handle, char* buffer);
PRESENTMON_API2_EXPORT PM_STATUS pmMiddlewareAdvanceTime_(PM_SESSION_HANDLE handle, uint32_t milliseconds);

// log configuration support functions
struct LoggingSingletons
{
	std::function<pmon::util::log::GlobalPolicy& ()> getGlobalPolicy;
	std::function<pmon::util::log::LineTable& ()> getLineTable;
};
// function to connect (subordinate) the dll logging system to the exe one
// replace default channel (nullptr) with a channel that copies entries to pChannel
// optionally hook up an id table to copy entries to as well
// return getters for config singletons in the dll to config from the exe
PRESENTMON_API2_EXPORT LoggingSingletons pmLinkLogging_(
	std::shared_ptr<pmon::util::log::IChannel> pChannel,
	std::function<pmon::util::log::IdentificationTable&()> getIdTable);
// function to flush the dll's log channel worker queue when before exiting
PRESENTMON_API2_EXPORT void pmFlushEntryPoint_() noexcept;

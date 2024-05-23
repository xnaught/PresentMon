#pragma once
#include "PresentMonAPI.h"
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

	enum PM_DIAGNOSTIC_LEVEL
	{
		PM_DIAGNOSTIC_LEVEL_NONE,
		PM_DIAGNOSTIC_LEVEL_FATAL,
		PM_DIAGNOSTIC_LEVEL_ERROR,
		PM_DIAGNOSTIC_LEVEL_WARNING,
		PM_DIAGNOSTIC_LEVEL_INFO,
		PM_DIAGNOSTIC_LEVEL_DEBUG,
		PM_DIAGNOSTIC_LEVEL_VERBOSE,
	};

	enum PM_DIAGNOSTIC_SUBSYSTEM
	{
		PM_DIAGNOSTIC_SUBSYSTEM_NONE,
		PM_DIAGNOSTIC_SUBSYSTEM_MIDDLEWARE,
		PM_DIAGNOSTIC_SUBSYSTEM_SERVER,
		PM_DIAGNOSTIC_SUBSYSTEM_WRAPPER,
		PM_DIAGNOSTIC_SUBSYSTEM_INTEL_PRESENTMON,
		PM_DIAGNOSTIC_SUBSYSTEM_USER = 0x8000,
	};

	enum PM_DIAGNOSTIC_WAKE_REASON
	{
		PM_DIAGNOSTIC_WAKE_REASON_NONE,
		PM_DIAGNOSTIC_WAKE_REASON_MESSAGE_AVAILABLE,
		PM_DIAGNOSTIC_WAKE_REASON_TIMEOUT,
		PM_DIAGNOSTIC_WAKE_REASON_SHUTDOWN,
		PM_DIAGNOSTIC_WAKE_REASON_MANUAL_UNBLOCK,
		PM_DIAGNOSTIC_WAKE_REASON_ERROR,
	};

	enum PM_DIAGNOSTIC_OUTPUT_FLAGS
	{
		PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER = 1,
		PM_DIAGNOSTIC_OUTPUT_FLAGS_STDERR = 2,
		PM_DIAGNOSTIC_OUTPUT_FLAGS_STDOUT = 4,
		PM_DIAGNOSTIC_OUTPUT_FLAGS_QUEUE = 8,
	};

	struct PM_DIAGNOSTIC_MESSAGE
	{
		PM_DIAGNOSTIC_LEVEL level;
		PM_DIAGNOSTIC_SUBSYSTEM system;
		const char* pText;
	};

	// all pmDiagnostic functions must NOT be invoked concurrently
	// the only exception is pmDiagnosticSignalWaiter which can be called concurrently with the other functions
	// from any number of threads
	// ideally, after setup is completed only one thread shfould be calling the get/dequeue/wait functions
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetup(PM_DIAGNOSTIC_OUTPUT_FLAGS);
	PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetQueuedMessageCount();
	PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetMaxQueuedMessages();
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetMaxQueuedMessages(uint32_t);
	PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetDiscardedMessageCount();
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticDequeueMessage(PM_DIAGNOSTIC_MESSAGE** ppMessage);
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticFreeMessage(PM_DIAGNOSTIC_MESSAGE* pMessage);
	PRESENTMON_API2_EXPORT PM_DIAGNOSTIC_WAKE_REASON pmDiagnosticWaitForMessage(uint32_t timeoutMs);
	// useful during shutdown if you have a worker thread blocked waiting for a message, you can
	// wake it up so that it can exit gracefully
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticUnblockWaitingThread();

#ifdef __cplusplus
} // extern "C"
#endif
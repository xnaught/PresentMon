#pragma once
#include "PresentMonAPI.h"


PRESENTMON_API2_EXPORT void pmSetMiddlewareAsMock_(bool mocked, bool useCrtHeapDebug = false, bool useLocalShmServer = true);
PRESENTMON_API2_EXPORT _CrtMemState pmCreateHeapCheckpoint_();
PRESENTMON_API2_EXPORT PM_STATUS pmMiddlewareSpeak_(char* buffer);
PRESENTMON_API2_EXPORT PM_STATUS pmMiddlewareAdvanceTime_(uint32_t milliseconds);

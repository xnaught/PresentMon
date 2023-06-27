// Copyright (C) 2020,2023 Intel Corporation
// SPDX-License-Identifier: MIT

struct PMTraceConsumer;
struct MRTraceConsumer;

struct PMTraceSession {
    PMTraceConsumer* mPMConsumer = nullptr; // Required PMTraceConsumer instance
    MRTraceConsumer* mMRConsumer = nullptr; // If nullptr, no WinMR tracing

    LARGE_INTEGER mStartQpc = {};
    LARGE_INTEGER mQpcFrequency = {};
    double mQpcPerMilliSecond = 0.0;
    double mMilliSecondsPerQpc = 0.0;
    FILETIME mStartTime = {};

    TRACEHANDLE mSessionHandle = 0;                         // invalid session handles are 0
    TRACEHANDLE mTraceHandle = INVALID_PROCESSTRACE_HANDLE; // invalid trace handles are INVALID_PROCESSTRACE_HANDLE

    ULONG mContinueProcessingBuffers = TRUE;

    ULONG mNumEventsLost = 0;
    ULONG mNumBuffersLost = 0;

    bool mIsRealtimeSession = false;

    ULONG Start(wchar_t const* etlPath,      // If nullptr, start a live/realtime tracing session
                wchar_t const* sessionName); // Required session name
    void Stop();

    double QpcDeltaToMilliSeconds(uint64_t qpcDelta) const;
    double QpcDeltaToMilliSeconds(uint64_t qpcFrom, uint64_t qpcTo) const;
    double QpcDeltaToUnsignedMilliSeconds(uint64_t qpcFrom, uint64_t qpcTo) const;
    double QpcToMilliSeconds(uint64_t qpc) const;
    void QpcToLocalSystemTime(uint64_t qpc, SYSTEMTIME* st, uint64_t* ns) const;
    uint64_t MilliSecondsDeltaToQpc(double millisecondsDelta) const;
};

ULONG StopNamedTraceSession(wchar_t const* sessionName);


// Copyright (C) 2023-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <assert.h>
#include <evntrace.h>
#include <evntprov.h>
#include <stdint.h>
#include <stdlib.h>

#include "PresentMonProvider.h"

namespace {

static GUID const ProviderGUID = { 0xecaa4712, 0x4644, 0x442f, { 0xb9, 0x4c, 0xa3, 0x2f, 0x6c, 0xf8, 0xa4, 0x99 }};

enum {
    ID_PresentFrameType = 1,
    ID_FlipFrameType    = 2,
};

enum {
    Keyword_FrameTypes = 1 << 0,
};

enum Event {
    Event_PresentFrameType,
    Event_FlipFrameType,
    Event_Count
};

// Note: code below (e.g., EnableCallback()) assumes that all of these use TRACE_LEVEL_INFORMATION,
// and would need to be updated if you extend this to use other levels.
static EVENT_DESCRIPTOR const EventDescriptor[] = {
    // ID, Version, Channel, Level, Opcode, Task, Keyword
    { ID_PresentFrameType, 0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_PresentFrameType, Keyword_FrameTypes },
    { ID_FlipFrameType,    0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_FlipFrameType,    Keyword_FrameTypes },
};

static_assert(Event_Count == _countof(EventDescriptor), "Event enum and EventDescriptor size mismatch");
static_assert(Event_Count <= 32,                        "Too many events for current PresentMonProvider::EnableBits");

}

struct PresentMonProvider {
    HMODULE   Advapi32Module;
    ULONG     (*pEventRegister)(LPCGUID, PENABLECALLBACK, PVOID, PREGHANDLE);
    ULONG     (*pEventUnregister)(REGHANDLE);
    ULONG     (*pEventWrite)(REGHANDLE, PCEVENT_DESCRIPTOR, ULONG, PEVENT_DATA_DESCRIPTOR);

    REGHANDLE ProviderHandle;
    ULONG     EnableBits;
};

namespace {

bool EventIsDisabled(
    PresentMonProvider const& ctxt,
    Event event)
{
    return (ctxt.EnableBits & (1u << event)) == 0;
}

bool KeywordIsEnabled(
    ULONGLONG Keyword,
    ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeyword)
{
    return Keyword == 0ull || ((Keyword & MatchAnyKeyword) && ((Keyword & MatchAllKeyword) == MatchAllKeyword));
}

void EnableCallback(
    LPCGUID, // SourceId
    ULONG ControlCode,
    UCHAR Level,
    ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeyword,
    PEVENT_FILTER_DESCRIPTOR, // FilterData
    PVOID CallbackContext)
{
    auto ctxt = (PresentMonProvider*) CallbackContext;
    if (ctxt != nullptr) {
        switch (ControlCode) {
        case EVENT_CONTROL_CODE_ENABLE_PROVIDER:
            ctxt->EnableBits = 0;
            if (Level == 0 || Level >= TRACE_LEVEL_INFORMATION) {
                for (uint32_t i = 0; i < Event_Count; ++i) {
                    ULONG bit = KeywordIsEnabled(EventDescriptor[i].Keyword, MatchAnyKeyword, MatchAllKeyword) ? 1u : 0u;
                    ctxt->EnableBits |= (bit << i);
                }
            }
            break;

        case EVENT_CONTROL_CODE_DISABLE_PROVIDER:
            ctxt->EnableBits = 0;
            break;
        }
    }
}

void FillDesc(EVENT_DATA_DESCRIPTOR*) {}

template<typename T, typename... Ts>
void FillDesc(
    EVENT_DATA_DESCRIPTOR* data,
    T* param,
    Ts... params)
{
    data->Ptr = (ULONGLONG) param;
    data->Size = sizeof(*param);

    FillDesc(data + 1, params...);
}

ULONG WriteEvent(
    PresentMonProvider* ctxt,
    Event event)
{
    if (EventIsDisabled(*ctxt, event)) return ERROR_SUCCESS;

    return (*ctxt->pEventWrite)(ctxt->ProviderHandle, &EventDescriptor[event], 0, nullptr);
}

template<typename... Ts>
ULONG WriteEvent(
    PresentMonProvider* ctxt,
    Event event,
    Ts... params)
{
    if (EventIsDisabled(*ctxt, event)) return ERROR_SUCCESS;

    EVENT_DATA_DESCRIPTOR data[sizeof...(Ts)];
    FillDesc(data, &params...);

    return (*ctxt->pEventWrite)(ctxt->ProviderHandle, &EventDescriptor[event], _countof(data), data);
}

bool IsValid(
    PresentMonProvider_FrameType frameType)
{
    return frameType == PresentMonProvider_FrameType_Unspecified ||
           frameType == PresentMonProvider_FrameType_Original ||
           frameType == PresentMonProvider_FrameType_Repeated ||
           frameType == PresentMonProvider_FrameType_AMD_AFMF;
}

}

PresentMonProvider* PresentMonProvider_Initialize()
{
    auto ctxt = new PresentMonProvider;
    if (ctxt == nullptr) {
        return nullptr;
    }

    ctxt->Advapi32Module = LoadLibraryExA("advapi32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (ctxt->Advapi32Module == nullptr) {
        delete ctxt;
        return nullptr;
    }

    ctxt->pEventRegister   = (decltype(ctxt->pEventRegister))   GetProcAddress(ctxt->Advapi32Module, "EventRegister");
    ctxt->pEventUnregister = (decltype(ctxt->pEventUnregister)) GetProcAddress(ctxt->Advapi32Module, "EventUnregister");
    ctxt->pEventWrite      = (decltype(ctxt->pEventWrite))      GetProcAddress(ctxt->Advapi32Module, "EventWrite");
    if (ctxt->pEventRegister == nullptr ||
        ctxt->pEventUnregister == nullptr ||
        ctxt->pEventWrite == nullptr) {
        FreeLibrary(ctxt->Advapi32Module);
        delete ctxt;
        return nullptr;
    }

    ctxt->EnableBits = 0;
    ULONG status = (*ctxt->pEventRegister)(&ProviderGUID, (PENABLECALLBACK) &EnableCallback, ctxt, &ctxt->ProviderHandle);
    if (status != ERROR_SUCCESS) {
        FreeLibrary(ctxt->Advapi32Module);
        delete ctxt;
        return nullptr;
    }

    return ctxt;
}

void PresentMonProvider_ShutDown(
    PresentMonProvider* ctxt)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    if (ctxt->ProviderHandle != 0) {
        ULONG e = (*ctxt->pEventUnregister)(ctxt->ProviderHandle);
        (void) e;
    }

    if (ctxt->Advapi32Module != nullptr) {
        FreeLibrary(ctxt->Advapi32Module);
    }

    RtlZeroMemory(ctxt, sizeof(PresentMonProvider));
    delete ctxt;
}

ULONG PresentMonProvider_PresentFrameType(
    PresentMonProvider* ctxt,
    PresentMonProvider_FrameType frameType)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);
    PRESENTMONPROVIDER_ASSERT(IsValid(frameType));

    return WriteEvent(ctxt, Event_PresentFrameType, (uint8_t) frameType);
}

ULONG PresentMonProvider_FlipFrameType(
    PresentMonProvider* ctxt,
    uint32_t vidPnSourceId,
    uint32_t layerIndex,
    uint64_t presentId,
    PresentMonProvider_FrameType frameType)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);
    PRESENTMONPROVIDER_ASSERT(IsValid(frameType));

    return WriteEvent(ctxt, Event_FlipFrameType, vidPnSourceId,
                                                 layerIndex,
                                                 presentId,
                                                 (uint8_t) frameType);
}


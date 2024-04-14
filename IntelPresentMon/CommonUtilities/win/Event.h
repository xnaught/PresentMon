#pragma once
#include "Handle.h"
#include <span>
#include <cstdint>
#include <optional>
#include <concepts>

namespace pmon::util::win
{
    class Event : public Handle
    {
    public:
        // types
        struct ConstructEmptyTag {};
        // functions
        Event(ConstructEmptyTag);
        Event(bool manualReset = true, bool initialState = false);
        void Set();
        void Pulse();
        void ResetEvent();
    };

    std::optional<uint32_t> WaitOnMultipleEvents(std::span<Event::HandleType> events, bool waitAll = 0, uint32_t milli = 0xFFFF'FFFF);

    template<std::same_as<Event>...E>
    std::optional<uint32_t> WaitAnyEvent(const E&...events)
    {
        Event::HandleType handles[] = { events... };
        return WaitOnMultipleEvents(handles);
    }
}
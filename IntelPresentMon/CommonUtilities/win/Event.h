#pragma once
#include "Handle.h"
#include <span>
#include <cstdint>
#include <optional>
#include <concepts>
#include <chrono>

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
        // set the event flag value to signalled
        void Set();
        // quickly sets and resets the event flag
        void Pulse();
        // resets the event flag value to unsignalled
        void Reset();
    };

    std::optional<uint32_t> WaitOnMultipleEvents(std::span<Event::HandleType> events, bool waitAll = false, uint32_t milli = 0xFFFF'FFFF);

    template<std::same_as<Event>...E>
    std::optional<uint32_t> WaitAnyEvent(const E&...events)
    {
        Event::HandleType handles[] = { events... };
        return WaitOnMultipleEvents(handles);
    }

    template<class D, std::same_as<Event>...E>
    std::optional<uint32_t> WaitAnyEventFor(D timeout, const E&...events)
    {
        Event::HandleType handles[] = { events... };
        const auto milli = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count();
        return WaitOnMultipleEvents(handles, false, milli);
    }
}
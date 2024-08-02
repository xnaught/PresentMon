#include "Event.h"
#include "WinAPI.h"
#include <stdexcept>
#include <cassert>

namespace pmon::util::win
{
	Event::Event(ConstructEmptyTag) {}

	Event::Event(bool manualReset, bool initialState)
		:
		Handle{ CreateEventA(nullptr, (BOOL)manualReset, (BOOL)initialState, nullptr) }
	{
		if (!*this) {
			throw std::runtime_error("Failed to create event");
		}
	}

	void Event::Set()
	{
		if (!SetEvent(*this)) {
			throw std::runtime_error("Failed to set event");
		}
	}

	void Event::Pulse()
	{
		if (!PulseEvent(*this)) {
			throw std::runtime_error("Failed to pulse event");
		}
	}

	void Event::Reset()
	{
		if (!::ResetEvent(*this)) {
			throw std::runtime_error("Failed to reset event");
		}
	}

	std::optional<uint32_t> WaitOnMultipleEvents(std::span<Event::HandleType> events, bool waitAll, uint32_t milli)
	{
		const auto nEvents = (DWORD)events.size();
		assert(nEvents <= MAXIMUM_WAIT_OBJECTS);
		const auto status = WaitForMultipleObjects(nEvents, events.data(), (BOOL)waitAll, (DWORD)milli);
		if (status >= WAIT_OBJECT_0) {
			if (const DWORD eventIndex = status - WAIT_OBJECT_0; eventIndex < nEvents) {
				return eventIndex;
			}
		}
		if (status == WAIT_TIMEOUT) {
			return {};
		}
		// failed, bailed
		throw std::runtime_error{ "Failed waiting on multiple objects" };
	}
}
#pragma once
#include <memory>
#include "../mt/Thread.h"

namespace pmon::util::log
{
	class IEntryMarshallReceiver;
	class IEntrySink;

	class EntryMarshallInjector
	{
	public:
		EntryMarshallInjector(IEntrySink* pTo_, std::shared_ptr<IEntryMarshallReceiver> pSink);
		~EntryMarshallInjector();
	private:
		std::shared_ptr<IEntryMarshallReceiver> pFrom_;
		IEntrySink* pTo_;
		mt::Thread worker_;
	};
}


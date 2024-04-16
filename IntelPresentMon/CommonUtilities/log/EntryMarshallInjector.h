#pragma once
#include <memory>
#include <thread>

namespace pmon::util::log
{
	class IEntryMarshallReceiver;
	class IEntrySink;

	class EntryMarshallInjector
	{
	public:
		EntryMarshallInjector(IEntrySink* pTo_, std::unique_ptr<IEntryMarshallReceiver> pSink);
		~EntryMarshallInjector();
	private:
		std::shared_ptr<IEntryMarshallReceiver> pFrom_;
		IEntrySink* pTo_;
		std::jthread worker_;
	};
}


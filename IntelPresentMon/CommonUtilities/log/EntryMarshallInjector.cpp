#include "EntryMarshallInjector.h"
#include "IEntryMarshallReceiver.h"
#include "IChannel.h"
#include "Entry.h"
#include "StackTrace.h"

namespace pmon::util::log
{
	EntryMarshallInjector::EntryMarshallInjector(IEntrySink* pTo, std::shared_ptr<IEntryMarshallReceiver> pSink)
		:
		pFrom_{ std::move(pSink) },
		pTo_{ pTo }
	{
		worker_ = std::jthread{ [this] {
			while (auto entry = pFrom_->Pop()) {
				pTo_->Submit(std::move(*entry));
			}
		}};
	}
	EntryMarshallInjector::~EntryMarshallInjector()
	{
		pFrom_->SignalExit();
	}
}
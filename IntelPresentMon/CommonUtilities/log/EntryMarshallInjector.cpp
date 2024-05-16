#include "EntryMarshallInjector.h"
#include "IEntryMarshallReceiver.h"
#include "IChannel.h"
#include "Entry.h"
#include "StackTrace.h"
#include "../Exception.h"
#include "PanicLogger.h"
#include "../str/String.h"

namespace pmon::util::log
{
	EntryMarshallInjector::EntryMarshallInjector(std::shared_ptr<IEntrySink> pTo, std::shared_ptr<IEntryMarshallReceiver> pSink)
		:
		pFrom_{ std::move(pSink) },
		pTo_{ std::move(pTo) }
	{
		worker_ = mt::Thread{ L"log-prcv-inj", [this] {
			try {
				while (auto entry = pFrom_->Pop()) {
					pTo_->Submit(std::move(*entry));
				}
			}
			catch (...) {
				pmlog_panic_(ReportExceptionWide());
			}
		}};
	}
	EntryMarshallInjector::~EntryMarshallInjector()
	{
		pmquell(pFrom_->SignalExit())
	}
}
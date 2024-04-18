#include "LinePolicy.h"
#include "Entry.h"
#include "LineTable.h"
#include <format>

namespace pmon::util::log
{

	bool LinePolicy::TransformFilter(Entry& e)
	{
		if (e.rateControl_.type != Entry::RateControl::Type::None) {
			auto& te = LineTable::Lookup(e.GetSourceFileName(), e.sourceLine_);
			e.hitCount_ = te.NextHit();
			switch (e.rateControl_.type) {
			case Entry::RateControl::Type::After:
				if (e.hitCount_ <= e.rateControl_.parameter) return false;
				break;
			case Entry::RateControl::Type::Every:
				if (e.hitCount_ % e.rateControl_.parameter) return false;
				break;
			case Entry::RateControl::Type::EveryAndFirst:
				if ((e.hitCount_ - 1) % e.rateControl_.parameter) return false;
				break;
			case Entry::RateControl::Type::First:
				if (e.hitCount_ > e.rateControl_.parameter) return false;
				break;
			}
		}
		return true;
	}
}

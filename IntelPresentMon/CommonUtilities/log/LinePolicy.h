#pragma once
#include "IPolicy.h"

namespace pmon::util::log
{
	class LinePolicy : public IPolicy
	{
	public:
		bool TransformFilter(Entry& e) override;
	};
}

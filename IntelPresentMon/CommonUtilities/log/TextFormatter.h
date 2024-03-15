#pragma once
#include "ITextFormatter.h"

namespace pmon::util::log
{
	struct Entry;

	class TextFormatter : public ITextFormatter
	{
	public:
		std::wstring Format(const Entry&) const override;
	};
}

#pragma once
#include <string>

namespace pmon::util::log
{
	struct Entry;

	class ITextFormatter
	{
	public:
		virtual ~ITextFormatter() = default;
		virtual std::wstring Format(const Entry&) const = 0;
	};
}

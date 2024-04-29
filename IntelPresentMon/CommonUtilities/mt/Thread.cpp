#include "Thread.h"
#include <format>


namespace pmon::util::mt
{
	std::wstring Thread::MakeThreadName_(const std::wstring& name, int number)
	{
		return std::format(L"{}-{}", name, number);
	}
}
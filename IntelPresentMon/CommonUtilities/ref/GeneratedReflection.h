#pragma once
#include <typeinfo>
#include <string>

namespace pmon::util::ref
{
	std::string DumpGenerated(const std::type_info&, const void* pStruct);

	template<class T>
	std::string DumpGenerated(const T& s)
	{
		return DumpGenerated(typeid(T), &s);
	}
}
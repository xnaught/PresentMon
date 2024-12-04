#pragma once
#include <typeinfo>
#include <string>

namespace pmon::util::ref
{
	std::string DumpStructGenerated(const std::type_info&, const void* pStruct);

	template<class T>
	std::string DumpStructGenerated(const T& s)
	{
		return DumpStructGenerated(typeid(T), &s);
	}
}
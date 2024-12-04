#pragma once
#include <unordered_map>
#include <functional>
#include <string>
#include <typeindex>
#include <sstream>
#include "../GeneratedReflection.h"

// target includes

#include "../../../../Reflector/Test1.h"


namespace pmon::util::ref::gen
{
	void RegisterStructDumpers_(std::unordered_map<std::type_index, std::function<std::string(const void*)>>& dumpers)
	{

		dumpers[typeid(B)] = [](const void* pStruct) {
			const auto& s = *static_cast<const B*>(pStruct);
			std::ostringstream oss;
			oss << "struct B {"

				<< " .fff = " << s.fff

				<< " }";
			return oss.str();
		};

		dumpers[typeid(A)] = [](const void* pStruct) {
			const auto& s = *static_cast<const A*>(pStruct);
			std::ostringstream oss;
			oss << "struct A {"

				<< " .x = " << s.x

				<< " .foo = " << /* Array type */ s.foo

				<< " .barff = " << DumpStructGenerated(typeid(s.barff), &s.barff)

				<< " }";
			return oss.str();
		};

	}
}

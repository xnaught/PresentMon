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
	using namespace std::literals;
	
	namespace {
		template<typename T, size_t N, bool Primitive>
		std::string DumpArray_(const void* pArray)
		{
			auto& arr = *reinterpret_cast<const T(*)[N]>(pArray);
			std::ostringstream oss;
			oss << "[";
			if constexpr (Primitive) {
				for (size_t i = 0; i < N; i++) {
					oss << " " << arr[i] << ",";
				}
			}
			else {
				if (SupportsGeneratedDump<T>()) {
					for (size_t i = 0; i < N; i++) {
						oss << " " << DumpGenerated(arr[i]) << ",";
					}
				}
				else {
					oss << " { unsupported } ";
				}
			}
			oss << "]";
			return oss.str();
		}
	}

	void RegisterDumpers_(std::unordered_map<std::type_index, std::function<std::string(const void*)>>& dumpers)
	{
		// structs
		dumpers[typeid(C)] = [](const void* pStruct) {
			const auto& s = *static_cast<const C*>(pStruct);
			std::ostringstream oss;
			oss << "struct C {"
				<< " .x = " << s.x
				<< " .y = " << s.y
				<< " }";
			return oss.str();
		};
		dumpers[typeid(B)] = [](const void* pStruct) {
			const auto& s = *static_cast<const B*>(pStruct);
			std::ostringstream oss;
			oss << "struct B {"
				<< " .fff = " << s.fff
				<< " .benum = " << DumpGenerated(s.benum)
				<< " .nnn = " << DumpArray_<int, 4, true>(s.nnn)
				<< " .ccc = " << DumpArray_<C, 2, false>(s.ccc)
				<< " .bbb = " << DumpArray_<Barbie, 3, false>(s.bbb)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(AAA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const AAA*>(pStruct);
			std::ostringstream oss;
			oss << "struct AAA {"
				<< " .x = " << s.x
				<< " .foo = " << s.foo
				<< " .barff = " << DumpGenerated(s.barff)
				<< " }";
			return oss.str();
		};

		// enums
		dumpers[typeid(Barbie)] = [](const void* pEnum) {
			const auto& e = *static_cast<const Barbie*>(pEnum);
			switch (e) {
			case Barbie::gir: return "gir"s;
			case Barbie::in: return "in"s;
			case Barbie::a: return "a"s;
			case Barbie::party: return "party"s;
			case Barbie::world: return "world"s;
			default: return "{ unknown }"s;
			}
		};
	}
}

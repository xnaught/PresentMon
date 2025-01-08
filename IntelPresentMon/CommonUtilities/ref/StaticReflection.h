#pragma once
#include <concepts>
#include <string>
#include <sstream>
#include <ranges>
#include <optional>
#include "WrapReflect.h"
#include "../Meta.h"


namespace pmon::util::ref
{
	template<class S>
	void DumpStaticImpl_(const S& s, std::ostringstream& oss)
	{
		if constexpr (IsContainer<std::optional, S>) {
			if (s) {
				oss << *s;
			}
			else {
				oss << "{empty}";
			}
		}
		else if constexpr (std::same_as<std::string, S> || std::same_as<bool, S> || std::is_arithmetic_v<S>) {
			oss << s;
		}
		else if constexpr (std::ranges::range<S>) {
			oss << "[ ";
			for (auto& el : s) {
				DumpStaticImpl_(el, oss);
				oss << ", ";
			}
			oss << "], ";
		}
		else if constexpr (IsContainerLike<S>) {
			oss << "{unknown}";
		}
		else if constexpr (std::is_enum_v<S>) {
			oss << reflect::enum_name(s);
		}
		else if constexpr (std::is_class_v<S>) {
			oss << "struct " << reflect::type_name(s) << " { ";
			if constexpr (reflect::size<S>() == 0) {
				oss << "{empty} }";
				return;
			}
			reflect::for_each([&](const auto I) {
				oss << "." << reflect::member_name<I>(s) << " = ";
				DumpStaticImpl_(reflect::get<I>(s), oss);
				oss << ", ";
				}, s);
			oss << " }";
		}
		else {
			oss << "{unknown}";
		}
	}

	template<class S>
	std::string DumpStatic(const S& s)
	{
		std::ostringstream oss;
		oss << std::boolalpha;
		DumpStaticImpl_(s, oss);
		return oss.str();
	}
}
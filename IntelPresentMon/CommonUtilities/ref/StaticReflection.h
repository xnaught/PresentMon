#pragma once
#include <concepts>
#include <string>
#include <sstream>
#include "WrapReflect.h"


namespace pmon::util::ref
{
	template <typename T> concept Arithmetic = std::is_arithmetic_v<T>;

	template <typename T> concept Container = requires(T t) {
		std::begin(t);
		std::end(t);
	};

	template<typename S, typename N>
	void DumpStaticScalarImpl(const S& s, N&& name, std::ostringstream& oss)
	{
		oss << name << ": " << s << ", ";
	}

	// TODO: support recursive dumping via nested structures and/or containers (vector, array, etc.)
	template<class S>
	void DumpStaticImpl(const S& s, std::ostringstream& oss)
	{
		if constexpr (reflect::size<S>() == 0) {
			oss << "{empty}";
			return;
		}
		reflect::for_each([&](const auto I) {
			auto&& v = reflect::get<I>(s);
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::same_as<std::string, T> || Arithmetic<T>) {
				DumpStaticScalarImpl(v, reflect::member_name<I>(s), oss);
			}
			else {
				oss << reflect::member_name<I>(s) << ": {unknown}, ";
			}
		}, s);
	}

	template<class S>
	std::string DumpStatic(const S& s)
	{
		std::ostringstream oss;
		DumpStaticImpl(s, oss);
		return oss.str();
	}
}
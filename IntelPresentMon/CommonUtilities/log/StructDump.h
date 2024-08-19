#pragma once
#include <concepts>
#include <string>
#include <sstream>
#include "../third/reflect.hpp"


namespace pmon::util::log
{
	template <typename T> concept Arithmetic = std::is_arithmetic_v<T>;

	template <typename T> concept Container = requires(T t) {
		std::begin(t);
		std::end(t);
	};

	template<typename S, typename N>
	void DumpScalarImpl(const S& s, N&& name, std::ostringstream& oss)
	{
		oss << name << ": " << s << ", ";
	}

	// TODO: support recursive dumping via nested structures and/or containers (vector, array, etc.)
	template<class S>
	void DumpStructImpl(const S& s, std::ostringstream& oss)
	{
		if constexpr (reflect::size<S>() == 0) {
			oss << "{empty}";
			return;
		}
		reflect::for_each([&](const auto I) {
			auto&& v = reflect::get<I>(s);
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::same_as<std::string, T> || Arithmetic<T>) {
				DumpScalarImpl(v, reflect::member_name<I>(s), oss);
			}
			else {
				oss << reflect::member_name<I>(s) << ": {unknown}, ";
			}
		}, s);
	}

	template<class S>
	std::string DumpStruct(const S& s)
	{
		std::ostringstream oss;
		DumpStructImpl(s, oss);
		return oss.str();
	}
}
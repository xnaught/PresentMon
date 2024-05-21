#pragma once
#include <cmath>
#include <limits>

namespace pmon::util
{
	template<typename T>
	T CommonEpsilonStrict(T a, T b)
	{
		return std::numeric_limits<T>::epsilon() * std::max(std::abs(a), std::abs(b));
	}

	template<typename T>
	T EpsilonEqual(T a, T b)
	{
		return std::abs(a - b) <= CommonEpsilonStrict(a, b);
	}

	template<typename T>
	T EpsilonLessThan(T a, T b)
	{
		return (a - b) < CommonEpsilon(a, b);
	}
}
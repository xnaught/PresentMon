#pragma once
#include <type_traits>
#include <utility>

namespace pmon::util
{
    template <typename T> struct MemberPointerInfo;

    template <typename S, typename M>
    struct MemberPointerInfo<M S::*> {
        using StructType = S;
        using MemberType = M;
    };

    template<typename T>
    using ContainerElementType = std::remove_reference_t<decltype(*std::begin(std::declval<T&>()))>;
}
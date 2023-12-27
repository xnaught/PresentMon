#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "metadata/EnumDataType.h"
#include <utility>

namespace pmon::ipc::intro {
	// static mapping of datatype enum to static type
	template<PM_DATA_TYPE T>
	struct DataTypeToStaticType;

	template<> struct DataTypeToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_DOUBLE> { using type = double; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_INT32> { using type = int32_t; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_UINT32> { using type = uint32_t; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_ENUM> { using type = int; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_STRING> { using type = char[260]; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_UINT64> { using type = uint64_t; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_BOOL> { using type = bool; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_VOID> { using type = void; };

	// TODO: check completeness of lookup and maybe check the typename matches shortName

	template<PM_DATA_TYPE T>
	using DataTypeToStaticType_t = typename DataTypeToStaticType<T>::type;

	template<PM_DATA_TYPE T>
	constexpr size_t DataTypeToStaticType_sz = sizeof(DataTypeToStaticType_t<T>);
	template<>
	constexpr size_t DataTypeToStaticType_sz<PM_DATA_TYPE::PM_DATA_TYPE_VOID> = 0ull;

	template<template<PM_DATA_TYPE> class F, typename...P>
	auto BridgeDataType(PM_DATA_TYPE dataType, P&&...args)
	{
		switch (dataType) {
#define HANDLE_DATA_TYPE_KEY(enumFrag, keyFrag, name, shortName, description) \
		case PM_DATA_TYPE_##keyFrag: return F<PM_DATA_TYPE_##keyFrag>::Invoke(std::forward<P>(args)...);

		ENUM_KEY_LIST_DATA_TYPE(HANDLE_DATA_TYPE_KEY)

#undef HANDLE_DATA_TYPE_KEY
		}
		return F<PM_DATA_TYPE_VOID>::Default(std::forward<P>(args)...);
	}
}
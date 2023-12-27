#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "IntrospectionMacroHelpers.h"

#include "metadata/EnumDataType.h"
#include "metadata/EnumDeviceType.h"
#include "metadata/EnumDeviceVendor.h"
#include "metadata/EnumGraphicsRuntime.h"
#include "metadata/EnumMetric.h"
#include "metadata/EnumMetricAvailability.h"
#include "metadata/EnumMetricType.h"
#include "metadata/EnumPresentMode.h"
#include "metadata/EnumPsuType.h"
#include "metadata/EnumStat.h"
#include "metadata/EnumStatus.h"
#include "metadata/EnumUnit.h"
#include "metadata/MasterEnumList.h"
#include "metadata/MetricList.h"
#include "metadata/UnitList.h"
#include "metadata/PreferredUnitList.h"

namespace pmon::ipc::intro {
	// static mapping of datatype enum to static type
	template<PM_DATA_TYPE T>
	struct EnumToStaticType;
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_DOUBLE> { using type = double; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_INT32> { using type = int32_t; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_UINT32> { using type = uint32_t; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_ENUM> { using type = int; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_STRING> { using type = char[260]; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_UINT64> { using type = uint64_t; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_BOOL> { using type = bool; };
	template<> struct EnumToStaticType<PM_DATA_TYPE::PM_DATA_TYPE_VOID> { using type = bool; };
	// TODO: find better place for this template glue
	template<PM_DATA_TYPE T>
	using EnumToStaticType_t = typename EnumToStaticType<T>::type;
	template<PM_DATA_TYPE T>
	constexpr size_t EnumToStaticType_sz = sizeof(EnumToStaticType_t<T>);
}
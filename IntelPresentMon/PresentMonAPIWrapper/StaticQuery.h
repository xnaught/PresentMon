#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../PresentMonAPIWrapperCommon/Exception.h"
#include "../PresentMonAPIWrapperCommon/EnumMap.h"
#include "../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "../CommonUtilities/source/str/String.h"
#include "../CommonUtilities/source/Meta.h"
#include "Session.h"
#include "ProcessTracker.h"
#include <format>
#include <string>
#include <memory>
#include <cassert>
#include <array>

namespace pmapi
{
    namespace
    {
        // this static functor converts static types when bridged with runtime PM_DATA_TYPE info
        template<PM_DATA_TYPE dt, PM_ENUM staticEnumId, typename DestType, size_t blobSize>
        struct SQReadBridger
        {
            using SourceType = typename pmon::ipc::intro::DataTypeToStaticType<dt, staticEnumId>::type;
            static_assert(pmon::util::VoidableSizeof<SourceType>() <= blobSize, "Inadequate blob size detected");
            static void Invoke(PM_ENUM enumId, DestType& dest, const uint8_t* pBlobBytes)
            {
                // void types are not handleable and generally should not occur
                if constexpr (dt == PM_DATA_TYPE_VOID) {
                    assert(false && "trying to convert void type");
                }
                // strings (char array) can convert to std::basic_string types
                else if constexpr (dt == PM_DATA_TYPE_STRING) {
                    if constexpr (std::same_as<DestType, std::string>) {
                        dest = reinterpret_cast<const char*>(pBlobBytes);
                    }
                    else if constexpr (std::same_as<DestType, std::wstring>) {
                        dest = pmon::util::str::ToWide(reinterpret_cast<const char*>(pBlobBytes));
                    }
                    else {
                        assert(false && "failure to convert enum type");
                    }
                }
                // enums can convert to numeric types or string types
                else if constexpr (dt == PM_DATA_TYPE_ENUM) {
                    if constexpr (std::same_as<DestType, std::string>) {
                        try {
                            const auto keyId = *reinterpret_cast<const int*>(pBlobBytes);
                            dest = EnumMap::GetKeyMap(enumId)->at(keyId).narrowName;
                        }
                        catch (...) { dest = "Invalid"; }
                    }
                    else if constexpr (std::same_as<DestType, std::wstring>) {
                        try {
                            const auto keyId = *reinterpret_cast<const int*>(pBlobBytes);
                            dest = EnumMap::GetKeyMap(enumId)->at(keyId).wideName;
                        }
                        catch (...) { dest = L"Invalid"; }
                    }
                    else if constexpr (
                        std::is_integral_v<DestType> ||
                        std::is_floating_point_v<DestType> ||
                        std::same_as<DestType, SourceType>) {
                        dest = DestType(*reinterpret_cast<const int*>(pBlobBytes));
                    }
                    else {
                        assert(false && "failure to convert enum type");
                    }
                }
                // what's left is actual numeric types
                else {
                    // don't output if the dest is string type
                    if constexpr (!std::same_as<DestType, std::string> && !std::same_as<DestType, std::wstring>) {
                        dest = static_cast<DestType>(reinterpret_cast<const SourceType&>(*pBlobBytes));
                    }
                    else {
                        assert(false && "failure to convert numeric type");
                    }
                }
            }
            static void Default(DestType& dest, const uint8_t* pBlobBytes) {
                assert(false && "failure to convert unknown type");
            }
        };

        // adapter to convert template taking 4 arguments to template taking 2
        // (we need this because you cannot define templates within a function
        // and the static info is only available inside the templated function)
        template<typename T, size_t blobSize>
        struct SQReadBridgerAdapter {
            template<PM_DATA_TYPE dt, PM_ENUM enumId>
            using Bridger = SQReadBridger<dt, enumId, T, blobSize>;
        };
    }

    class StaticQueryResult
    {
        friend StaticQueryResult PollStatic(const Session& session, const ProcessTracker& process,
            PM_METRIC metric, uint32_t deviceId, uint32_t arrayIndex);
    public:
        template<typename T>
        T As() const
        {
            T val;
            // the bridge will execute the bridger with the correct blob/source static type based
            // on the runtime PM_DATA_TYPE value passed in, and the bridger will convert that to
            // the requested type T and store in val
            pmon::ipc::intro::BridgeDataTypeWithEnum
                <typename SQReadBridgerAdapter<T, blobSize_>::Bridger>
                (dataType_, enumId_, val, blob_.data());
            return val;
        }
        template<typename T>
        operator T() const
        {
            return As<std::remove_reference_t<std::remove_const_t<T>>>();
        }
    private:
        // functions
        StaticQueryResult(PM_DATA_TYPE dataType, PM_ENUM enumId) : dataType_{ dataType }, enumId_{ enumId } {}
        // although NRVO should prevent this from being called, it's not standard guarantee
        // so we still need the copy ctor
        StaticQueryResult(const StaticQueryResult&) = default;
        // data
        // 260 bytes is the maximum possible size for query element data
        static constexpr size_t blobSize_ = 260;
        std::array<uint8_t, blobSize_> blob_;
        PM_DATA_TYPE dataType_;
        PM_ENUM enumId_;
    };

    StaticQueryResult PollStatic(const Session& session, const ProcessTracker& process,
        PM_METRIC metric, uint32_t deviceId = 0, uint32_t arrayIndex = 0)
    {
        const auto pIntro = session.GetIntrospectionRoot();
        const auto dti = pIntro->FindMetric(metric).GetDataTypeInfo();
        StaticQueryResult result{ dti.GetFrameType(), dti.GetEnumId() };
        
        const PM_QUERY_ELEMENT element{
            .metric = metric,
            .stat = PM_STAT_NONE,
            .deviceId = deviceId,
            .arrayIndex = arrayIndex,
        };
        if (const auto err = pmPollStaticQuery(session.GetHandle(), &element, process.GetPid(), result.blob_.data());
            err != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ err, "Error polling static query" };
        }
        return result;
    }
}

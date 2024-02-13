#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
#include "../../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "../../CommonUtilities/source/str/String.h"
#include "Session.h"
#include "ProcessTracker.h"
#include <format>
#include <string>
#include <memory>
#include <cassert>
#include <array>

namespace pmapi
{
    // this static functor converts static types when bridged with runtime PM_DATA_TYPE info
    template<PM_DATA_TYPE dt, typename DestType>
    struct SQReadBridger
    {
        using SourceType = typename pmon::ipc::intro::DataTypeToStaticType<dt>::type;
        static void Invoke(DestType& dest, const uint8_t* pBlobBytes)
        {
            // if src is convertible to dest, then doit
            if constexpr (dt == PM_DATA_TYPE_VOID) {
                // TODO: log or assert?
                return;
            }
            else if constexpr (dt == PM_DATA_TYPE_STRING) {
                if constexpr (std::same_as<DestType, std::string>) {
                    dest = (const char*)pBlobBytes;
                }
                else if constexpr (std::same_as<DestType, std::wstring>) {
                    dest = pmon::util::str::ToWide((const char*)pBlobBytes);
                }
                else {
                    // TODO: log or assert?
                }
            }
            else {
                if constexpr (!std::same_as<DestType, std::string> && !std::same_as<DestType, std::wstring>) {
                    dest = static_cast<DestType>(reinterpret_cast<const SourceType&>(*pBlobBytes));
                }
                else {
                    // TODO: log or assert?
                }
            }
        }
        static void Default(DestType& dest, const uint8_t* pBlobBytes) {}
    };

    // adapter to convert template taking 2 arguments to template taking 1
    // (we need this because you cannot define templates within a function
    // and the static info is only available inside the templated function)
    template<typename T>
    struct SQReadBridgerAdapter {
        template<PM_DATA_TYPE dt>
        using Bridger = SQReadBridger<dt, T>;
    };

    class StaticQueryResult
    {
        friend StaticQueryResult PollStatic(const Session& session, const ProcessTracker& process,
            PM_METRIC metric, uint32_t deviceId, uint32_t arrayIndex);
    public:
        template<typename T>
        T As() const
        {
            using pmon::ipc::intro::BridgeDataType;
            T val;
            // the bridge will execute the bridger with the correct blob/source static type based
            // on the runtime PM_DATA_TYPE value passed in, and the bridger will convert that to
            // the requested type T and store in val
            BridgeDataType<typename SQReadBridgerAdapter<T>::Bridger>(dataType_, val, blob_.data());
            return val;
        }
        template<typename T>
        operator T() const
        {
            return As<std::remove_reference_t<std::remove_const_t<T>>>();
        }
    private:
        // functions
        StaticQueryResult(PM_DATA_TYPE dataType) : dataType_{ dataType } {}
        // although NRVO should prevent this from being called, it's not standard guarantee
        // so we still need the copy ctor
        StaticQueryResult(const StaticQueryResult&) = default;
        // data
        // 260 bytes is the maximum possible size for query element data
        std::array<uint8_t, 260> blob_;
        PM_DATA_TYPE dataType_;
    };

    StaticQueryResult PollStatic(const Session& session, const ProcessTracker& process,
        PM_METRIC metric, uint32_t deviceId = 0, uint32_t arrayIndex = 0)
    {
        const auto pIntro = session.GetIntrospectionRoot();
        StaticQueryResult result{ pIntro->FindMetric(metric).GetDataTypeInfo().GetFrameType() };
        // TODO: make sure the blob size is adequate (use introspection or pass in blob size)
        const PM_QUERY_ELEMENT element{
            .metric = metric,
            .stat = PM_STAT_NONE,
            .deviceId = deviceId,
            .arrayIndex = arrayIndex,
        };
        pmPollStaticQuery(session.GetHandle(), &element, process.GetPid(), result.blob_.data());
        return result;
    }
}

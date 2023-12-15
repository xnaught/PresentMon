#include "DynamicPollingMetric.h"
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include <CommonUtilities/source/str/String.h>
#include <ranges>

namespace p2c::pmon::met
{
    namespace
    {
        std::unordered_map<PM_ENUM, std::unique_ptr<EnumKeyMap>> enumMap;
    }

    using ::pmon::util::str::ToWide;
    DynamicPollingMetric::DynamicPollingMetric(PM_METRIC metricId_, uint32_t arrayIndex_, PM_STAT stat_,
        const pmapi::intro::Root& introRoot)
        :
        Metric{
            ToWide(introRoot.FindMetric(metricId_).Introspect().GetName()),
            ToWide(introRoot.FindMetric(metricId_).IntrospectUnit().GetShortName())
        },
        metricId{ metricId_ },
        stat{ stat_ },
        arrayIndex{ arrayIndex_ },
        statName{ ToWide(introRoot.FindEnumKey(PM_ENUM_STAT, stat).GetShortName()) }
    {
        const auto type = introRoot.FindMetric(metricId).GetDataTypeInfo().GetPolledType();
        if (type == PM_DATA_TYPE_STRING || type == PM_DATA_TYPE_ENUM) {
            numeric = false;
        }
    }
    std::wstring DynamicPollingMetric::GetStatName() const { return statName; }
    const std::wstring& DynamicPollingMetric::GetCategory() const
    {
        static std::wstring cat = L"";
        return cat;
    }
    const std::wstring& DynamicPollingMetric::GetMetricClassName() const
    {
        if (numeric) {
            static std::wstring cls = L"Numeric";
            return cls;
        }
        else {
            static std::wstring cls = L"Text";
            return cls;
        }
    }
    PM_QUERY_ELEMENT DynamicPollingMetric::MakeQueryElement() const
    {
        return PM_QUERY_ELEMENT{
            .metric = metricId,
            .stat = stat,
            .deviceId = deviceId,
            .arrayIndex = arrayIndex,
        };
    }
    void DynamicPollingMetric::Finalize(uint32_t offset_)
    {
        offset = offset_;
    }
    void DynamicPollingMetric::InitEnumMap(const pmapi::intro::Root& introRoot)
    {
        for (auto e : introRoot.GetEnums()) {
            auto pKeys = std::make_unique<EnumKeyMap>(); auto& keys = *pKeys;
            for (auto k : e.GetKeys()) {
                keys[k.GetValue()] = ToWide(k.GetName());
            }
            enumMap[e.GetId()] = std::move(pKeys);
        }
    }
    std::unique_ptr<DynamicPollingMetric> DynamicPollingMetric::RealizeMetric(const pmapi::intro::Root& introRoot,
        CachingQuery* pQuery, uint32_t activeGpuDeviceId)
    {
        namespace rn = std::ranges;
        const auto metricId = this->metricId;
        const auto metricIntro = introRoot.FindMetric(metricId);
        const auto dataTypeInfo = metricIntro.GetDataTypeInfo();
        const auto dataTypeId = dataTypeInfo.GetPolledType();
        // if we determine a metric is targeting a gpu, use the activeGpuId instead of universal device (0)
        const bool isGpuMetric = rn::any_of(metricIntro.GetDeviceMetricInfo(), [](auto&& info) {
            return info.GetDevice().GetType() == PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
        });
        const auto deviceId = isGpuMetric ? activeGpuDeviceId : 0u;
        switch (dataTypeId) {
        case PM_DATA_TYPE_BOOL:
            return std::make_unique<met::TypedDynamicPollingMetric<bool>>(*this, pQuery, deviceId);
        case PM_DATA_TYPE_INT32:
            return std::make_unique<met::TypedDynamicPollingMetric<int32_t>>(*this, pQuery, deviceId);
        case PM_DATA_TYPE_UINT32:
            return std::make_unique<met::TypedDynamicPollingMetric<uint32_t>>(*this, pQuery, deviceId);
        case PM_DATA_TYPE_UINT64:
            return std::make_unique<met::TypedDynamicPollingMetric<uint64_t>>(*this, pQuery, deviceId);
        case PM_DATA_TYPE_DOUBLE:
            return std::make_unique<met::TypedDynamicPollingMetric<double>>(*this, pQuery, deviceId);
        case PM_DATA_TYPE_STRING:
            return std::make_unique<met::TypedDynamicPollingMetric<const char*>>(*this, pQuery, deviceId);
        case PM_DATA_TYPE_ENUM:
            return std::make_unique<met::TypedDynamicPollingMetric<PM_ENUM>>(*this, pQuery, deviceId,
                enumMap.at(dataTypeInfo.GetEnumId()).get());
        }
        // TODO: maybe throw exception here?
        return {};
    }
}
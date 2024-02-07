#include "DynamicPollingFetcher.h"
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include <CommonUtilities/source/str/String.h>
#include <ranges>
#include "../EnumMap.h" 

namespace p2c::pmon::met
{
    using ::pmon::util::str::ToWide;


    DynamicPollingFetcher::DynamicPollingFetcher(const PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& introRoot,
        std::shared_ptr<DynamicQuery> pQuery)
        :
        pQuery_{ std::move(pQuery) },
        offset_{ (uint32_t)qel.dataOffset }
    {
        // overlay will always indicate preferred unit in the widget labels
        // so we must scale from output unit if necessary to match
        const auto metric = introRoot.FindMetric(qel.metric);
        const auto type = metric.GetDataTypeInfo().GetPolledType();
        if (metric.GetUnit() != metric.GetPreferredUnitHint()) {
            scale_ = (float)introRoot.FindUnit(metric.GetUnit())
                .MakeConversionFactor(metric.GetPreferredUnitHint());
        }
    }

    TypedDynamicPollingFetcher<PM_ENUM>::TypedDynamicPollingFetcher(const PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& introRoot,
        std::shared_ptr<DynamicQuery> pQuery, const EnumMap::KeyMap& map)
        :
        DynamicPollingFetcher{ qel, introRoot, std::move(pQuery) },
        keyMap_{ map }
    {}
    std::wstring TypedDynamicPollingFetcher<PM_ENUM>::ReadStringValue()
    {
        if (auto pBlobBytes = pQuery_->GetBlobData()) {
            return keyMap_.at(*reinterpret_cast<const int*>(&pBlobBytes[offset_]));
        }
        return {};
    }

    std::optional<float> TypedDynamicPollingFetcher<PM_ENUM>::ReadValue()
    {
        return {};
    }

    std::shared_ptr<DynamicPollingFetcher> MakeDynamicPollingFetcher(const PM_QUERY_ELEMENT& qel,
        const pmapi::intro::Root& introRoot, std::shared_ptr<DynamicQuery> pQuery, uint32_t activeGpuDeviceId)
    {
        namespace rn = std::ranges;
        const auto metricId = qel.metric;
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
            return std::make_shared<TypedDynamicPollingFetcher<bool>>(qel, introRoot, std::move(pQuery));
        case PM_DATA_TYPE_INT32:
            return std::make_unique<TypedDynamicPollingFetcher<int32_t>>(qel, introRoot, std::move(pQuery));
        case PM_DATA_TYPE_UINT32:
            return std::make_unique<TypedDynamicPollingFetcher<uint32_t>>(qel, introRoot, std::move(pQuery));
        case PM_DATA_TYPE_UINT64:
            return std::make_unique<TypedDynamicPollingFetcher<uint64_t>>(qel, introRoot, std::move(pQuery));
        case PM_DATA_TYPE_DOUBLE:
            return std::make_unique<TypedDynamicPollingFetcher<double>>(qel, introRoot, std::move(pQuery));
        case PM_DATA_TYPE_STRING:
            return std::make_unique<TypedDynamicPollingFetcher<const char*>>(qel, introRoot, std::move(pQuery));
        case PM_DATA_TYPE_ENUM:
            return std::make_unique<TypedDynamicPollingFetcher<PM_ENUM>>(qel, introRoot, std::move(pQuery),
                *EnumMap::GetMapPtr(dataTypeInfo.GetEnumId()));
        }
        // TODO: maybe throw exception here?
        return {};
    }
}
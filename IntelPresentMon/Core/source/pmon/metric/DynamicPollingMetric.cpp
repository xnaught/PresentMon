#include "DynamicPollingMetric.h"

namespace p2c::pmon::met
{
    DynamicPollingMetric::DynamicPollingMetric(PM_METRIC metricId_, uint32_t deviceId_, uint32_t arrayIndex_, PM_STAT stat_,
        std::wstring name_, std::wstring units_)
        :
        Metric{ std::move(name_), std::move(units_) },
        metricId{ metricId_ },
        stat{ stat_ },
        deviceId{ deviceId_ },
        arrayIndex{ arrayIndex_ }
    {}
    std::wstring DynamicPollingMetric::GetStatName() const { return L"shup"; }
    const std::wstring& DynamicPollingMetric::GetCategory() const
    {
        static std::wstring cat = L"";
        return cat;
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
}
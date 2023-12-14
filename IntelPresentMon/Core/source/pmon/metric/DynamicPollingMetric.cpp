#include "DynamicPollingMetric.h"
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include <CommonUtilities/source/str/String.h>

namespace p2c::pmon::met
{
    using ::pmon::util::str::ToWide;
    DynamicPollingMetric::DynamicPollingMetric(PM_METRIC metricId_, uint32_t arrayIndex_, PM_STAT stat_,
        const pmapi::intro::Root& introRoot)
        :
        Metric{
            ToWide(introRoot.FindMetric(metricId_).GetName()),
            ToWide(introRoot.FindMetric(metricId_).GetUnit().GetShortName())
        },
        metricId{ metricId_ },
        stat{ stat_ },
        arrayIndex{ arrayIndex_ },
        statName{ ToWide(introRoot.FindEnumKey(PM_ENUM_STAT, stat).GetShortName()) }
    {
        if (introRoot.FindMetric(metricId).GetDataTypeInfo().GetBasePtr()->polledType == PM_DATA_TYPE_STRING) {
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
}
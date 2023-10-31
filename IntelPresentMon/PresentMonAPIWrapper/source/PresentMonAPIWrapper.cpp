#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "PresentMonAPIWrapper.h"

namespace pmapi::intro
{
    EnumKeyView MetricView::GetMetricKey() const
    {
        return pDataset->FindEnumKey(PM_ENUM_METRIC, (int)pBase->id);
    }
}

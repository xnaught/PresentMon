#include "Introspection.h"

namespace pmapi::intro
{
    bool MetricTypeIsDynamic(PM_METRIC_TYPE type)
    {
        return type == PM_METRIC_TYPE_DYNAMIC || type == PM_METRIC_TYPE_DYNAMIC_FRAME;
    }

    bool MetricTypeIsFrameEvent(PM_METRIC_TYPE type)
    {
        return type == PM_METRIC_TYPE_FRAME_EVENT || type == PM_METRIC_TYPE_DYNAMIC_FRAME;
    }

    EnumKeyView MetricView::GetMetricKey() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC, (int)pBase->id);
    }

    EnumKeyView MetricView::GetUnit() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, (int)pBase->unit);
    }

    EnumKeyView MetricView::GetType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC_TYPE, (int)pBase->type);
    }

    EnumKeyView DataTypeInfoView::GetPolledType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DATA_TYPE, (int)pBase->polledType);
    }

    EnumKeyView DataTypeInfoView::GetFrameType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DATA_TYPE, (int)pBase->frameType);
    }

    EnumView DataTypeInfoView::GetEnum() const
    {
        if (pBase->polledType != PM_DATA_TYPE_ENUM && pBase->frameType != PM_DATA_TYPE_ENUM) {
            throw DatatypeException{ "cannot get enum data for non-enum data type" };
        }
        return pRoot->FindEnum(pBase->enumId);
    }

    EnumKeyView DeviceView::GetType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DEVICE_TYPE, (int)pBase->type);
    }

    EnumKeyView DeviceView::GetVendor() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DEVICE_VENDOR, (int)pBase->vendor);
    }

    DeviceView DeviceMetricInfoView::GetDevice() const
    {
        return pRoot->FindDevice(pBase->deviceId);
    }

    EnumKeyView DeviceMetricInfoView::GetAvailablity() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC_AVAILABILITY, (int)pBase->availability);
    }

    EnumKeyView StatInfoView::GetStat() const
    {
        return pRoot->FindEnumKey(PM_ENUM_STAT, (int)pBase->stat);
    }
}
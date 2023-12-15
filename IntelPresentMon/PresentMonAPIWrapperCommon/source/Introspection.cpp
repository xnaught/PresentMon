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

    EnumKeyView MetricView::Introspect() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC, pBase->id);
    }

    PM_METRIC MetricView::GetId() const
    {
        return pBase->id;
    }

    EnumKeyView MetricView::IntrospectUnit() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, pBase->unit);
    }

    PM_UNIT MetricView::GetUnit() const
    {
        return pBase->unit;
    }

    EnumKeyView MetricView::IntrospectType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC_TYPE, pBase->type);
    }

    PM_METRIC_TYPE MetricView::GetType() const
    {
        return pBase->type;
    }

    DataTypeInfoView MetricView::GetDataTypeInfo() const
    {
        return { pRoot, pBase->pTypeInfo };
    }

    ViewRange<StatInfoView> MetricView::GetStatInfo() const
    {
        return { GetStatInfoBegin_(), GetStatInfoEnd_() };
    }

    EnumKeyView DataTypeInfoView::GetPolledType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DATA_TYPE, pBase->polledType);
    }

    EnumKeyView DataTypeInfoView::GetFrameType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DATA_TYPE, pBase->frameType);
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
        return pRoot->FindEnumKey(PM_ENUM_DEVICE_TYPE, pBase->type);
    }

    EnumKeyView DeviceView::GetVendor() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DEVICE_VENDOR, pBase->vendor);
    }

    DeviceView DeviceMetricInfoView::GetDevice() const
    {
        return pRoot->FindDevice(pBase->deviceId);
    }

    EnumKeyView DeviceMetricInfoView::GetAvailablity() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC_AVAILABILITY, pBase->availability);
    }

    EnumKeyView StatInfoView::IntrospectStat() const
    {
        return pRoot->FindEnumKey(PM_ENUM_STAT, pBase->stat);
    }
}
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
        return pDataset->FindEnumKey(PM_ENUM_METRIC, (int)pBase->id);
    }

    EnumKeyView MetricView::GetUnit() const
    {
        return pDataset->FindEnumKey(PM_ENUM_UNIT, (int)pBase->unit);
    }

    EnumKeyView MetricView::GetType() const
    {
        return pDataset->FindEnumKey(PM_ENUM_METRIC_TYPE, (int)pBase->type);
    }

    EnumKeyView DataTypeInfoView::GetType() const
    {
        return pDataset->FindEnumKey(PM_ENUM_DATA_TYPE, (int)pBase->type);
    }

    EnumView DataTypeInfoView::GetEnum() const
    {
        if (pBase->type != PM_DATA_TYPE_ENUM) {
            throw DatatypeException{ std::format(
                "cannot get enum data for non-enum data type {}",
                GetType().GetName())
            };
        }
        return pDataset->FindEnum(pBase->enumId);
    }

    EnumKeyView DeviceView::GetType() const
    {
        return pDataset->FindEnumKey(PM_ENUM_DEVICE_TYPE, (int)pBase->type);
    }

    EnumKeyView DeviceView::GetVendor() const
    {
        return pDataset->FindEnumKey(PM_ENUM_DEVICE_VENDOR, (int)pBase->vendor);
    }

    DeviceView DeviceMetricInfoView::GetDevice() const
    {
        return pDataset->FindDevice(pBase->deviceId);
    }

    EnumKeyView DeviceMetricInfoView::GetAvailablity() const
    {
        return pDataset->FindEnumKey(PM_ENUM_METRIC_AVAILABILITY, (int)pBase->availability);
    }

    EnumKeyView StatInfoView::GetStat() const
    {
        return pDataset->FindEnumKey(PM_ENUM_STAT, (int)pBase->stat);
    }
}
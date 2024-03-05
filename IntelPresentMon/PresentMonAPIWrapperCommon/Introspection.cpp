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

    EnumKeyView MetricView::IntrospectPreferredUnitHint() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, pBase->preferredUnitHint);
    }

    PM_UNIT MetricView::GetPreferredUnitHint() const
    {
        return pBase->preferredUnitHint;
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

    EnumKeyView DataTypeInfoView::IntrospectPolledType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DATA_TYPE, pBase->polledType);
    }

    EnumKeyView DataTypeInfoView::IntrospectFrameType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DATA_TYPE, pBase->frameType);
    }

    EnumView DataTypeInfoView::IntrospectEnum() const
    {
        if (pBase->polledType != PM_DATA_TYPE_ENUM && pBase->frameType != PM_DATA_TYPE_ENUM) {
            throw DatatypeException{ "cannot get enum data for non-enum data type" };
        }
        return pRoot->FindEnum(pBase->enumId);
    }

    EnumKeyView DeviceView::IntrospectType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DEVICE_TYPE, pBase->type);
    }

    EnumKeyView DeviceView::IntrospectVendor() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DEVICE_VENDOR, pBase->vendor);
    }

    DeviceView DeviceMetricInfoView::GetDevice() const
    {
        return pRoot->FindDevice(pBase->deviceId);
    }

    EnumKeyView DeviceMetricInfoView::IntrospectAvailablity() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC_AVAILABILITY, pBase->availability);
    }

    EnumKeyView StatInfoView::IntrospectStat() const
    {
        return pRoot->FindEnumKey(PM_ENUM_STAT, pBase->stat);
    }

    EnumKeyView UnitView::Introspect() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, pBase->id);
    }

    PM_UNIT UnitView::GetId() const
    {
        return pBase->id;
    }

    EnumKeyView UnitView::IntrospectBaseUnit() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, pBase->baseUnitId);
    }

    PM_UNIT UnitView::GetBaseUnit() const
    {
        return pBase->baseUnitId;
    }

    double UnitView::GetScale() const
    {
        return pBase->scale;
    }

    double UnitView::MakeConversionFactor(PM_UNIT destinationUnitId) const
    {
        auto destUnit = pRoot->FindUnit(destinationUnitId);
        if (destUnit.GetBaseUnit() != GetBaseUnit()) {
            throw std::runtime_error{ "cannot convert incompatible units" };
        }
        return GetScale() / destUnit.GetScale();
    }
}
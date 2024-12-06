#include "PowerTelemetryAdapter.h"
#include "Logging.h"
#include <format>


namespace pwr
{
    void PowerTelemetryAdapter::SetTelemetryCapBit(GpuTelemetryCapBits telemetryCapBit) noexcept
    {
        gpuTelemetryCapBits_.set(static_cast<size_t>(telemetryCapBit));
    }
    PowerTelemetryAdapter::SetTelemetryCapBitset PowerTelemetryAdapter::GetPowerTelemetryCapBits()
    {
        return gpuTelemetryCapBits_;
    }
    bool PowerTelemetryAdapter::HasTelemetryCapBit(GpuTelemetryCapBits bit) const
    {
        pmlog_verb(v::gpu)("Telemetry cap bits being retrieved").pmwatch(GetName()).pmwatch(gpuTelemetryCapBits_.to_string());
        return gpuTelemetryCapBits_.test(size_t(bit));
    }
}
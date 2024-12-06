// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "IntelPowerTelemetryAdapter.h"
#include "Logging.h"
#include "../CommonUtilities/Math.h"
#include "../CommonUtilities/ref/GeneratedReflection.h"

using namespace pmon;
using namespace util;

namespace pwr::intel
{
    // public interface functions

    IntelPowerTelemetryAdapter::IntelPowerTelemetryAdapter(ctl_device_adapter_handle_t handle)
        :
        deviceHandle{ handle }
    {
        properties = {
            .Size = sizeof(ctl_device_adapter_properties_t),
            .pDeviceID = &deviceId,
            .device_id_size = sizeof(deviceId),
        };

        if (auto result = ctlGetDeviceProperties(deviceHandle, &properties);
            result != CTL_RESULT_SUCCESS) {
            throw std::runtime_error{ "Failure to get device properties" };
        }
        pmlog_verb(v::gpu)("Device properties").pmwatch(ref::DumpGenerated(properties));

        if (properties.device_type != CTL_DEVICE_TYPE_GRAPHICS) {
            throw NonGraphicsDeviceException{};
        }

        if (auto result = EnumerateMemoryModules(); result != CTL_RESULT_SUCCESS) {
            throw std::runtime_error{ "Failed to enumerate memory modules" };
        }
    }

    bool IntelPowerTelemetryAdapter::Sample() noexcept
    {
        pmlog_verb(v::gpu)("Sample called");

        LARGE_INTEGER qpc;
        QueryPerformanceCounter(&qpc);
        bool success = true;

        decltype(previousSampleVariant) currentSampleVariant;

        if (useV1PowerTelemetry) {
            currentSampleVariant = ctl_power_telemetry2_t{
                .Size = sizeof(ctl_power_telemetry2_t),
                .Version = 1
            };
            auto currentSample = std::get_if<ctl_power_telemetry2_t>(&currentSampleVariant);
            // sanity check; should never fail
            if (!currentSample) {
                success = false;
                IGCL_ERR(CTL_RESULT_ERROR_INVALID_ARGUMENT);
            }
            if (const auto result = ctlPowerTelemetryGet(deviceHandle,
                (ctl_power_telemetry_t*)currentSample); result != CTL_RESULT_SUCCESS) {
                // treating any error as unavailability of the API version since some driver version do not correctly report
                // version error
                useV1PowerTelemetry = false;
                useNewBandwidthTelemetry = false;
                success = false;
                pmlog_warn("Failed to access ctlPowerTelemetryGet.v1, falling back to .v0");
            }
            pmlog_verb(v::gpu)("get power telemetry V1").pmwatch(ref::DumpGenerated(*currentSample));
        }
        if (!useV1PowerTelemetry) {
            currentSampleVariant = ctl_power_telemetry_t{
                .Size = sizeof(ctl_power_telemetry_t),
                .Version = 0
            };
            auto currentSample = std::get_if<ctl_power_telemetry_t>(&currentSampleVariant);
            // sanity check; should never fail
            if (!currentSample) {
                success = false;
                IGCL_ERR(CTL_RESULT_ERROR_INVALID_ARGUMENT);
            }
            if (const auto result = ctlPowerTelemetryGet(deviceHandle, currentSample);
                result != CTL_RESULT_SUCCESS) {
                success = false;
                IGCL_ERR(result);
            }
            pmlog_verb(v::gpu)("get power telemetry V0").pmwatch(ref::DumpGenerated(*currentSample));
        }

        // Query memory state and bandwidth if supported
        ctl_mem_state_t memory_state = {.Size = sizeof(ctl_mem_state_t)};
        ctl_mem_bandwidth_t memory_bandwidth = {
            .Size = sizeof(ctl_mem_bandwidth_t),
            .Version = 1,
        };
        // Question: why are we only using the first element of memoryModules here?
        if (memoryModules.size() > 0) {
            if (const auto result = ctlMemoryGetState(memoryModules[0], &memory_state);
                result != CTL_RESULT_SUCCESS) {
                success = false;
                IGCL_ERR(result);
            }
            pmlog_verb(v::gpu)("get memory state").pmwatch(ref::DumpGenerated(memory_state));
            if (const auto result = ctlMemoryGetBandwidth(memoryModules[0], &memory_bandwidth);
                result != CTL_RESULT_SUCCESS) {
                success = false;
                IGCL_ERR(result);
            }
            pmlog_verb(v::gpu)("get memory bandwidth").pmwatch(ref::DumpGenerated(memory_bandwidth));
        }

        std::optional<double> gpu_sustained_power_limit_mw;
        {
            double gpu_sustained_power_limit_mw_temp = 0.;
            if (const auto result = ctlOverclockPowerLimitGet(deviceHandle, &gpu_sustained_power_limit_mw_temp);
                result == CTL_RESULT_SUCCESS || result == CTL_RESULT_ERROR_CORE_OVERCLOCK_DEPRECATED_API) {
                if (result == CTL_RESULT_ERROR_CORE_OVERCLOCK_DEPRECATED_API) {
                    pmlog_warn("ctlOverclockPowerLimitGet indicates deprecation");
                }
                gpu_sustained_power_limit_mw = gpu_sustained_power_limit_mw_temp;
            }
            else {
                success = false;
                IGCL_ERR(result);
            }
            pmlog_verb(v::gpu)(std::format("ctlOverclockPowerLimitGet output: {}", gpu_sustained_power_limit_mw_temp));
        }

        if (useV1PowerTelemetry) {
            auto currentSample = std::get_if<ctl_power_telemetry2_t>(&currentSampleVariant);
            // sanity check; should never fail
            if (!currentSample) {
                success = false;
                IGCL_ERR(CTL_RESULT_ERROR_INVALID_ARGUMENT);
            }
            else {
                success = GatherSampleData(*currentSample, memory_state,
                    memory_bandwidth, gpu_sustained_power_limit_mw, (uint64_t)qpc.QuadPart) && success;
            }
        }
        else {
            auto currentSample = std::get_if<ctl_power_telemetry_t>(&currentSampleVariant);
            // sanity check; should never fail
            if (!currentSample) {
                success = false;
                IGCL_ERR(CTL_RESULT_ERROR_INVALID_ARGUMENT);
            }
            else {
                success = GatherSampleData(*currentSample, memory_state,
                    memory_bandwidth, gpu_sustained_power_limit_mw, (uint64_t)qpc.QuadPart) && success;
            }
        }

        return success;
    }

    std::optional<PresentMonPowerTelemetryInfo> IntelPowerTelemetryAdapter::GetClosest(uint64_t qpc) const noexcept
    {
        std::lock_guard<std::mutex> lock(historyMutex);
        const auto nearest = history.GetNearest(qpc);
        if constexpr (PMLOG_BUILD_LEVEL_ >= pmon::util::log::Level::Verbose) {
            if (!nearest) {
                pmlog_verb(v::gpu)("Empty telemetry info sample returned");
            }
            else {
                pmlog_verb(v::gpu)(std::format("Nearest telemetry info sampled; read bw [{}] write bw [{}]",
                    nearest->gpu_mem_read_bandwidth_bps, nearest->gpu_mem_write_bandwidth_bps
                ));
            }
        }
        return nearest;
    }

    PM_DEVICE_VENDOR IntelPowerTelemetryAdapter::GetVendor() const noexcept
    {
        return PM_DEVICE_VENDOR::PM_DEVICE_VENDOR_INTEL;
    }

    std::string IntelPowerTelemetryAdapter::GetName() const noexcept
    {
        return properties.name;
    }

    uint64_t IntelPowerTelemetryAdapter::GetDedicatedVideoMemory() const noexcept
    {
        ctl_mem_state_t memory_state = {.Size = sizeof(ctl_mem_state_t)};
        uint64_t video_mem_size = 0;
        if (memoryModules.size() > 0) {
            if (const auto result = ctlMemoryGetState(memoryModules[0], &memory_state);
                result == CTL_RESULT_SUCCESS) {
                video_mem_size = memory_state.size;
            }
            else {
                IGCL_ERR(result);
            }
            pmlog_verb(v::gpu)("get memory state").pmwatch(ref::DumpGenerated(memory_state));
        }
        return video_mem_size;
    }

    uint64_t IntelPowerTelemetryAdapter::GetVideoMemoryMaxBandwidth() const noexcept
    {
        ctl_mem_bandwidth_t memoryBandwidth = {
            .Size = sizeof(ctl_mem_bandwidth_t),
            .Version = 1,
        };
        uint64_t videoMemMaxBandwidth = 0;
        if (memoryModules.size() > 0) {
            if (const auto result = ctlMemoryGetBandwidth(memoryModules[0], &memoryBandwidth);
                result == CTL_RESULT_SUCCESS) {
                videoMemMaxBandwidth = memoryBandwidth.maxBandwidth;
            }
            else {
                IGCL_ERR(result);
            }
            pmlog_verb(v::gpu)("get memory bandwidth").pmwatch(ref::DumpGenerated(memoryBandwidth));
        }
        return videoMemMaxBandwidth;
    }

    double IntelPowerTelemetryAdapter::GetSustainedPowerLimit() const noexcept
    {
        double gpuSustainedPowerLimit = 0.;
        if (const auto result = ctlOverclockPowerLimitGet(deviceHandle, &gpuSustainedPowerLimit);
            result == CTL_RESULT_SUCCESS || result == CTL_RESULT_ERROR_CORE_OVERCLOCK_DEPRECATED_API) {
            if (result == CTL_RESULT_ERROR_CORE_OVERCLOCK_DEPRECATED_API) {
                pmlog_warn("ctlOverclockPowerLimitGet indicates deprecation");
            }
            // Control lib returns back in milliwatts
            gpuSustainedPowerLimit = gpuSustainedPowerLimit / 1000.;
        }
        pmlog_verb(v::gpu)(std::format("ctlOverclockPowerLimitGet output: {}", gpuSustainedPowerLimit));
        return gpuSustainedPowerLimit;
    }

    // private implementation functions

    ctl_result_t IntelPowerTelemetryAdapter::EnumerateMemoryModules()
    {
        // first call ctlEnumMemoryModules with nullptr to get number of modules
        // and resize vector to accomodate
        uint32_t memory_module_count = 0;
        {
            if (auto result = ctlEnumMemoryModules(deviceHandle, &memory_module_count,
                nullptr); result != CTL_RESULT_SUCCESS)
            {
                return result;
            }
            pmlog_verb(v::gpu)("Memory module count").pmwatch(memory_module_count);
            memoryModules.resize(size_t(memory_module_count));
        }

        if (auto result = ctlEnumMemoryModules(deviceHandle, &memory_module_count,
            memoryModules.data()); result != CTL_RESULT_SUCCESS)
        {
            return result;
        }

        return CTL_RESULT_SUCCESS;
    }

    template<class T>
    bool IntelPowerTelemetryAdapter::GatherSampleData(T& currentSample,
        ctl_mem_state_t& memory_state,
        ctl_mem_bandwidth_t& memory_bandwidth,
        std::optional<double> gpu_sustained_power_limit_mw,
        uint64_t qpc)
    {
        bool success = true;

        if (const auto result = GetTimeDelta(currentSample);
            result != CTL_RESULT_SUCCESS)
        {
            success = false;
            IGCL_ERR(result);
        }

        PresentMonPowerTelemetryInfo pm_gpu_power_telemetry_info{ .qpc = qpc };

        if (previousSampleVariant.index()) {

            if (const auto result = GetGPUPowerTelemetryData(
                currentSample, pm_gpu_power_telemetry_info); result != CTL_RESULT_SUCCESS)
            {
                success = false;
                IGCL_ERR(result);
            }

            if (const auto result = GetVramPowerTelemetryData(
                currentSample, pm_gpu_power_telemetry_info); result != CTL_RESULT_SUCCESS)
            {
                success = false;
                IGCL_ERR(result);
            }

            if (const auto result = GetFanPowerTelemetryData(currentSample,
                pm_gpu_power_telemetry_info); result != CTL_RESULT_SUCCESS)
            {
                success = false;
                IGCL_ERR(result);
            }

            if (const auto result = GetPsuPowerTelemetryData(
                currentSample, pm_gpu_power_telemetry_info); result != CTL_RESULT_SUCCESS)
            {
                success = false;
                IGCL_ERR(result);
            }

            // Get memory state and bandwidth data
            if (memoryModules.size() > 0) {
                GetMemStateTelemetryData(memory_state,
                    pm_gpu_power_telemetry_info);
                GetMemBandwidthData(memory_bandwidth,
                    pm_gpu_power_telemetry_info);
            }

            // Save and convert the gpu sustained power limit
            pm_gpu_power_telemetry_info.gpu_sustained_power_limit_w =
                gpu_sustained_power_limit_mw.value_or(0.) / 1000.;
            if (gpu_sustained_power_limit_mw) {
                SetTelemetryCapBit(GpuTelemetryCapBits::gpu_sustained_power_limit);                
            }

            // Save off the calculated PresentMon power telemetry values. These are
            // saved off for clients to extrace out timing information based on QPC
            SavePmPowerTelemetryData(pm_gpu_power_telemetry_info);
        }

        // Save off the raw control library data for calculating time delta
        // and usage data.
        if (const auto result = SaveTelemetry(currentSample, memory_bandwidth);
            result != CTL_RESULT_SUCCESS)
        {
            success = false;
            IGCL_ERR(result);
        }

        return success;
    }

    // TODO: stop using CTL stuff for non-ctl logic
    // TODO: better functional programming
    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetTimeDelta(const T& currentSample)
    {
        if (!previousSampleVariant.index()) {
            // We do not have a previous power telemetry item to calculate time
            // delta against.
            time_delta_ = 0.f;
        }
        else {
            auto previousSample = std::get_if<T>(&previousSampleVariant);
            if (previousSample && currentSample.timeStamp.type == CTL_DATA_TYPE_DOUBLE) {
                time_delta_ = currentSample.timeStamp.value.datadouble -
                    previousSample->timeStamp.value.datadouble;
            }
            else {
                return CTL_RESULT_ERROR_INVALID_ARGUMENT;
            }
        }

        return CTL_RESULT_SUCCESS;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetGPUPowerTelemetryData(
        const T& currentSample,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info)
    {
        ctl_result_t result;

        auto previousSample = std::get_if<T>(&previousSampleVariant);
        if (!previousSample) {
            return CTL_RESULT_ERROR_INVALID_ARGUMENT;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.timeStamp,
            pm_gpu_power_telemetry_info.time_stamp,
            GpuTelemetryCapBits::time_stamp);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.gpuVoltage,
            pm_gpu_power_telemetry_info.gpu_voltage_v,
            GpuTelemetryCapBits::gpu_voltage);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.gpuCurrentClockFrequency,
            pm_gpu_power_telemetry_info.gpu_frequency_mhz,
            GpuTelemetryCapBits::gpu_frequency);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.gpuCurrentTemperature,
            pm_gpu_power_telemetry_info.gpu_temperature_c,
            GpuTelemetryCapBits::gpu_temperature);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsage(
            currentSample.gpuEnergyCounter,
            previousSample->gpuEnergyCounter,
            pm_gpu_power_telemetry_info.gpu_power_w,
            GpuTelemetryCapBits::gpu_power);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsagePercent(
            currentSample.globalActivityCounter,
            previousSample->globalActivityCounter,
            pm_gpu_power_telemetry_info.gpu_utilization,
            GpuTelemetryCapBits::gpu_utilization);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsagePercent(
            currentSample.renderComputeActivityCounter,
            previousSample->renderComputeActivityCounter,
            pm_gpu_power_telemetry_info.gpu_render_compute_utilization,
            GpuTelemetryCapBits::gpu_render_compute_utilization);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsagePercent(
            currentSample.mediaActivityCounter,
            previousSample->mediaActivityCounter,
            pm_gpu_power_telemetry_info.gpu_media_utilization,
            GpuTelemetryCapBits::gpu_media_utilization);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        pm_gpu_power_telemetry_info.gpu_power_limited =
            currentSample.gpuPowerLimited;
        pm_gpu_power_telemetry_info.gpu_temperature_limited =
            currentSample.gpuTemperatureLimited;
        pm_gpu_power_telemetry_info.gpu_current_limited =
            currentSample.gpuCurrentLimited;
        pm_gpu_power_telemetry_info.gpu_voltage_limited =
            currentSample.gpuVoltageLimited;
        pm_gpu_power_telemetry_info.gpu_utilization_limited =
            currentSample.gpuUtilizationLimited;

        // On Intel all GPU limitation indicators are active
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_power_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_temperature_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_current_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_voltage_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_utilization_limited);

        return result;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetVramPowerTelemetryData(
        const T& currentSample,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info)
    {
        ctl_result_t result;

        auto previousSample = std::get_if<T>(&previousSampleVariant);
        if (!previousSample) {
            return CTL_RESULT_ERROR_INVALID_ARGUMENT;
        }

        result = GetInstantaneousPowerTelemetryItem(currentSample.vramVoltage,
            pm_gpu_power_telemetry_info.vram_voltage_v,
            GpuTelemetryCapBits::vram_voltage);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.vramCurrentClockFrequency,
            pm_gpu_power_telemetry_info.vram_frequency_mhz,
            GpuTelemetryCapBits::vram_frequency);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.vramCurrentEffectiveFrequency,
            pm_gpu_power_telemetry_info.vram_effective_frequency_gbps,
            GpuTelemetryCapBits::vram_effective_frequency);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.vramCurrentTemperature,
            pm_gpu_power_telemetry_info.vram_temperature_c,
            GpuTelemetryCapBits::vram_temperature);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        // bandwidth telemetry has 2 possible paths for aquisition
        if constexpr (std::same_as<T, ctl_power_telemetry2_t>) {
            using namespace pmon::util;
            if (useNewBandwidthTelemetry) {
                double gpuMemReadBandwidthMegabytesPerSecond = 0;
                result = GetInstantaneousPowerTelemetryItem(
                    currentSample.vramReadBandwidth,
                    gpuMemReadBandwidthMegabytesPerSecond,
                    GpuTelemetryCapBits::gpu_mem_read_bandwidth);
                // we need bandwidth in bits per second, IGCL V1 gives in megabytes per second
                pm_gpu_power_telemetry_info.gpu_mem_read_bandwidth_bps = ConvertMagnitudePrefix(
                    gpuMemReadBandwidthMegabytesPerSecond * 8.,
                    MagnitudePrefix::Mega,
                    MagnitudePrefix::Base);
                pmlog_verb(v::gpu)(std::format("VRAM read BW V1: bSupported [{}] type [{}] units [{}] data_64 [{}] data_double [{}] info []{}",
                    currentSample.vramReadBandwidth.bSupported, (int)currentSample.vramReadBandwidth.type,
                    (int)currentSample.vramReadBandwidth.units, currentSample.vramReadBandwidth.value.datau64,
                    currentSample.vramReadBandwidth.value.datadouble, pm_gpu_power_telemetry_info.gpu_mem_read_bandwidth_bps));
                if (result != CTL_RESULT_SUCCESS ||
                    !(HasTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_read_bandwidth))) {
                    useNewBandwidthTelemetry = false;
                    pmlog_info("V1 vram bandwidth not available, falling back to V0 counters")
                        .code(result).pmwatch(HasTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_read_bandwidth));
                }
            }
            if (useNewBandwidthTelemetry) {
                double gpuMemWriteBandwidthMegabytesPerSecond = 0;
                result = GetInstantaneousPowerTelemetryItem(
                    currentSample.vramWriteBandwidth,
                    gpuMemWriteBandwidthMegabytesPerSecond,
                    GpuTelemetryCapBits::gpu_mem_write_bandwidth);
                // we need bandwidth in bits per second, IGCL V1 gives in megabytes per second
                pm_gpu_power_telemetry_info.gpu_mem_write_bandwidth_bps = ConvertMagnitudePrefix(
                    gpuMemWriteBandwidthMegabytesPerSecond * 8.,
                    MagnitudePrefix::Mega,
                    MagnitudePrefix::Base);
                pmlog_verb(v::gpu)(std::format("VRAM write BW V1: bSupported [{}] type [{}] units [{}] data_64 [{}] data_double [{}] info []{}",
                    currentSample.vramWriteBandwidth.bSupported, (int)currentSample.vramWriteBandwidth.type,
                    (int)currentSample.vramWriteBandwidth.units, currentSample.vramWriteBandwidth.value.datau64,
                    currentSample.vramWriteBandwidth.value.datadouble, pm_gpu_power_telemetry_info.gpu_mem_write_bandwidth_bps));
                if (result != CTL_RESULT_SUCCESS ||
                    !(HasTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_write_bandwidth))) {
                    useNewBandwidthTelemetry = false;
                    pmlog_info("V1 vram bandwidth not available, falling back to V0 counters")
                        .code(result).pmwatch(HasTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_write_bandwidth));
                }
            }
        }
        if (!useNewBandwidthTelemetry) {
            result = GetPowerTelemetryItemUsage(
                currentSample.vramReadBandwidthCounter,
                previousSample->vramReadBandwidthCounter,
                pm_gpu_power_telemetry_info.gpu_mem_read_bandwidth_bps,
                GpuTelemetryCapBits::gpu_mem_read_bandwidth);
            pmlog_verb(v::gpu)(std::format("VRAM read BW V0: bSupported [{}] type [{}] units [{}] data_64 [{}] data_double [{}] info []{}",
                currentSample.vramReadBandwidthCounter.bSupported, (int)currentSample.vramReadBandwidthCounter.type,
                (int)currentSample.vramReadBandwidthCounter.units, currentSample.vramReadBandwidthCounter.value.datau64,
                currentSample.vramReadBandwidthCounter.value.datadouble, pm_gpu_power_telemetry_info.gpu_mem_read_bandwidth_bps));
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }
            result = GetPowerTelemetryItemUsage(
                currentSample.vramWriteBandwidthCounter,
                previousSample->vramWriteBandwidthCounter,
                pm_gpu_power_telemetry_info.gpu_mem_write_bandwidth_bps,
                GpuTelemetryCapBits::gpu_mem_write_bandwidth);
            pmlog_verb(v::gpu)(std::format("VRAM write BW V0: bSupported [{}] type [{}] units [{}] data_64 [{}] data_double [{}] info []{}",
                currentSample.vramWriteBandwidthCounter.bSupported, (int)currentSample.vramWriteBandwidthCounter.type,
                (int)currentSample.vramWriteBandwidthCounter.units, currentSample.vramWriteBandwidthCounter.value.datau64,
                currentSample.vramWriteBandwidthCounter.value.datadouble, pm_gpu_power_telemetry_info.gpu_mem_write_bandwidth_bps));
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }
        }

        result = GetPowerTelemetryItemUsage(currentSample.vramEnergyCounter,
            previousSample->vramEnergyCounter,
            pm_gpu_power_telemetry_info.vram_power_w,
            GpuTelemetryCapBits::vram_power);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        pm_gpu_power_telemetry_info.vram_power_limited =
            currentSample.vramPowerLimited;
        pm_gpu_power_telemetry_info.vram_temperature_limited =
            currentSample.vramTemperatureLimited;
        pm_gpu_power_telemetry_info.vram_current_limited =
            currentSample.vramCurrentLimited;
        pm_gpu_power_telemetry_info.vram_voltage_limited =
            currentSample.vramVoltageLimited;
        pm_gpu_power_telemetry_info.vram_utilization_limited =
            currentSample.vramUtilizationLimited;

        // On Intel all GPU limitation indicators are active
        SetTelemetryCapBit(GpuTelemetryCapBits::vram_power_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::vram_temperature_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::vram_current_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::vram_voltage_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::vram_utilization_limited);

        return result;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetFanPowerTelemetryData(
        const T& currentSample,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info)
    {
        ctl_result_t result = CTL_RESULT_SUCCESS;

        for (uint32_t i = 0; i < CTL_FAN_COUNT; i++) {
            GpuTelemetryCapBits fan_telemetry_bit;
            try {
                fan_telemetry_bit = GetFlagTelemetryCapBit(i);
                result = GetInstantaneousPowerTelemetryItem(
                    currentSample.fanSpeed[i],
                    pm_gpu_power_telemetry_info.fan_speed_rpm[i],
                    fan_telemetry_bit);
                if (result != CTL_RESULT_SUCCESS) {
                  break;
                }
            } catch (...) {
                result = CTL_RESULT_ERROR_INVALID_ARGUMENT;
            } 
        }

        return result;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetPsuPowerTelemetryData(
        const T& currentSample,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info)
    {
        ctl_result_t result = CTL_RESULT_SUCCESS;

        auto previousSample = std::get_if<T>(&previousSampleVariant);
        if (!previousSample) {
            return CTL_RESULT_ERROR_INVALID_ARGUMENT;
        }

        for (uint32_t i = 0; i < CTL_PSU_COUNT; i++) {
            if (currentSample.psu[i].bSupported) {
                GpuTelemetryCapBits psu_telemetry_bit;
                try {
                  psu_telemetry_bit = GetPsuTelemetryCapBit(i);
                  pm_gpu_power_telemetry_info.psu[i].psu_type =
                      PresentMonPsuType(currentSample.psu[i].psuType);
                  result = GetInstantaneousPowerTelemetryItem(
                      currentSample.psu[i].voltage,
                      pm_gpu_power_telemetry_info.psu[i].psu_voltage,
                      psu_telemetry_bit);
                  if (result != CTL_RESULT_SUCCESS) {
                    break;
                  }
                  result = GetPowerTelemetryItemUsage(
                      currentSample.psu[i].energyCounter,
                      previousSample->psu[i].energyCounter,
                      pm_gpu_power_telemetry_info.psu[i].psu_power,
                      psu_telemetry_bit);
                  if (result != CTL_RESULT_SUCCESS) {
                    break;
                  }
                } catch (...) {
                  result = CTL_RESULT_ERROR_INVALID_ARGUMENT;
                  break;
                } 
            }
            else {
                pm_gpu_power_telemetry_info.psu[i].psu_type =
                    PresentMonPsuType::None;
                pm_gpu_power_telemetry_info.psu[i].psu_power = 0.0f;
                pm_gpu_power_telemetry_info.psu[i].psu_voltage = 0.0f;
            }
        }

        return result;
    }

    void IntelPowerTelemetryAdapter::GetMemStateTelemetryData(
        const ctl_mem_state_t& mem_state,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info) {
        pm_gpu_power_telemetry_info.gpu_mem_total_size_b = mem_state.size;
        pm_gpu_power_telemetry_info.gpu_mem_used_b = mem_state.size - mem_state.free;
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_size);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_used);
        return;
    }

    void IntelPowerTelemetryAdapter::GetMemBandwidthData(
        const ctl_mem_bandwidth_t& mem_bandwidth,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info) {
        pm_gpu_power_telemetry_info.gpu_mem_max_bandwidth_bps = mem_bandwidth.maxBandwidth;
        gpu_mem_max_bw_cache_value_bps_ = mem_bandwidth.maxBandwidth;
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_max_bandwidth);
        return;
    }
    
    ctl_result_t IntelPowerTelemetryAdapter::GetInstantaneousPowerTelemetryItem(
        const ctl_oc_telemetry_item_t& telemetry_item,
        double& pm_telemetry_value, GpuTelemetryCapBits telemetry_cap_bit)
    {
        if (telemetry_item.bSupported) {
            if (telemetry_item.type == CTL_DATA_TYPE_DOUBLE) {
                pm_telemetry_value = telemetry_item.value.datadouble;
                SetTelemetryCapBit(telemetry_cap_bit);
            }
            else {
                // Expecting a double return type here
                return CTL_RESULT_ERROR_INVALID_ARGUMENT;
            }
        }
        else {
            pm_telemetry_value = 0.0;
        }
        return CTL_RESULT_SUCCESS;
    }

    ctl_result_t IntelPowerTelemetryAdapter::GetPowerTelemetryItemUsagePercent(
        const ctl_oc_telemetry_item_t& current_telemetry_item,
        const ctl_oc_telemetry_item_t& previous_telemetry_item,
        double& pm_telemetry_value, GpuTelemetryCapBits telemetry_cap_bit)
    {
        if (current_telemetry_item.bSupported) {
            if (current_telemetry_item.type == CTL_DATA_TYPE_DOUBLE) {
                auto data_delta = current_telemetry_item.value.datadouble -
                    previous_telemetry_item.value.datadouble;
                pm_telemetry_value = (data_delta / time_delta_) * 100.0f;
                SetTelemetryCapBit(telemetry_cap_bit);
            }
            else {
                // Expecting a double return type here
                return CTL_RESULT_ERROR_INVALID_ARGUMENT;
            }
        }
        return CTL_RESULT_SUCCESS;
    }

    ctl_result_t IntelPowerTelemetryAdapter::GetPowerTelemetryItemUsage(
        const ctl_oc_telemetry_item_t& current_telemetry_item,
        const ctl_oc_telemetry_item_t& previous_telemetry_item,
        double& pm_telemetry_value, GpuTelemetryCapBits telemetry_cap_bit)
    {
        if (current_telemetry_item.bSupported) {
            if (current_telemetry_item.type == CTL_DATA_TYPE_DOUBLE) {
                auto data_delta = current_telemetry_item.value.datadouble -
                    previous_telemetry_item.value.datadouble;
                pm_telemetry_value = (data_delta / time_delta_);
                SetTelemetryCapBit(telemetry_cap_bit);
            } 
            else if (current_telemetry_item.type == CTL_DATA_TYPE_INT64) {
                auto data_delta = current_telemetry_item.value.data64 -
                    previous_telemetry_item.value.data64;
                pm_telemetry_value = static_cast<double>(data_delta) / time_delta_;
                SetTelemetryCapBit(telemetry_cap_bit);
            }
            else if (current_telemetry_item.type == CTL_DATA_TYPE_UINT64) {
              auto data_delta = current_telemetry_item.value.datau64 -
                                previous_telemetry_item.value.datau64;
              pm_telemetry_value =
                  static_cast<double>(data_delta) / time_delta_;
              SetTelemetryCapBit(telemetry_cap_bit);
              // stopgap measure for bad vram bandwidth telemetry coming out of V0 api
              if (telemetry_cap_bit == GpuTelemetryCapBits::gpu_mem_read_bandwidth && !useNewBandwidthTelemetry) {
                  if ((current_telemetry_item.value.datau64 < previous_telemetry_item.value.datau64) ||
                      ((current_telemetry_item.value.datau64 - previous_telemetry_item.value.datau64) > gpu_mem_max_bw_cache_value_bps_)) {
                      pm_telemetry_value = gpu_mem_read_bw_cache_value_bps_;
                  }
                  else {
                      gpu_mem_read_bw_cache_value_bps_ = pm_telemetry_value;
                  }
              }
            }
            else {
                // Expecting a double return type here
                return CTL_RESULT_ERROR_INVALID_ARGUMENT;
            }
        }
        return CTL_RESULT_SUCCESS;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::SaveTelemetry(
        const T& currentSample,
        const ctl_mem_bandwidth_t& currentMemBandwidthSample)
    {
        if (currentSample.timeStamp.type == CTL_DATA_TYPE_DOUBLE) {
            previousSampleVariant = currentSample;
        }
        else {
            return CTL_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return CTL_RESULT_SUCCESS;
    }

    void IntelPowerTelemetryAdapter::SavePmPowerTelemetryData(PresentMonPowerTelemetryInfo& info)
    {
        pmlog_verb(v::gpu)(std::format("Saving gathered telemetry info to history; read bw [{}] write bw [{}]",
            info.gpu_mem_read_bandwidth_bps, info.gpu_mem_write_bandwidth_bps
        ));
        std::lock_guard<std::mutex> lock(historyMutex);
        history.Push(info);
    }

    GpuTelemetryCapBits IntelPowerTelemetryAdapter::GetFlagTelemetryCapBit(
        uint32_t fan_index) {
        switch (fan_index) {
            case 0:
                return GpuTelemetryCapBits::fan_speed_0;
            case 1:
                return GpuTelemetryCapBits::fan_speed_1;
            case 2:
                return GpuTelemetryCapBits::fan_speed_2;
            case 3:
                return GpuTelemetryCapBits::fan_speed_3;
            case 4:
                return GpuTelemetryCapBits::fan_speed_4;
            default:
                throw std::runtime_error{"Invalid fan index"};
        }
    }

    GpuTelemetryCapBits IntelPowerTelemetryAdapter::GetPsuTelemetryCapBit(
        uint32_t psu_index) {
        switch (psu_index) {
            case 0:
                return GpuTelemetryCapBits::psu_info_0;
            case 1:
                return GpuTelemetryCapBits::psu_info_1;
            case 2:
                return GpuTelemetryCapBits::psu_info_2;
            case 3:
                return GpuTelemetryCapBits::psu_info_3;
            case 4:
                return GpuTelemetryCapBits::psu_info_4;
            default:
                throw std::runtime_error{"Invalid PSU index"};
        }
    }

    }
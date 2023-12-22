// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "RawFrameDataWriter.h"
#include <Core/source/infra/util/Util.h>
#include <Core/source/infra/util/Assert.h>
#include <CommonUtilities/source/str/String.h>
#include "EnumMap.h"
#include <format>
#include <array>

namespace p2c::pmon
{
    using ::pmon::util::str::ToNarrow;

    // TODO: output NA for metrics without availability
    struct Annotation_
    {
        virtual ~Annotation_() = default;
        virtual void Write(std::ostream& out, const uint8_t* pBytes) const = 0;
        std::string columnName;
        static std::unique_ptr<Annotation_> MakeTyped(PM_METRIC metricId, uint32_t index, const pmapi::intro::Root& introRoot);
    };
    template<typename T>
    struct TypedAnnotation_ : public Annotation_
    {
        void Write(std::ostream& out, const uint8_t* pBytes) const override
        {
            if constexpr (std::same_as<T, const char*>) {
                out << reinterpret_cast<T>(pBytes);
            }
            else {
                out << *reinterpret_cast<const T*>(pBytes);
            }
        }
    };
    template<>
    struct TypedAnnotation_<void> : public Annotation_
    {
        void Write(std::ostream& out, const uint8_t* pBytes) const override
        {
            out << "NA";
        }
    };
    template<>
    struct TypedAnnotation_<PM_ENUM> : public Annotation_
    {
        TypedAnnotation_(PM_ENUM enumId) : pKeyMap{ EnumMap::GetMapPtr(enumId) } {}
        void Write(std::ostream& out, const uint8_t* pBytes) const override
        {
            out << ToNarrow(pKeyMap->at(*reinterpret_cast<const int*>(pBytes)));
        }
        const EnumMap::KeyMap* pKeyMap = nullptr;
    };
    std::unique_ptr<Annotation_> Annotation_::MakeTyped(PM_METRIC metricId, uint32_t index, const pmapi::intro::Root& introRoot)
    {
        const auto metric = introRoot.FindMetric(metricId);
        const auto typeId = metric.GetDataTypeInfo().GetFrameType();
        switch (typeId) {
        case PM_DATA_TYPE_BOOL: return std::make_unique<TypedAnnotation_<bool>>();
        case PM_DATA_TYPE_INT32: return std::make_unique<TypedAnnotation_<int32_t>>();
        case PM_DATA_TYPE_UINT32: return std::make_unique<TypedAnnotation_<uint32_t>>();
        case PM_DATA_TYPE_UINT64: return std::make_unique<TypedAnnotation_<uint64_t>>();
        case PM_DATA_TYPE_DOUBLE: return std::make_unique<TypedAnnotation_<double>>();
        case PM_DATA_TYPE_STRING: return std::make_unique<TypedAnnotation_<const char*>>();
        case PM_DATA_TYPE_ENUM: return std::make_unique<TypedAnnotation_<PM_ENUM>>(
                metric.GetDataTypeInfo().GetEnumId());
        default: return std::make_unique<TypedAnnotation_<void>>();
        }
    }

    class QueryElementContainer_
    {
    public:
        // types
        struct ElementDefinition
        {
            PM_METRIC metricId;
            uint32_t deviceId;
            std::optional<uint32_t> index;
        };
        // functions
        QueryElementContainer_(std::span<const ElementDefinition> elements, uint32_t nBlobsToCreate,
            pmapi::Session& session, const pmapi::intro::Root& introRoot)
            :
            nBlobs{ nBlobsToCreate }
        {
            for (auto& el : elements) {
                const auto metric = introRoot.FindMetric(el.metricId);
                // set availability (defaults to false, if we find matching device use its availability)
                bool available = false;
                for (auto di : metric.GetDeviceMetricInfo()) {
                    if (di.GetDevice().GetId() != el.deviceId) continue;
                    available = di.IsAvailable();
                }
                if (available) {
                    annotationPtrs_.push_back(Annotation_::MakeTyped(el.metricId, el.deviceId, introRoot));
                }
                else {
                    // void annotation means Not Available
                    annotationPtrs_.push_back(std::make_unique<TypedAnnotation_<void>>());
                }
                // doing the name composition here instead of in MakeTyped to work around compiler bug
                // remove space from metric name
                annotationPtrs_.back()->columnName = metric.Introspect().GetName() |
                    std::views::filter([](char c) { return c != ' '; }) |
                    std::ranges::to<std::basic_string>();
                // append index if array metric
                if (el.index.has_value()) {
                    annotationPtrs_.back()->columnName += std::format("[{}]", *el.index);
                }
                queryElements_.push_back(PM_QUERY_ELEMENT{
                    .metric = el.metricId,
                    .stat = PM_STAT_NONE,
                    .deviceId = el.deviceId,
                    .arrayIndex = el.index.value_or(0),
                });
            }
            pQuery = session.RegisterFrameQuery(queryElements_);
            pBlobs = std::make_unique<uint8_t[]>(pQuery->GetBlobSize() * nBlobs);
        }
        void WriteFrames(uint32_t pid, const std::string& procName, std::ostream& out)
        {
            auto nBlobsInOut = nBlobs;
            // if consume sets nblobs to max, it means there (might) be more, so go again
            while (nBlobsInOut == nBlobs) {
                pQuery->Consume(pid, pBlobs.get(), nBlobsInOut);
                // use outparam from Consume to determin how many blobs to loop over
                const auto pEnd = pBlobs.get() + pQuery->GetBlobSize() * nBlobsInOut;
                for (auto pBlob = pBlobs.get(); pBlob < pEnd; pBlob += pQuery->GetBlobSize()) {
                    // process details are hardcoded here
                    out << procName << ',' << pid;
                    // loop over each element (column/field) in a frame of data
                    for (auto&& [pAnno, query] : std::views::zip(annotationPtrs_, queryElements_)) {
                        out << ',';
                        // using output from the query registration of get offset of column's data
                        const auto pBytes = pBlob + query.dataOffset;
                        // annotation contains polymorphic info to reinterpret and convert bytes
                        pAnno->Write(out, pBytes);
                    }
                    out << "\n";
                }
            }
            out << std::flush;
        }
        void WriteHeader(std::ostream& out)
        {
            out << "ProcessName,ProcessID";
            for (const auto& pAnno : annotationPtrs_) {
                out << ',' << pAnno->columnName;
            }
            out << std::endl;
        }
    private:
        std::shared_ptr<pmapi::FrameQuery> pQuery;
        std::unique_ptr<uint8_t[]> pBlobs;
        uint32_t nBlobs;
        std::vector<std::unique_ptr<Annotation_>> annotationPtrs_;
        std::vector<PM_QUERY_ELEMENT> queryElements_;
    };

    RawFrameDataWriter::RawFrameDataWriter(std::wstring path, uint32_t processId, std::wstring processName,
        pmapi::Session& session, std::optional<std::wstring> frameStatsPathIn, const pmapi::intro::Root& introRoot)
        :
        pid{ processId },
        procName{ ToNarrow(processName) },
        frameStatsPath{ std::move(frameStatsPathIn) },
        pStatsTracker{ frameStatsPath ? std::make_unique<StatisticsTracker>() : nullptr },
        file{ path }
    {
        using Element = QueryElementContainer_::ElementDefinition;

        const uint32_t activeDeviceId = 1;

        std::array queryElements{
            Element{.metricId = PM_METRIC_SWAP_CHAIN, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PRESENT_RUNTIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_SYNC_INTERVAL, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PRESENT_FLAGS, .deviceId = 0 },
            Element{.metricId = PM_METRIC_DROPPED_FRAMES, .deviceId = 0 },
            Element{.metricId = PM_METRIC_TIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_TIME_IN_PRESENT_API, .deviceId = 0 },
            Element{.metricId = PM_METRIC_TIME_BETWEEN_PRESENTS, .deviceId = 0 },
            Element{.metricId = PM_METRIC_ALLOWS_TEARING, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PRESENT_MODE, .deviceId = 0 },
            Element{.metricId = PM_METRIC_TIME_UNTIL_RENDER_COMPLETE, .deviceId = 0 },
            Element{.metricId = PM_METRIC_TIME_UNTIL_DISPLAYED, .deviceId = 0 },
            Element{.metricId = PM_METRIC_TIME_BETWEEN_DISPLAY_CHANGE, .deviceId = 0 },
            Element{.metricId = PM_METRIC_TIME_UNTIL_RENDER_START, .deviceId = 0 },
            Element{.metricId = PM_METRIC_GPU_BUSY_TIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_GPU_VIDEO_BUSY_TIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_TIME_SINCE_INPUT, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PRESENT_QPC, .deviceId = 0 },

            Element{.metricId = PM_METRIC_GPU_POWER, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_VOLTAGE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_FREQUENCY, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_TEMPERATURE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_UTILIZATION, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEDIA_UTILIZATION, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_POWER, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_VOLTAGE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_FREQUENCY, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_EFFECTIVE_FREQUENCY, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_TEMPERATURE, .deviceId = activeDeviceId },
            // Element{.metricId = PM_METRIC_GPU_MEM_SIZE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_USED, .deviceId = activeDeviceId },
            // Element{.metricId = PM_METRIC_GPU_MEM_MAX_BANDWIDTH, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_READ_BANDWIDTH, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_WRITE_BANDWIDTH, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_FAN_SPEED, .deviceId = activeDeviceId, .index = 0 },
            Element{.metricId = PM_METRIC_GPU_FAN_SPEED, .deviceId = activeDeviceId, .index = 1 },
            Element{.metricId = PM_METRIC_GPU_FAN_SPEED, .deviceId = activeDeviceId, .index = 2 },
            Element{.metricId = PM_METRIC_GPU_FAN_SPEED, .deviceId = activeDeviceId, .index = 3 },
            Element{.metricId = PM_METRIC_GPU_POWER_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_TEMPERATURE_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_CURRENT_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_VOLTAGE_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_UTILIZATION_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_POWER_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_TEMPERATURE_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_CURRENT_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_VOLTAGE_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_VRAM_UTILIZATION_LIMITED, .deviceId = activeDeviceId },

            Element{.metricId = PM_METRIC_CPU_UTILIZATION, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_CPU_POWER, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_CPU_TEMPERATURE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_CPU_FREQUENCY, .deviceId = activeDeviceId },
        };
        pQueryElementContainer = std::make_unique<QueryElementContainer_>(
            queryElements, numberOfBlobs, session, introRoot);
                
        // write header
        pQueryElementContainer->WriteHeader(file);
    }

    void RawFrameDataWriter::Process()
    {
        pQueryElementContainer->WriteFrames(pid, procName, file);
    }

    double RawFrameDataWriter::GetDuration_() const
    {
        return endTime - startTime;
    }

    void RawFrameDataWriter::WriteStats_()
    {
        auto& stats = *pStatsTracker;

        std::ofstream statsFile{ *frameStatsPath, std::ios::trunc };

        // write header
        statsFile <<
            "Duration,"
            "Total Frames,"
            "Average FPS,"
            "Minimum FPS,"
            "99th Percentile FPS,"
            "95th Percentile FPS,"
            "Maximum FPS\n";

        // lambda to make sure we don't divide by zero
        // caps max fps output to 1,000,000 fps
        const auto SafeInvert = [](double ft) {
            return ft == 0. ? 1'000'000. : 1. / ft;
		};

        if (stats.GetCount() > 0) {
            // write data
            statsFile <<
                GetDuration_() << "," <<
                stats.GetCount() << "," <<
                SafeInvert(stats.GetMean()) << "," <<
                SafeInvert(stats.GetMax()) << "," <<
                SafeInvert(stats.GetPercentile(.99)) << "," <<
                SafeInvert(stats.GetPercentile(.95)) << "," <<
                SafeInvert(stats.GetMin()) << "\n";
        }
        else {
			// write null data
			statsFile <<
				0. << "," <<
				0. << "," <<
				0. << "," <<
				0. << "," <<
				0. << "," <<
				0. << "," <<
				0. << "\n";
        }
    }

    RawFrameDataWriter::~RawFrameDataWriter()
    {
        try {
            if (pStatsTracker) {
                WriteStats_();
            }
        }
        catch (...) {}
    }
}
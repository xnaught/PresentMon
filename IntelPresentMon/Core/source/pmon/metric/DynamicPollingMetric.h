// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <PresentMonAPI/PresentMonAPI.h>
#include <PresentMonAPI2/source/PresentMonAPI.h>
#include <Core/source/gfx/layout/GraphData.h>
#include "Metric.h"
#include "../CachingQuery.h"
#include "../Timekeeper.h"
#include <concepts>

namespace p2c::pmon::met
{
    // TODO: derive strings using reference to intro root
    class DynamicPollingMetric : public Metric
    {
    public:
        DynamicPollingMetric(PM_METRIC metricId_, uint32_t deviceId_, uint32_t arrayIndex_, PM_STAT stat_,
            std::wstring name_, std::wstring units_);
        std::wstring GetStatName() const override;
        const std::wstring& GetCategory() const override;
        PM_QUERY_ELEMENT MakeQueryElement() const;
    protected:
        PM_METRIC metricId;
        PM_STAT stat;
        uint32_t deviceId;
        uint32_t arrayIndex;
    };

    // TODO: idea; don't template metric, just template a polymorphic type deserializer
    // then we can encapsulate the PM_DATA_TYPE => static type conversion switch here
    template<typename T>
    class TypedDynamicPollingMetric : public DynamicPollingMetric
    {
    public:
        TypedDynamicPollingMetric(const DynamicPollingMetric& mold, CachingQuery* pQuery_, size_t offset_)
            :
            DynamicPollingMetric{ mold },
            pQuery{ pQuery_ },
            offset{ offset_ }
        {}
        void PopulateData(gfx::lay::GraphData& graphData, double timestamp) override
        {
            graphData.Push(gfx::lay::DataPoint{ .value = ReadValue(timestamp), .time = timestamp });
            graphData.Trim(timestamp);
        }
        const std::wstring& GetMetricClassName() const override
        {
            if constexpr (std::integral<T> || std::floating_point<T>) {
                static std::wstring cls = L"Numeric";
                return cls;
            }
            else {
                static std::wstring cls = L"Text";
                return cls;
            }
        }
        std::optional<float> ReadValue(double timestamp) override
        {
            if constexpr (std::integral<T> || std::floating_point<T>) {
                auto pBlob = pQuery->Poll(timestamp);
                return (float)*reinterpret_cast<const T*>(&pBlob[offset]);
            }
            else {
                return 0.f;
            }
        }

    private:
        CachingQuery* pQuery = nullptr;
        size_t offset;
    };

}
// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <PresentMonAPI/PresentMonAPI.h>
#include <PresentMonAPI2/source/PresentMonAPI.h>
#include <Core/source/gfx/layout/GraphData.h>
#include <CommonUtilities/source/str/String.h>
#include "Metric.h"
#include "../CachingQuery.h"
#include "../Timekeeper.h"
#include <concepts>

namespace p2c::pmon::met
{
    namespace
    {
        using EnumKeyMap = std::unordered_map<int, std::wstring>;
        using ::pmon::util::str::ToWide;
    }
    // TODO: derive strings using reference to intro root
    class DynamicPollingMetric : public Metric
    {
    public:
        DynamicPollingMetric(PM_METRIC metricId_, uint32_t arrayIndex_, PM_STAT stat_,
            const pmapi::intro::Root& introRoot);
        std::wstring GetStatName() const override;
        const std::wstring& GetCategory() const override;
        const std::wstring& GetMetricClassName() const override;
        void PopulateData(gfx::lay::GraphData& graphData, double timestamp) override
        {
            graphData.Push(gfx::lay::DataPoint{ .value = ReadValue(timestamp), .time = timestamp });
            graphData.Trim(timestamp);
        }
        std::optional<float> ReadValue(double timestamp) override { return {};}
        PM_QUERY_ELEMENT MakeQueryElement() const;
        void Finalize(uint32_t offset);
        static void InitEnumMap(const pmapi::intro::Root& introRoot);
        std::unique_ptr<DynamicPollingMetric> RealizeMetric(const pmapi::intro::Root& introRoot,
            CachingQuery* pQuery, uint32_t activeGpuDeviceId);
        static uint32_t CalculateMaxArrayIndex(PM_METRIC metricId, const pmapi::intro::Root& introRoot);
    protected:
        // data
        PM_METRIC metricId;
        PM_STAT stat;
        uint32_t deviceId = 0;
        uint32_t arrayIndex;
        std::wstring statName;
        bool numeric = true;
        std::optional<uint32_t> offset;
    private:
        // functions
        static std::wstring MakeMetricName_(PM_METRIC metricId, uint32_t arrayIndex, const pmapi::intro::Root& introRoot);
        // data
    };

    // TODO: idea: don't template metric, just template a polymorphic type deserializer
    // then we can encapsulate the PM_DATA_TYPE => static type conversion switch here
    template<typename T>
    class TypedDynamicPollingMetric : public DynamicPollingMetric
    {
    public:
        TypedDynamicPollingMetric(const DynamicPollingMetric& mold, CachingQuery* pQuery_, uint32_t deviceId_)
            :
            DynamicPollingMetric{ mold },
            pQuery{ pQuery_ }
        {
            deviceId = deviceId_;
        }
        std::optional<float> ReadValue(double timestamp) override
        {
            if constexpr (std::integral<T> || std::floating_point<T>) {
                if (!offset) {
                    throw std::runtime_error{ "Metric not finalized" };
                }
                auto pBlob = pQuery->Poll(timestamp);
                return (float)*reinterpret_cast<const T*>(&pBlob[*offset]);
            }
            else {
                return {};
            }
        }
        std::wstring ReadStringValue(double timestamp) override
        {
            if (!offset) {
                throw std::runtime_error{ "Metric not finalized" };
            }
            if constexpr (std::integral<T> || std::floating_point<T>) {
                return Metric::ReadStringValue(timestamp);
            }
            else if constexpr (std::same_as<T, const char*>) {
                auto pBlob = pQuery->Poll(timestamp);
                return ToWide(reinterpret_cast<const char*>(&pBlob[*offset]));
            }
            else {
                p2clog.warn(L"Unknown type").commit();
                return {};
            }
        }
    private:
        CachingQuery* pQuery = nullptr;
    };

    template<>
    class TypedDynamicPollingMetric<PM_ENUM> : public DynamicPollingMetric
    {
    public:
        TypedDynamicPollingMetric(const DynamicPollingMetric& mold, CachingQuery* pQuery_, uint32_t deviceId_,
            const EnumKeyMap* pMap)
            :
            DynamicPollingMetric{ mold },
            pQuery{ pQuery_ },
            pKeyMap{ pMap }
        {
            deviceId = deviceId_;
        }
        std::optional<float> ReadValue(double timestamp) override
        {
            return {};
        }
        std::wstring ReadStringValue(double timestamp) override
        {
            if (!offset) {
                throw std::runtime_error{ "Metric not finalized" };
            }
            auto pBlob = pQuery->Poll(timestamp);
            return pKeyMap->at(*reinterpret_cast<const int*>(&pBlob[*offset]));
        }
    private:
        CachingQuery* pQuery = nullptr;
        const EnumKeyMap* pKeyMap = nullptr;
    };
}
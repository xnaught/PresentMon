// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include <Core/source/kernel/Kernel.h>
#include "../CefValues.h"

namespace p2c::client::util::async
{
    class Introspect : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "introspect"; }
        Introspect() : AsyncEndpoint{ AsyncEndpoint::Environment::KernelTask } {}
        // {} => {
        //     metrics: [{id: number, name: string, description: string, availableDeviceIds: number[], preferredUnitId: number, arraySize: number, availableStatIds: number[], numeric: boolean}],
        //     stats: [{id: number, name: string, shortName: string, description: string}],
        //     units: [{id: number, name: string, shortName: string, description: string}],
        // }
        Result ExecuteOnKernelTask(uint64_t uid, CefRefPtr<CefValue> pArgObj, kern::Kernel& kernel) const override
        {
            using namespace std::string_literals;
            // basic dummy device list
            auto deviceListCef0 = MakeCefList(1);
            deviceListCef0->SetInt(0, 0);
            auto deviceListCef1 = MakeCefList(1);
            deviceListCef1->SetInt(0, 0);
            auto deviceListCef2 = MakeCefList(1);
            deviceListCef2->SetInt(0, 0);
            // make available stat lists
            auto as0 = MakeCefList(3);
            as0->SetInt(0, 0);
            as0->SetInt(1, 1);
            as0->SetInt(2, 2);
            auto as1 = MakeCefList(2);
            as1->SetInt(0, 0);
            as1->SetInt(1, 1);
            auto as2 = MakeCefList(2);
            as2->SetInt(0, 1);
            as2->SetInt(1, 2);
            // metric list
            auto metricListCef = MakeCefList(3);
            metricListCef->SetValue(0, MakeCefObject(
                CefProp{ "id", 0 },
                CefProp{ "name", "chupa"s},
                CefProp{ "description", "itschupa"s },
                CefProp{ "availableDeviceIds", CefValueDecay(std::move(deviceListCef0)) },
                CefProp{ "preferredUnitId", 0 },
                CefProp{ "arraySize", 1 },
                CefProp{ "availableStatIds", CefValueDecay(as0) },
                CefProp{ "numeric", true }
            ));
            metricListCef->SetValue(1, MakeCefObject(
                CefProp{ "id", 1 },
                CefProp{ "name", "dhupa"s },
                CefProp{ "description", "itsdhupa"s },
                CefProp{ "availableDeviceIds", CefValueDecay(std::move(deviceListCef1)) },
                CefProp{ "preferredUnitId", 0 },
                CefProp{ "arraySize", 1 },
                CefProp{ "availableStatIds", CefValueDecay(as1) },
                CefProp{ "numeric", true }
            ));
            metricListCef->SetValue(2, MakeCefObject(
                CefProp{ "id", 2 },
                CefProp{ "name", "fhupa"s },
                CefProp{ "description", "itsfhupa"s },
                CefProp{ "availableDeviceIds", CefValueDecay(std::move(deviceListCef2)) },
                CefProp{ "preferredUnitId", 0 },
                CefProp{ "arraySize", 1 },
                CefProp{ "availableStatIds", CefValueDecay(as2) },
                CefProp{ "numeric", true }
            ));
            // stat list
            auto statListCef = MakeCefList(3);
            statListCef->SetValue(0, MakeCefObject(
                CefProp{ "id", 0 },
                CefProp{ "name", "carto"s },
                CefProp{ "shortName", "ct"s },
                CefProp{ "description", "itscarto"s }
            ));
            statListCef->SetValue(1, MakeCefObject(
                CefProp{ "id", 1 },
                CefProp{ "name", "carto2"s },
                CefProp{ "shortName", "ct2"s },
                CefProp{ "description", "itscarto2"s }
            ));
            statListCef->SetValue(2, MakeCefObject(
                CefProp{ "id", 2 },
                CefProp{ "name", "carto3"s },
                CefProp{ "shortName", "ct3"s },
                CefProp{ "description", "itscarto3"s }
            ));
            // unit list
            auto unitListCef = MakeCefList(0);

            //auto metricList = kernel.EnumerateMetrics();
            //auto metricListCef = MakeCefList(metricList.size());
            //for (int i = 0; i < metricList.size(); i++)
            //{
            //    auto& metric = metricList[i];
            //    metricListCef->SetValue(i, MakeCefObject(
            //        CefProp{ "category", std::move(metric.category) },
            //        CefProp{ "name", std::move(metric.name) },
            //        CefProp{ "statType", std::move(metric.statType) },
            //        CefProp{ "units", std::move(metric.units) },
            //        CefProp{ "index", (int)metric.index },
            //        CefProp{ "className", std::move(metric.className) }
            //    ));
            //}

            return { true, MakeCefObject(
                CefProp{ "metrics", CefValueDecay(std::move(metricListCef)) },
                CefProp{ "stats", CefValueDecay(std::move(statListCef)) },
                CefProp{ "units", CefValueDecay(std::move(unitListCef)) }
            ) };
        }
    };
}
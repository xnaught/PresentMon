// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { Metric } from "./metric"

export interface QualifiedMetric {
    metricId: number,
    arrayIndex: number,
    statId: number,
    deviceId: number,
    desiredUnitId: number,
}

export function makeDefaultQualifiedMetric(): QualifiedMetric {
    // HARDCODED: introspectable metric data
    return {
        metricId: 0,
        arrayIndex: 0,
        statId: 0,
        deviceId: 0,
        desiredUnitId: 0,
    };
}
// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
export interface Metric {
    id: number,
    name: string,
    description: string,
    availableStatIds: number[],
    preferredUnitId: number,
    arraySize: number,
    availableDeviceIds: number[],
    numeric: boolean,
}
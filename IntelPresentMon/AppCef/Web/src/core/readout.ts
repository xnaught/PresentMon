// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { Widget, WidgetType, GenerateKey } from './widget'
import { makeDefaultWidgetMetric } from './widget-metric';
import { RgbaColor } from './color';

export interface Readout extends Widget {
    showLabel: boolean,
    fontSize: number,
    fontColor: RgbaColor,
    backgroundColor: RgbaColor,
}

export function makeDefaultReadout(metricId: number): Readout {
    return {
        key: GenerateKey(),
        metrics: [makeDefaultWidgetMetric(metricId)],
        widgetType: WidgetType.Readout,
        showLabel: true,
        fontSize: 12,
        fontColor: {
            r: 205,
            g: 211,
            b: 233,
            a: 1
        },
        backgroundColor: {
            r: 45,
            g: 50,
            b: 96,
            a: 0.4
        },
    };
}
// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { Widget, WidgetType, GenerateKey } from './widget'
import { makeDefaultWidgetMetric } from './widget-metric';
import { QualifiedMetric } from './qualified-metric';
import { RgbaColor } from './color';
import { compareVersions } from './signature';

export interface Readout extends Widget {
    showLabel: boolean,
    fontSize: number,
    fontColor: RgbaColor,
    backgroundColor: RgbaColor,
}

export function makeDefaultReadout(metric: QualifiedMetric|null = null): Readout {
    return {
        key: GenerateKey(),
        metrics: [makeDefaultWidgetMetric(metric)],
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

interface Migration {
    version: string;
    migrate: (readout: Readout) => void;
}

const migrations: Migration[] = [
    {
        version: '0.10.0',
        migrate: (readout: Readout) => {
            const def = makeDefaultReadout();
            readout.backgroundColor = def.backgroundColor;
        }
    },
    {
        version: '0.12.0',
        migrate: (readout: Readout) => {
            throw new Error('Cannot migrate loadouts below version 0.12.0');
        }
    },
];

migrations.sort((a, b) => compareVersions(a.version, b.version));

export function migrateReadout(readout: Readout, sourceVersion: string): void {
    for (const mig of migrations) {
        if (compareVersions(mig.version, sourceVersion) > 0) {
            mig.migrate(readout);
        }
    }
}
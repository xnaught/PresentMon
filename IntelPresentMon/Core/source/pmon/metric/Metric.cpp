// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Metric.h"
#include <format>

namespace p2c::pmon
{
    Metric::Metric(std::wstring name, std::wstring units) : name{ std::move(name) }, units{ std::move(units) } {}
    Metric::~Metric() = default;
    std::wstring Metric::ReadStringValue(double timestamp)
    {
        const auto val = ReadValue(timestamp);
        if (val) {
            const auto digitsBeforeDecimal = int(log10(std::abs(*val)));
            const int maxFractionalDigits = 2;
            return std::format(L"{:.{}f}", *val, std::max(maxFractionalDigits - digitsBeforeDecimal, 0));
        }
        return L"NA";
    }
    std::wstring Metric::GetStatName() const { return {}; }
    const std::wstring& Metric::GetName() const { return name; }
    const std::wstring& Metric::GetUnits() const { return units; }
    Metric::Info Metric::GetInfo(size_t index) const
    {
        return {
            index,
            GetCategory(),
            GetName(),
            GetStatName(),
            GetUnits(),
            GetMetricClassName()
        };
    }
}
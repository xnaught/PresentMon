// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <optional>

namespace p2c::pmon::met
{
    class MetricFetcher
    {
    public:
        virtual ~MetricFetcher();
        virtual std::wstring ReadStringValue();
        virtual std::optional<float> ReadValue() = 0;
    };
}
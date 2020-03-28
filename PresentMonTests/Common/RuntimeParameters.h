// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <unordered_map>
#pragma once
namespace RuntimeParameters
{
    // Runtime parameters passed through CLI arguments
    extern std::unordered_map<std::string, std::string> Parameters;
}
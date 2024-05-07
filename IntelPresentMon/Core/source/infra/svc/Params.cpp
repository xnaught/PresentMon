// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Params.h"
#include <CommonUtilities/str/String.h>

namespace p2c::infra::svc
{
    using namespace pmon::util;

    Params& Params::At(const std::string& key)
    {
        return Get<Params>(key);
    }

    const Params& Params::At(const std::string& key) const
    {
        return Get<Params>(key);
    }

    bool Params::IsEmpty() const noexcept
    {
        return container.empty();
    }

    Params::Params(std::initializer_list<ContainerType::value_type> elements)
        :
        container{ elements }
    {}

    std::any& Params::GetAny(const std::string& key)
    {
        try {
            return container.at(key);
        }
        catch (const std::out_of_range&) {
            pmlog_error(std::format(L"Params::Get unavailable key: {}", str::ToWide(key)));
            throw;
        }
    }

    const std::any& Params::GetAny(const std::string& key) const
    {
        return const_cast<Params*>(this)->GetAny(key);
    }
}
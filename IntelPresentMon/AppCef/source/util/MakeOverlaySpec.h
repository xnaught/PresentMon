// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <memory>

namespace p2c::kern
{
	struct OverlaySpec;
}

namespace p2c::client::util::kact::push_spec_impl
{
	struct Params;
}

namespace p2c::client::util
{
	std::unique_ptr<kern::OverlaySpec> MakeOverlaySpec(const kact::push_spec_impl::Params& spec);
}
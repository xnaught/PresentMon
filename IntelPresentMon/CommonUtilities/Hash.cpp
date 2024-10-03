// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Hash.h"
#include "win/WinAPI.h"


namespace pmon::util::hash
{
	size_t HashCombine(size_t lhs, size_t rhs) noexcept
	{
		return lhs ^ (rhs + 0x517c'c1b7'2722'0a95 + (lhs << 6) + (lhs >> 2));
	}

	size_t HashGuid(const _GUID& guid) noexcept
	{
		return HashCombine(reinterpret_cast<const size_t&>(guid.Data1),
			reinterpret_cast<const size_t&>(guid.Data4));
	}
}
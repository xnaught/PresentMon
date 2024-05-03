// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CefProcessCompass.h"
#include <Core/source/infra/svc/Services.h>
#include <Core/source/cli/CliOptions.h>

namespace p2c::client::util
{
	CefProcessCompass::CefProcessCompass()
	{
		if (cli::Options::Get().cefType)
		{
			type = *cli::Options::Get().cefType;
		}
	}

	const std::optional<std::string>& CefProcessCompass::GetType() const
	{
		return type;
	}

	bool CefProcessCompass::IsClient() const
	{
		return !type.has_value();
	}
}

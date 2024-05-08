// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "ServiceBooter.h"
#include <Core/source/infra/svc/Services.h>
#include <Core/source/cli/CliOptions.h>
#include <Core/source/infra/util/FolderResolver.h>
#include "CefProcessCompass.h"

void p2c::client::util::BootServices()
{
	using infra::svc::Services;
	auto&& cli = cli::Options::Get();
	
	if (cli.filesWorking)
	{
		Services::Singleton<infra::util::FolderResolver>([] { return std::make_shared<infra::util::FolderResolver>(); });
	}
	else
	{
		Services::Singleton<infra::util::FolderResolver>([] { return std::make_shared<infra::util::FolderResolver>(L"Intel\\PresentMon", L"PresentMon"); });
	}
	Services::Singleton<CefProcessCompass>([] { return std::make_shared<CefProcessCompass>(); });
}

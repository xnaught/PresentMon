// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/pmon/metric/Metric.h>
#include <Core/source/gfx/layout/GraphData.h>
#include <format>

namespace p2c::kern
{
	struct GraphDataPack
	{
		GraphDataPack(pmon::met::Metric* pMetric_, double windowSize_)
			:
			pMetric{ pMetric_ },
			pData{ std::make_shared<gfx::lay::GraphData>(windowSize_) }
		{}
		void Populate(double timestamp)
		{
			pMetric->PopulateData(*pData, timestamp);
		}
		std::wstring GetFullName() const
		{
			auto stat = pMetric->GetStatName();
			auto statPart = stat.empty() ? std::wstring{} : std::format(L" ({})", stat);
			return std::format(L"{}{}", pMetric->GetName(), statPart);
		}
		std::shared_ptr<gfx::lay::GraphData> pData;
		pmon::met::Metric* pMetric = nullptr;
	};
}
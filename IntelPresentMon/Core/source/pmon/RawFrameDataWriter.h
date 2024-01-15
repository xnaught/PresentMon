// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include "StatisticsTracker.h"
#include <PresentMonAPI/PresentMonAPI.h>
#include <PresentMonAPI2/source/PresentMonAPI.h>
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include <Core/source/infra/log/Logging.h>
#include <Core/source/infra/util/Util.h>

namespace p2c::pmon
{
	class QueryElementContainer_;

	class RawFrameDataWriter
	{
	public:
        RawFrameDataWriter(std::wstring path, uint32_t processId, std::wstring processName, pmapi::Session& session,
			std::optional<std::wstring> frameStatsPath, const pmapi::intro::Root& introRoot);
		RawFrameDataWriter(const RawFrameDataWriter&) = delete;
		RawFrameDataWriter& operator=(const RawFrameDataWriter&) = delete;
		void Process();
		~RawFrameDataWriter();
	private:
		// functions
		double GetDuration_() const;
		void WriteStats_();
		// data
		static constexpr uint32_t numberOfBlobs = 150u;
		uint32_t pid;
		std::string procName;
		std::unique_ptr<QueryElementContainer_> pQueryElementContainer;
		std::optional<std::wstring> frameStatsPath;
		std::unique_ptr<StatisticsTracker> pStatsTracker;
		pmapi::BlobContainer blobs;
		double startTime = -1.;
		double endTime = -1.;
		std::ofstream file;
	};
}

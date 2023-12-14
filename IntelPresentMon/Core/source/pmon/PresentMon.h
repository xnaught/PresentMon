// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <optional>
#include <vector>
#include <memory>
#include "metric/Metric.h"
#include "AdapterInfo.h"

namespace pmapi
{
	class Session;
	class ProcessTracker;
	namespace intro
	{
		class Root;
	}
}

namespace p2c::pmon
{
	class RawFrameDataWriter;

	class PresentMon
	{
	public:
		// functions
		PresentMon(std::optional<std::string> namedPipeName, std::optional<std::string> sharedMemoryName, double window = 1000., double offset = 1000., uint32_t telemetrySampleRateMs = 16);
		~PresentMon();
		void StartTracking(uint32_t pid_);
		void StopTracking();
		double GetWindow() const;
		void SetWindow(double window_);
		double GetOffset() const;
		void SetOffset(double offset_);
		void SetGpuTelemetryPeriod(uint32_t period);
		uint32_t GetGpuTelemetryPeriod();
		std::wstring GetCpuName() const;
		std::vector<AdapterInfo> EnumerateAdapters() const;
		void SetAdapter(uint32_t id);
		met::Metric* GetMetricByIndex(size_t index);
		std::vector<met::Metric::Info> EnumerateMetrics() const;
		std::optional<uint32_t> GetPid() const;
		std::shared_ptr<RawFrameDataWriter> MakeRawFrameDataWriter(std::wstring path, std::optional<std::wstring> statsPath);
		void FlushRawData();
		std::optional<uint32_t> GetSelectedAdapter() const;
	private:
		// functions
		void AddMetric(std::unique_ptr<met::Metric> metric_);
		void AddMetrics(std::vector<std::unique_ptr<met::Metric>> metrics_);
		// data
		double window = -1.;
		double offset = -1.;
		uint32_t telemetrySamplePeriod = 0;
		std::unique_ptr<pmapi::Session> pSession;
		std::vector<std::unique_ptr<met::Metric>> metrics;
		std::shared_ptr<pmapi::intro::Root> pIntrospectionRoot;
		std::shared_ptr<pmapi::ProcessTracker> pTracker;
		std::optional<uint32_t> selectedAdapter;
	};
}
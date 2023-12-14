// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "PresentMon.h"
#include <Core/source/infra/log/Logging.h>
#include <PresentMonAPI/PresentMonAPI.h>
#include <PresentMonAPI2/source/PresentMonAPI.h>
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include <Core/source/infra/util/Util.h>
#include "metric/NoisySineFakeMetric.h"
#include "metric/SimpleMetric.h"
#include "RawFrameDataWriter.h"
#include "metric/SquareWaveMetric.h"

namespace p2c::pmon
{
	PresentMon::PresentMon(std::optional<std::string> namedPipeName, std::optional<std::string> sharedMemoryName, double window_in, double offset_in, uint32_t telemetrySamplePeriodMs_in)
	{
		const auto RemoveDoubleQuotes = [](std::string s) {
			if (s.front() == '"' && s.back() == '"' && s.size() >= 2) {
				s = s.substr(1, s.size() - 2);
			}
			return s;
		};
		if (namedPipeName && sharedMemoryName) {
			auto pipeName = RemoveDoubleQuotes(*namedPipeName);
			auto shmName = RemoveDoubleQuotes(*sharedMemoryName);
			p2clog.info(std::format(L"Connecting to service with custom pipe [{}] and nsm [{}]",
				infra::util::ToWide(pipeName),
				infra::util::ToWide(shmName)
			)).commit();
			pSession = std::make_unique<pmapi::Session>(std::move(pipeName), std::move(shmName));
		}
		else {
			p2clog.info(L"Connecting to service with default pipe name").commit();
			pSession = std::make_unique<pmapi::Session>();
		}

		// acquire introspection data
		pIntrospectionRoot = pSession->GetIntrospectionRoot();

		// Build table of available metrics using introspection
		using namespace met;

		// fake metrics for testing
#ifdef _DEBUG
		AddMetric(std::make_unique<NoisySineFakeMetric>(L"Fake Noisy Sinusoid", 0.1f, 0.f, 30.f, 50.f, 3.f, 3.f));
		AddMetric(std::make_unique<NoisySineFakeMetric>(L"Fake Pure Sinusoid", 2.f, 0.f, 40.f, 50.f, 3.f, 0.f));
		AddMetric(std::make_unique<NoisySineFakeMetric>(L"Fake Slow Sinusoid", 0.05f, 0.f, 40.f, 50.f, 3.f, 0.f));
		AddMetric(std::make_unique<SquareWaveMetric>(L"Fake Square Wave", 1., 0.f, 100.f));
#endif

		// establish initial sampling / window / processing setting values
		SetWindow(window_in);
		SetOffset(offset_in);
		SetGpuTelemetryPeriod(telemetrySamplePeriodMs_in);
	}
	PresentMon::~PresentMon() = default;
	void PresentMon::StartTracking(uint32_t pid_)
	{
		if (pTracker) {
			if (pTracker->GetPid() == pid_) {
				return;
			}
			p2clog.warn(std::format(L"Starting stream [{}] while previous stream [{}] still active",
				pid_, pTracker->GetPid())).commit();
		}
		pTracker = pSession->TrackProcess(pid_);
		p2clog.info(std::format(L"started pmon stream for pid {}", pid_)).commit();
	}
	void PresentMon::StopTracking()
	{
		if (!pTracker) {
			p2clog.warn(L"Cannot stop stream: no stream active").commit();
			return;
		}
		const auto pid = pTracker->GetPid();
		pTracker.reset();
		// TODO: caches cleared here maybe
		p2clog.info(std::format(L"stopped pmon stream for pid {}", pid)).commit();
	}
	double PresentMon::GetWindow() const { return window; }
	void PresentMon::SetWindow(double window_) { window = window_; }
	double PresentMon::GetOffset() const { return offset; }
	void PresentMon::SetOffset(double offset_) { offset = offset_; }
	void PresentMon::SetGpuTelemetryPeriod(uint32_t period)
	{
		// TODO: implement telemetry period setting
		p2clog.info(std::format(L"Mocking call to set gpu telemetry period to {}", offset)).commit();
		//if (auto sta = pmSetGPUTelemetryPeriod(period); sta != PM_STATUS::PM_STATUS_SUCCESS)
		//{
		//	p2clog.warn(std::format(L"could not set gpu telemetry sample period to {}", offset)).code(sta).commit();
		//}
		//else
		//{
		//	telemetrySamplePeriod = period;
		//}
	}
	uint32_t PresentMon::GetGpuTelemetryPeriod()
	{
		return telemetrySamplePeriod;
	}
	std::wstring PresentMon::GetCpuName() const
	{
		char buffer[512];
		uint32_t bufferSize = sizeof(buffer);
		if (auto sta = pmGetCpuName(buffer, &bufferSize); sta != PM_STATUS_SUCCESS) {
			p2clog.warn(L"could not get cpu name").code(sta).commit();
			return {};
		}
		if (bufferSize >= sizeof(buffer)) {
			p2clog.warn(std::format(L"insufficient buffer size to get cpu name. written: {}", bufferSize)).commit();
		}
		return infra::util::ToWide(std::string{ buffer, bufferSize });
	}
	met::Metric* PresentMon::GetMetricByIndex(size_t index) { return metrics.at(index).get(); };
	std::vector<met::Metric::Info> PresentMon::EnumerateMetrics() const
	{
		std::vector<met::Metric::Info> info;
		size_t index = 0;
		for (auto& m : metrics)
		{
			info.push_back(m->GetInfo(index));
			index++;
		}
		return info;
	}
	std::vector<AdapterInfo> PresentMon::EnumerateAdapters() const
	{
		std::vector<AdapterInfo> infos;
		for (const auto& info : pIntrospectionRoot->GetDevices()) {
			if (info.GetBasePtr()->type != PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
				continue;
			}
			infos.push_back(AdapterInfo{
				.id = info.GetId(),
				.vendor = info.GetVendor().GetName(),
				.name = info.GetName(),
			});
		}
		return infos;
	}
	void PresentMon::SetAdapter(uint32_t id)
	{
		if (auto sta = pmSetActiveAdapter(id); sta != PM_STATUS::PM_STATUS_SUCCESS) {
			p2clog.note(L"could not set active adapter").code(sta).nox().commit();
		}
		else {
			selectedAdapter = id;
		}
	}
	std::optional<uint32_t> PresentMon::GetPid() const {
		return bool(pTracker) ? pTracker->GetPid() : std::optional<uint32_t>{};
	}
	std::shared_ptr<RawFrameDataWriter> PresentMon::MakeRawFrameDataWriter(std::wstring path, std::optional<std::wstring> statsPath)
	{
		return {};
		// return std::make_shared<RawFrameDataWriter>(std::move(path), &rawAdaptor, std::move(statsPath));
	}
	void PresentMon::FlushRawData()
	{
		//for (double t = -1.;; t -= 1.) {
		//	if (rawAdaptor.Pull(t).empty()) break;
		//}
	}
	std::optional<uint32_t> PresentMon::GetSelectedAdapter() const
	{
		return selectedAdapter;
	}
	void PresentMon::AddMetric(std::unique_ptr<met::Metric> metric_)
	{
		metrics.push_back(std::move(metric_));
	}
	void PresentMon::AddMetrics(std::vector<std::unique_ptr<met::Metric>> metrics_)
	{
		metrics.insert(
			metrics.end(),
			std::make_move_iterator(metrics_.begin()),
			std::make_move_iterator(metrics_.end())
		);
	}
}
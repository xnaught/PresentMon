#pragma once
#include "DataFetchPack.h"
#include "../pmon/MetricFetcherFactory.h"
#include "../pmon/DynamicQuery.h"
#include "OverlaySpec.h"
#include <CommonUtilities\source\hash\Hash.h>
#include <unordered_map>
#include <ranges>

namespace std
{
	template<>
	struct hash<p2c::kern::QualifiedMetric>
	{
		size_t operator()(const p2c::kern::QualifiedMetric& q) const
		{
			using namespace pmon::util::hash;
			return HashCombine(
				DualHash(q.metricId, q.arrayIndex),
				DualHash(q.deviceId, q.statId)
			);
		}
	};
}

namespace p2c::kern
{
	namespace rn = std::ranges;
	namespace vi = std::views;
	// maps QualifiedMetrics to packs (data sinks + sources)
	// interacts with MetricFetcherFactory (outputs range of QualifiedMetrics, inputs Fetchers and poll obj)
	// manages carry-over of graph data buffers between pushes while refreshing query / fetchers
	// exposes interface that polls sources and fetches data from them into buffers
	class MetricPackMapper
	{
	public:
		void CommitChanges(uint32_t pid, uint32_t activeGpuDeviceId, double winSizeMs, double metricOffsetMs, pmon::MetricFetcherFactory& factory)
		{
			// eliminate data / packs which are not necessary anymore
			for (auto&& [qmet, pPack] : metricPackMap_) {
				if (auto&& i = usageMap_.find(qmet); i != usageMap_.end()) {
					if (!i->second.graph) {
						pPack.graphData.reset();
					}
					if (!i->second.text) {
						pPack.textData.reset();
					}
				}
				else {
					pPack.graphData.reset();
					pPack.textData.reset();
				}
			}
			std::erase_if(metricPackMap_, [](const auto& e) { return !(e.second.graphData || e.second.textData); });
			usageMap_.clear();
			// build vector of qmet
			const auto qualifiedMetrics = metricPackMap_ | vi::keys | rn::to<std::vector>();
			// build fetchers
			auto buildResult = factory.Build(pid, activeGpuDeviceId, winSizeMs, metricOffsetMs, qualifiedMetrics);
			// fill fetchers into map
			for (auto&& [qmet, pFetcher] : buildResult.fetchers) {
				metricPackMap_[qmet].pFetcher = std::move(pFetcher);
			}
			// move query
			pQuery_ = std::move(buildResult.pQuery);
		}
		void AddGraph(const QualifiedMetric& qmet, double timeWindow)
		{
			auto& pPack = metricPackMap_[qmet];
			if (!pPack.graphData) {
				pPack.graphData = std::make_shared<gfx::lay::GraphData>(timeWindow);
			}
			else if (pPack.graphData->GetWindowSize() != timeWindow) {
				pPack.graphData->Resize(timeWindow);
			}
			usageMap_[qmet].graph = true;
		}
		void AddReadout(const QualifiedMetric& qmet)
		{
			auto& pPack = metricPackMap_[qmet];
			if (!pPack.textData) {
				pPack.textData = std::make_shared<std::wstring>();
			}
			usageMap_[qmet].text = true;
		}
		void Populate(double timestamp)
		{
			for (auto&& [qmet, pPack] : metricPackMap_) {
				pPack.Populate(timestamp);
			}
		}
		DataFetchPack& operator[](const QualifiedMetric& qmet)
		{
			return metricPackMap_.at(qmet);
		}
	private:
		// types
		struct MetricUsage_
		{
			bool graph = false;
			bool text = false;
		};
		// data
		std::unordered_map<QualifiedMetric, DataFetchPack> metricPackMap_;
		// we might need a class that encapsulates all pollable sources, including DynamicQuery
		std::shared_ptr<pmon::DynamicQuery> pQuery_;
		// map used to determine which metrics are no longer needed after a push
		// i.e. which ones can carry over, differentiates between graph and readout
		std::unordered_map<QualifiedMetric, MetricUsage_> usageMap_;
	};
}
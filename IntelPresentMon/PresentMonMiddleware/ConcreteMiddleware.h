#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "Middleware.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Streamer/StreamClient.h"
#include <optional>
#include <string>
#include <queue>
#include "../CommonUtilities/Hash.h"

namespace pmapi::intro
{
	class Root;
}

namespace pmon::mid
{
	// Used to calculate correct start frame based on metric offset
	struct MetricOffsetData {
		uint64_t queryToFrameDataDelta = 0;
		uint64_t metricOffset = 0;
	};

    // Copied from: PresentMon/PresentMon.hpp
    // We store SwapChainData per process and per swapchain, where we maintain:
    // - information on previous presents needed for console output or to compute metrics for upcoming
    //   presents,
    // - pending presents whose metrics cannot be computed until future presents are received,
    // - exponential averages of key metrics displayed in console output.
	struct fpsSwapChainData {
        // Pending presents waiting for the next displayed present.
        std::vector<PmNsmPresentEvent> mPendingPresents;

        // The most recent present that has been processed (e.g., output into CSV and/or used for frame
        // statistics).
        PmNsmPresentEvent mLastPresent;
        bool mLastPresentIsValid = false;

        // Whether to include frame data in the next PresentEvent's FrameMetrics.
        bool mIncludeFrameData = true;

        // IntelPresentMon specifics:
        std::vector<double> mCPUBusy;
        std::vector<double> mCPUWait;
        std::vector<double> mGPULatency;
        std::vector<double> mGPUBusy;
        std::vector<double> mVideoBusy;
        std::vector<double> mGPUWait;
        std::vector<double> mDisplayLatency;
        std::vector<double> mDisplayedTime;
        std::vector<double> mAppDisplayedTime;
		std::vector<double> mAnimationError;
        std::vector<double> mClickToPhotonLatency;
		std::vector<double> mAllInputToPhotonLatency;
        std::vector<double> mDropped;
		std::vector<double> mInstrumentedDisplayLatency;

		std::vector<double> mInstrumentedSleep;
		std::vector<double> mInstrumentedRenderLatency;
		std::vector<double> mInstrumentedGpuLatency;
		std::vector<double> mInstrumentedReadyTimeToDisplayLatency;

		// QPC of last received input data that did not make it to the screen due 
		// to the Present() being dropped
		uint64_t mLastReceivedNotDisplayedAllInputTime;
		uint64_t mLastReceivedNotDisplayedMouseClickTime;

        // begin/end screen times to optimize average calculation:
		uint64_t mLastDisplayedScreenTime = 0;    // The last presented frame's ScreenTime (qpc)
		uint64_t display_0_screen_time = 0;       // The first presented frame's ScreenTime (qpc)
		uint64_t mLastDisplayedSimStart = 0;      // The simulation start of the last presented frame
		uint32_t display_count = 0;               // The number of presented frames
	};

	struct DeviceInfo
	{
		PM_DEVICE_VENDOR deviceVendor;
		std::string deviceName;
		uint32_t deviceId;
		std::optional<uint32_t> adapterId;
		std::optional<double> gpuSustainedPowerLimit;
		std::optional<uint64_t> gpuMemorySize;
		std::optional<uint64_t> gpuMemoryMaxBandwidth;
		std::optional<double> cpuPowerLimit;
	};

	struct MetricInfo
	{
		// Map of array indices to associated data
		std::unordered_map<uint32_t, std::vector<double>> data;
	};

	class ConcreteMiddleware : public Middleware
	{
	public:
		ConcreteMiddleware(std::optional<std::string> pipeNameOverride = {}, std::optional<std::string> introNsmOverride = {});
		~ConcreteMiddleware() override;
		void Speak(char* buffer) const override;
		const PM_INTROSPECTION_ROOT* GetIntrospectionData() override;
		void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) override;
		PM_STATUS StartStreaming(uint32_t processId) override;
		PM_STATUS StopStreaming(uint32_t processId) override;
		PM_STATUS SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t timeMs) override;
		PM_STATUS SetEtwFlushPeriod(std::optional<uint32_t> periodMs) override;
		PM_DYNAMIC_QUERY* RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, double windowSizeMs, double metricOffsetMs) override;
		void FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery) override {}
		void PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains) override;
		void PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob) override;
		PM_FRAME_QUERY* RegisterFrameEventQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t& blobSize) override;
		void FreeFrameEventQuery(const PM_FRAME_QUERY* pQuery) override;
		void ConsumeFrameEvents(const PM_FRAME_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t& numFrames) override;
	private:
		PmNsmFrameData* GetFrameDataStart(StreamClient* client, uint64_t& index, uint64_t dataOffset, uint64_t& queryFrameDataDelta, double& windowSampleSizeMs);
		uint64_t GetAdjustedQpc(uint64_t current_qpc, uint64_t frame_data_qpc, uint64_t queryMetricsOffset, LARGE_INTEGER frequency, uint64_t& queryFrameDataDelta);
		bool DecrementIndex(NamedSharedMem* nsm_view, uint64_t& index);
		PM_STATUS SetActiveGraphicsAdapter(uint32_t deviceId);
		void GetStaticGpuMetrics();

		void CalculateFpsMetric(fpsSwapChainData& swapChain, const PM_QUERY_ELEMENT& element, uint8_t* pBlob, LARGE_INTEGER qpcFrequency);
		void CalculateGpuCpuMetric(std::unordered_map<PM_METRIC, MetricInfo>& metricInfo, const PM_QUERY_ELEMENT& element, uint8_t* pBlob);
		double CalculateStatistic(std::vector<double>& inData, PM_STAT stat, bool invert = false) const;
		double CalculatePercentile(std::vector<double>& inData, double percentile, bool invert) const;
		bool GetGpuMetricData(size_t telemetry_item_bit, PresentMonPowerTelemetryInfo& power_telemetry_info, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo);
		bool GetCpuMetricData(size_t telemetryBit, CpuTelemetryInfo& cpuTelemetry, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo);
		void GetStaticCpuMetrics();
		std::string GetProcessName(uint32_t processId);
		void CopyStaticMetricData(PM_METRIC metric, uint32_t deviceId, uint8_t* pBlob, uint64_t blobOffset, size_t sizeInBytes = 0);

		void CalculateMetrics(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains, LARGE_INTEGER qpcFrequency, std::unordered_map<uint64_t, fpsSwapChainData>& swapChainData, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo);
		void SaveMetricCache(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob);
		void CopyMetricCacheToBlob(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob);

		std::optional<size_t> GetCachedGpuInfoIndex(uint32_t deviceId);

		const pmapi::intro::Root& GetIntrospectionRoot();

		std::shared_ptr<class ActionClient> pActionClient;
		uint32_t clientProcessId = 0;
		// Stream clients mapping to process id
		std::map<uint32_t, std::unique_ptr<StreamClient>> presentMonStreamClients;
		// App sim start time for each process id
		std::map<uint32_t, uint64_t> appSimStartTime;
		std::unique_ptr<ipc::MiddlewareComms> pComms;
		// Dynamic query handle to frame data delta
		std::unordered_map<std::pair<const PM_DYNAMIC_QUERY*, uint32_t>, uint64_t> queryFrameDataDeltas;
		// Dynamic query handle to cache data
		std::unordered_map<std::pair<const PM_DYNAMIC_QUERY*, uint32_t>, std::unique_ptr<uint8_t[]>> cachedMetricDatas;
		std::vector<DeviceInfo> cachedGpuInfo;
		std::vector<DeviceInfo> cachedCpuInfo;
		uint32_t currentGpuInfoIndex = UINT32_MAX;
		std::optional<uint32_t> activeDevice;
		std::unique_ptr<pmapi::intro::Root> pIntroRoot;
	};
}

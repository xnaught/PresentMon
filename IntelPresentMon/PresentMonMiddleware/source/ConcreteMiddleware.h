#pragma once
#include "../../PresentMonAPI/PresentMonAPI.h"
#include "Middleware.h"
#include "../../Interprocess/source/Interprocess.h"
#include "../../PresentMonUtils/MemBuffer.h"
#include "../../Streamer/StreamClient.h"

namespace pmon::mid
{
	// Used to calculate correct start frame based on metric offset
	struct MetricOffsetData {
		uint64_t queryToFrameDataDelta = 0;
		uint64_t metricOffset = 0;
	};

	class ConcreteMiddleware : public Middleware
	{
	public:
		ConcreteMiddleware();
		void Speak(char* buffer) const override;
		const PM_INTROSPECTION_ROOT* GetIntrospectionData() override { return nullptr; }
		void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) override {}
		PM_STATUS StartStreaming(uint32_t processId) override;
		PM_STATUS StopStreaming(uint32_t processId) override;
		PM_DYNAMIC_QUERY* RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t processId, double windowSizeMs, double metricOffsetMs) override;
		void FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery) override {}
		void PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint8_t* pBlob, uint32_t* numSwapChains) override;
		void PollStaticQuery(const PM_QUERY_ELEMENT& element, uint8_t* pBlob) override {}
	private:
		struct HandleDeleter {
			void operator()(HANDLE handle) const {
				// Custom deletion logic for HANDLE
				CloseHandle(handle);
			}
		};
		PM_STATUS SendRequest(MemBuffer* requestBuffer);
		PM_STATUS ReadResponse(MemBuffer* responseBuffer);
		PM_STATUS CallPmService(MemBuffer* requestBuffer, MemBuffer* responseBuffer);
		PmNsmFrameData* GetFrameDataStart(StreamClient* client, uint64_t& index, uint64_t dataOffset, uint64_t& queryFrameDataDelta, double& windowSampleSizeMs);
		uint64_t GetAdjustedQpc(uint64_t current_qpc, uint64_t frame_data_qpc, uint64_t queryMetricsOffset, LARGE_INTEGER frequency, uint64_t& queryFrameDataDelta);
		bool DecrementIndex(NamedSharedMem* nsm_view, uint64_t& index);

		std::unique_ptr<void, HandleDeleter> pNamedPipeHandle;
		uint32_t clientProcessId = 0;
		// Stream clients mapping to process id
		std::map<uint32_t, std::unique_ptr<StreamClient>> presentMonStreamClients;
		std::unique_ptr<ipc::MiddlewareComms> pComms;
		// Dynamic query handle to frame data delta
		std::map<PM_DYNAMIC_QUERY*, uint64_t> queryFrameDataDeltas;
	};
}
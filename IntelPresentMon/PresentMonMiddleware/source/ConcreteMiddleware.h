#pragma once
#include "Middleware.h"
#include "../../PresentMonUtils/MemBuffer.h"
#include "../../Streamer/StreamClient.h"

namespace pmon::mid
{
	class ConcreteMiddleware : public Middleware
	{
	public:
		ConcreteMiddleware();
		void Speak(char* buffer) const override;
		const PM_INTROSPECTION_ROOT* GetIntrospectionData() override { return nullptr; }
		void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) override {}
		PM_STATUS StartStreaming(uint32_t processId) override;
		PM_STATUS StopStreaming(uint32_t processId) override;
		PM_DYNAMIC_QUERY* RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, double windowSizeMs, double metricOffsetMs) override { return nullptr; }
		void FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery) override {}
		void PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint8_t* pBlob) override {}
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

		std::unique_ptr<void, HandleDeleter> pNamedPipeHandle;
		uint32_t clientProcessId = 0;
		// Stream clients mapping to process id
		std::map<uint32_t, std::unique_ptr<StreamClient>> presentMonStreamClients;
	};
}
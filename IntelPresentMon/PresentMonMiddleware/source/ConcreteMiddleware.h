#pragma once
#include "Middleware.h"
#include <Windows.h>
#include <memory>
#include "../../PresentMonUtils/MemBuffer.h"

namespace pmon::mid
{
	class ConcreteMiddleware : public Middleware
	{
	public:
		ConcreteMiddleware();
		void Speak(char* buffer) const override;
		const PM_INTROSPECTION_ROOT* GetIntrospectionData() override { return nullptr; }
		void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) override {}
		PM_STATUS OpenSession(uint32_t processId) override;
		PM_STATUS CloseSession(uint32_t processId) override;
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
		std::unique_ptr<HANDLE, HandleDeleter> uniqueHandleMember;
		std::unique_ptr<void, HandleDeleter> testHandle;
		PM_STATUS SendRequest(MemBuffer* requestBuffer);
		PM_STATUS ReadResponse(MemBuffer* responseBuffer);
		PM_STATUS CallPmService(MemBuffer* requestBuffer, MemBuffer* responseBuffer);
		HANDLE uniqueHandleMember = INVALID_HANDLE_VALUE;
		uint32_t clientProcessId = 0;
	};
}
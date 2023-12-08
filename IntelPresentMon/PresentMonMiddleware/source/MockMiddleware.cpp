#include "MockMiddleware.h"
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdlib>
#include "../../PresentMonAPI2/source/Internal.h"
#include "../../PresentMonAPIWrapperCommon/source/Introspection.h"
// TODO: don't need transfer if we can somehow get the PM_ struct generation working without inheritance
// needed right now because even if we forward declare, we don't have the inheritance info
#include "../../Interprocess/source/IntrospectionTransfer.h"
#include "../../Interprocess/source/IntrospectionHelpers.h"
#include "../../Interprocess/source/IntrospectionCloneAllocators.h"
#include "MockCommon.h"
#include "DynamicQuery.h"

namespace pmon::mid
{
	using namespace ipc::intro;
	using namespace std::string_literals;

	MockMiddleware::MockMiddleware(bool useLocalShmServer)
	{
		if (useLocalShmServer) {
			pServiceComms = ipc::MakeServiceComms(mockIntrospectionNsmName);
			ipc::intro::RegisterMockIntrospectionDevices(*pServiceComms);
		}
		pMiddlewareComms = ipc::MakeMiddlewareComms(mockIntrospectionNsmName);
	}

	void MockMiddleware::AdvanceTime(uint32_t milliseconds)
	{
		t += milliseconds;
	}

	void MockMiddleware::Speak(char* buffer) const
	{
		strcpy_s(buffer, 256, "mock-middle");
	}

	const PM_INTROSPECTION_ROOT* MockMiddleware::GetIntrospectionData()
	{
		return pMiddlewareComms->GetIntrospectionRoot();
	}

	void MockMiddleware::FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot)
	{
		free(const_cast<PM_INTROSPECTION_ROOT*>(pRoot));
	}

	PM_DYNAMIC_QUERY* MockMiddleware::RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, double windowSizeMs, double metricOffsetMs)
	{
		// get introspection data for reference
		// TODO: cache this data so it's not required to be generated every time
		pmapi::intro::Dataset ispec{ GetIntrospectionData(), [this](auto p){FreeIntrospectionData(p);} };

		// make the query object that will be managed by the handle
		auto pQuery = std::make_unique<PM_DYNAMIC_QUERY>();

		uint64_t offset = 0u;
		for (auto& qe : queryElements) {
			auto metricView = ispec.FindMetric(qe.metric);
			if (metricView.GetType().GetValue() != int(PM_METRIC_TYPE_DYNAMIC)) {
				// TODO: specific exception here
				throw std::runtime_error{ "Static metric in dynamic metric query specification" };
			}
			// TODO: validate device id
			// TODO: validate array index
			qe.dataOffset = offset;
			qe.dataSize = GetDataTypeSize(metricView.GetDataTypeInfo().GetBasePtr()->type);
			offset += qe.dataSize;
		}

		pQuery->elements = std::vector<PM_QUERY_ELEMENT>{ queryElements.begin(), queryElements.end() };

		return pQuery.release();
	}

	void MockMiddleware::FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery)
	{
		delete pQuery;
	}

	void MockMiddleware::PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains)
	{
		for (auto& qe : pQuery->elements) {
			if (qe.metric == PM_METRIC_PRESENT_MODE) {
				auto& output = reinterpret_cast<int&>(pBlob[qe.dataOffset]);
				if (t % 2 == 0) {
					output = (int)PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP;
				}
				else {
					output = (int)PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP;
				}
			}
			else {
				auto& output = reinterpret_cast<double&>(pBlob[qe.dataOffset]);
				if (t % 2 == 0) {
					output = (double)qe.metric;
				}
				else {
					output = 0.;
				}
			}
		}
	}

	void MockMiddleware::PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob)
	{
		// get introspection data for reference
		// TODO: cache this data so it's not required to be generated every time
		pmapi::intro::Dataset ispec{ GetIntrospectionData(), [this](auto p) {FreeIntrospectionData(p); } };

		auto metricView = ispec.FindMetric(element.metric);
		if (metricView.GetType().GetValue() != int(PM_METRIC_TYPE_STATIC)) {
			// TODO: more specific exception
			throw std::runtime_error{ "dynamic metric in static query poll" };
		}
		if (element.metric == PM_METRIC_PROCESS_NAME) {
			strcpy_s(reinterpret_cast<char*>(pBlob), 260, "dota2.exe");
		}
		else {
			throw std::runtime_error{ "unknown metric in static poll" };
		}
	}
}
#include "PresentMonDiagnostics.h"
#include "../CommonUtilities/log/DiagnosticDriver.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Exception.h"
#include "../PresentMonMiddleware/LogSetup.h"

using namespace pmon;
using namespace pmon::util;

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetup(const PM_DIAGNOSTIC_CONFIGURATION* pConfig)
{
	try {
		log::SetupDiagnosticLayer(pConfig);
		return PM_STATUS_SUCCESS;
	} pmcatch_report;
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetQueuedMessageCount()
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->GetQueuedMessageCount();
		}
	} pmcatch_report;
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetMaxQueuedMessages()
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->GetMaxQueuedMessages();
		}
	} pmcatch_report;
	return 0;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetMaxQueuedMessages(uint32_t max)
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			pDiag->SetMaxQueuedMessages(max);
			return PM_STATUS_SUCCESS;
		}
	} pmcatch_report;
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetDiscardedMessageCount()
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->GetDiscardedMessageCount();
		}
	} pmcatch_report;
	return 0;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticDequeueMessage(PM_DIAGNOSTIC_MESSAGE** ppMessage)
{
	try {
		if (ppMessage) {
			if (auto pDiag = log::GetDiagnostics()) {
				if (auto pMessage = pDiag->DequeueMessage()) {
					*ppMessage = pMessage.release();
					return PM_STATUS_SUCCESS;
				}
				else {
					return PM_STATUS_NO_DATA;
				}
			}
		}
	} pmcatch_report;
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticEnqueueMessage(const PM_DIAGNOSTIC_MESSAGE* pMessage)
{
	try {
		if (pMessage) {
			if (auto pDiag = log::GetDiagnostics()) {
				pDiag->EnqueueMessage(pMessage);
				return PM_STATUS_SUCCESS;
			}
		}
	} pmcatch_report;
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticFreeMessage(PM_DIAGNOSTIC_MESSAGE* pMessage)
{
	try {
		if (pMessage) {
			delete pMessage;
			return PM_STATUS_SUCCESS;
		}
	} pmcatch_report;
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_DIAGNOSTIC_WAKE_REASON pmDiagnosticWaitForMessage(uint32_t timeoutMs)
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->WaitForMessage(timeoutMs);
		}
	} pmcatch_report;
	return PM_DIAGNOSTIC_WAKE_REASON_ERROR;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticUnblockWaitingThread()
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			pDiag->UnblockWaitingThread();
			return PM_STATUS_SUCCESS;
		}
	} pmcatch_report;
	return PM_STATUS_FAILURE;
}

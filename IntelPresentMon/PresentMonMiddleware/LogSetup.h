#pragma once
#include <Memory>
#include "../CommonUtilities/log/IChannel.h"
#include "../PresentMonAPI2/PresentMonDiagnostics.h"

namespace pmon::util::log
{
	void InjectCopyChannel(std::shared_ptr<IChannel> pCopyTargetChannel) noexcept;
	void InjectStandaloneChannel() noexcept;
	void SetupDiagnosticLayer(const PM_DIAGNOSTIC_CONFIGURATION* pConfig);
	std::shared_ptr<class DiagnosticDriver> GetDiagnostics();
}
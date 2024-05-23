#pragma once
#include <Memory>
#include "../CommonUtilities/log/IChannel.h"
#include "PresentMonDiagnostics.h"

namespace pmon::util::log
{
	void InjectCopyChannel(std::shared_ptr<IChannel> pCopyTargetChannel) noexcept;
	void InjectStandaloneChannel() noexcept;
	void SetupDiagnosticLayer(PM_DIAGNOSTIC_OUTPUT_FLAGS flags);
	std::shared_ptr<class DiagnosticDriver> GetDiagnostics();
}
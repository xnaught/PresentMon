#include "ActionExecutionContext.h"
#include <CommonUtilities/rng/MemberSlice.h>
#include <CommonUtilities/rng/OptionalMinMax.h>
#include "../Interprocess/source/act/ActionHelper.h"

namespace pmon::svc::acts
{
    void ActionExecutionContext::Dispose(SessionContextType& stx)
    {
        for (auto& tracked : stx.trackedPids) {
            pPmon->StopStreaming(stx.remotePid, tracked);
        }
        stx.requestedTelemetryPeriodMs.reset();
        UpdateTelemetryPeriod();
    }
    void ActionExecutionContext::UpdateTelemetryPeriod() const
    {
        // gather requests across all sessions
        auto&& reqPeriods = util::rng::MemberSlice(*pSessionMap, &SessionContextType::requestedTelemetryPeriodMs);
        // determine the prioritized setting among those
        const auto prioritizedPeriod = util::rng::OptionalMin(reqPeriods);
        // execute the setting on the service system
        if (auto sta = pPmon->SetGpuTelemetryPeriod(prioritizedPeriod); sta != PM_STATUS_SUCCESS) {
            pmlog_error("Set telemetry period failed").code(sta);
            throw util::Except<ipc::act::ActionExecutionError>(sta);
        }
    }
}
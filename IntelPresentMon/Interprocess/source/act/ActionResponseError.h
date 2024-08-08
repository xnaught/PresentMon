#pragma once
#include "../PmStatusError.h"


namespace pmon::ipc::act
{
	class ActionResponseError : public PmStatusError {
	public: ActionResponseError(PM_STATUS code) : PmStatusError{ code, "Error occurred while processing Action" } {}
	};
}
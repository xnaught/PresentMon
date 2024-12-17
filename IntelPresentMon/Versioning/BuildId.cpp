#include "BuildId.h"
#include "generated/build_id.h"

namespace pmon::bid
{
	const char* BuildIdShortHash() noexcept
	{
		return PM_BID_GIT_HASH_SHORT;
	}
	const char* BuildIdLongHash() noexcept
	{
		return PM_BID_GIT_HASH;
	}
	const char* BuildIdTimestamp() noexcept
	{
		return PM_BID_TIME;
	}
	const char* BuildIdUid() noexcept
	{
		return PM_BID_UID;
	}
	bool BuildIdDirtyFlag() noexcept
	{
		return PM_BID_DIRTY;
	}
}
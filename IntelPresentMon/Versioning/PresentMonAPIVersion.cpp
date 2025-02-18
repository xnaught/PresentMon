#include "PresentMonAPIVersion.h"
#include "BuildId.h"
#include <cstring>

namespace pmon::bid
{
	PM_VERSION GetApiVersion() noexcept
	{
		PM_VERSION ver{
			.major = 3,
			.minor = 0,
			.patch = 0,
			.tag = "beta",
		};
		strncpy_s(ver.hash, BuildIdShortHash(), _TRUNCATE);
		return ver;
	}
}
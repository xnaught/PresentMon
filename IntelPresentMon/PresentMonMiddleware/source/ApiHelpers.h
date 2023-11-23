#pragma once
#include <cstdlib>
#include <memory>
#include "../../PresentMonAPI2/source/PresentMonAPI.h"

namespace pmon::mid
{
	struct ApiBlockDeleter
	{
		template<typename T>
		void operator()(T* p) const { free(p); }
	};
	using UniqueApiRootPtr = std::unique_ptr<PM_INTROSPECTION_ROOT, ApiBlockDeleter>;
}
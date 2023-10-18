#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"

namespace pmid
{
	class Middleware
	{
	public:
		virtual ~Middleware() = default;
		virtual void Speak(char* buffer) const = 0;
		virtual const PM_INTROSPECTION_ROOT* GetIntrospectionData() const = 0;
	};
}
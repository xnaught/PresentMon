#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include <span>

namespace pmon::mid
{
	class Middleware
	{
	public:
		virtual ~Middleware() = default;
		virtual void Speak(char* buffer) const = 0;
		virtual const PM_INTROSPECTION_ROOT* GetIntrospectionData() const = 0;
		virtual void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) const = 0;
		virtual PM_DYNAMIC_QUERY* RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements) const = 0;
		virtual void FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery) const = 0;
		virtual void PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint8_t* pBlob) const = 0;
		virtual void PollStaticQuery(const PM_QUERY_ELEMENT& element, uint8_t* pBlob) const = 0;
	};
}
#pragma once
#include "Middleware.h"

namespace pmid
{
	class ConcreteMiddleware : public Middleware
	{
	public:
		ConcreteMiddleware();
		void Speak(char* buffer) const override;
		const PM_INTROSPECTION_ROOT* GetIntrospectionData() const override { return nullptr; }
		void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) const override {}
		PM_DYNAMIC_QUERY* RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements) const override { return nullptr; }
		void FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery) const override {}
		void PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint8_t* pBlob) const override {}
		void PollStaticQuery(const PM_QUERY_ELEMENT& element, uint8_t* pBlob) const override {}
	};
}
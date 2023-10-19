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
	};
}
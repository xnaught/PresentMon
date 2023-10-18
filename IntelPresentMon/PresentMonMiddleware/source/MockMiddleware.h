#pragma once
#include "Middleware.h"

namespace pmid
{
	class MockMiddleware : public Middleware
	{
	public:
		MockMiddleware();
		void Speak(char* buffer) const override;
		const PM_INTROSPECTION_ROOT* GetIntrospectionData() const override;
	};
}
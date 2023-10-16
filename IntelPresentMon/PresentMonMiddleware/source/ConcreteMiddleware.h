#pragma once
#include "Middleware.h"

namespace pmid
{
	class ConcreteMiddleware : public Middleware
	{
	public:
		ConcreteMiddleware();
		void Speak(char* buffer) const override;
	};
}
#include "ConcreteMiddleware.h"
#include <cstring>

namespace pmon::mid
{
	ConcreteMiddleware::ConcreteMiddleware() = default;

	void ConcreteMiddleware::Speak(char* buffer) const
	{
		strcpy_s(buffer, 256, "concrete-middle");
	}
}
#include "PresentMonAPI.h"
#include <memory>
#include "../../PresentMonMiddleware/source/MockMiddleware.h"
#include "../../PresentMonMiddleware/source/ConcreteMiddleware.h"


using namespace pmid;

// global state
std::unique_ptr<Middleware> pMiddleware;


PRESENTMON_API_EXPORT void pmSetMiddlewareAsMock(bool mocked)
{
	if (mocked) {
		pMiddleware = std::make_unique<MockMiddleware>();
	}
	else {
		pMiddleware = std::make_unique<ConcreteMiddleware>();
	}
}

PRESENTMON_API_EXPORT void pmMiddlewareSpeak(char* buffer)
{
	pMiddleware->Speak(buffer);
}
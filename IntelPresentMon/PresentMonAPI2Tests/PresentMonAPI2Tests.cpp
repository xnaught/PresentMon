#include "CppUnitTest.h"
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include <cstring>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2
{
	TEST_CLASS(TrialTests)
	{
	public:		
		TEST_METHOD(InterfaceVirtualization)
		{
			char buffer[256]{};

			pmSetMiddlewareAsMock(true);
			pmMiddlewareSpeak(buffer);
			Assert::AreEqual(0, strcmp("mock-middle", buffer));

			pmSetMiddlewareAsMock(false);
			pmMiddlewareSpeak(buffer);
			Assert::AreEqual(0, strcmp("concrete-middle", buffer));
		}
	};
}

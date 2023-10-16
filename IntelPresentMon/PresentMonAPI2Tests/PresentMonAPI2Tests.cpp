#include "CppUnitTest.h"
#include "../PresentMonAPI2/source/PresentMonAPI.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2Tests
{
	TEST_CLASS(PresentMonAPI2Tests)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			Assert::AreEqual(420, pmTest(69));
			Assert::AreEqual(-10, pmTest(10));
		}
	};
}

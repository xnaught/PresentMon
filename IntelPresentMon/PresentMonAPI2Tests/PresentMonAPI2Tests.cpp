#include "CppUnitTest.h"
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include <cstring>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

PRESENTMON_API_EXPORT void pmSetMiddlewareAsMock_(bool mocked);
PRESENTMON_API_EXPORT PM_STATUS pmMiddlewareSpeak_(char* buffer);

namespace PresentMonAPI2
{
	TEST_CLASS(CAPITests)
	{
	public:		
		TEST_METHOD(OpenMockSession)
		{
			char buffer[256]{};

			pmSetMiddlewareAsMock_(true);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmMiddlewareSpeak_(buffer));
			Assert::AreEqual("mock-middle", buffer);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
			Assert::AreEqual((int)PM_STATUS_SESSION_NOT_OPEN, (int)pmMiddlewareSpeak_(buffer));
		}
		TEST_METHOD(OpenConcreteSession)
		{
			char buffer[256]{};

			pmSetMiddlewareAsMock_(false);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmMiddlewareSpeak_(buffer));
			Assert::AreEqual("concrete-middle", buffer);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
		}
		TEST_METHOD(FailUsingClosedSession)
		{
			char buffer[256]{};

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
			Assert::AreEqual((int)PM_STATUS_SESSION_NOT_OPEN, (int)pmMiddlewareSpeak_(buffer));
		}
		TEST_METHOD(Introspect)
		{
			// initialization
			pmSetMiddlewareAsMock_(true);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());

			// introspection query
			const PM_INTROSPECTION_ROOT* pRoot{};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmEnumerateInterface(&pRoot));
			Assert::IsNotNull(pRoot);
			Assert::AreEqual(2ull, pRoot->pEnums->size);

			// checking 1st enum
			{
				auto pEnum = static_cast<const PM_INTROSPECTION_ENUM*>(pRoot->pEnums->pData[0]);
				Assert::IsNotNull(pEnum);
				Assert::IsNotNull(pEnum->pSymbol);
				Assert::AreEqual("PM_UNIT", pEnum->pSymbol->pData);
				Assert::AreEqual((int)PM_ENUM_UNIT, (int)pEnum->id);
				Assert::AreEqual(2ull, pEnum->pKeys->size);
				// 1st key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[0]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_UNIT_FPS", pKey->pSymbol->pData);
					Assert::AreEqual("FPS", pKey->pName->pData);
					Assert::AreEqual((int)PM_ENUM_UNIT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_UNIT::PM_UNIT_FPS, pKey->value);
				}
				// 2nd key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[1]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_UNIT_WATTS", pKey->pSymbol->pData);
					Assert::AreEqual("Watts", pKey->pName->pData);
					Assert::AreEqual((int)PM_ENUM_UNIT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_UNIT::PM_UNIT_WATTS, pKey->value);
				}
			}

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
		}
	};
}

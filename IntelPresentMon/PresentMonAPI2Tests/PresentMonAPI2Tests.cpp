#include "CppUnitTest.h"
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include <cstring>
#include <crtdbg.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

PRESENTMON_API_EXPORT void pmSetMiddlewareAsMock_(bool mocked, bool useCrtHeapDebug = false);
PRESENTMON_API_EXPORT _CrtMemState pmCreateHeapCheckpoint_();
PRESENTMON_API_EXPORT PM_STATUS pmMiddlewareSpeak_(char* buffer);

namespace PresentMonAPI2
{
	bool CrtDiffHasMemoryLeaks(const _CrtMemState& before, const _CrtMemState& after)
	{
		_CrtMemState difference;
		if (_CrtMemDifference(&difference, &before, &after)) {
			if (difference.lCounts[_NORMAL_BLOCK] > 0) {
				return true;
			}
		}
		return false;
	}

	TEST_CLASS(CAPISessionTests)
	{
	public:
		TEST_METHOD_CLEANUP(AfterEachTestMethod)
		{
			pmCloseSession();
		}
		TEST_METHOD(OpenAndCloseMockSession)
		{
			char buffer[256]{};

			pmSetMiddlewareAsMock_(true);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmMiddlewareSpeak_(buffer));
			Assert::AreEqual("mock-middle", buffer);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
			Assert::AreEqual((int)PM_STATUS_SESSION_NOT_OPEN, (int)pmMiddlewareSpeak_(buffer));
		}
		TEST_METHOD(OpenAndCloseConcreteSession)
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

			pmSetMiddlewareAsMock_(true);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
			Assert::AreEqual((int)PM_STATUS_SESSION_NOT_OPEN, (int)pmMiddlewareSpeak_(buffer));
		}
		TEST_METHOD(OpenAndCloseWithoutLeak)
		{
			pmSetMiddlewareAsMock_(true, true);
			const auto heapBefore = pmCreateHeapCheckpoint_();

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsFalse(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
		TEST_METHOD(OpenWithoutCloseCausesLeak)
		{
			pmSetMiddlewareAsMock_(true, true);
			const auto heapBefore = pmCreateHeapCheckpoint_();

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsTrue(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
		TEST_METHOD(LeaksArentDetectedWithoutMockSetting)
		{
			pmSetMiddlewareAsMock_(true, false);
			const auto heapBefore = pmCreateHeapCheckpoint_();

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession());

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsFalse(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
	};

	TEST_CLASS(CAPIMainTests)
	{
	public:
		TEST_METHOD_INITIALIZE(BeforeEachTestMethod)
		{
			pmSetMiddlewareAsMock_(true, true);
			pmOpenSession();
		}
		TEST_METHOD_CLEANUP(AfterEachTestMethod)
		{
			pmCloseSession();
		}
		TEST_METHOD(FreeIntrospectionTree)
		{
			const auto heapBefore = pmCreateHeapCheckpoint_();

			const PM_INTROSPECTION_ROOT* pRoot{};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmEnumerateInterface(&pRoot));
			Assert::IsNotNull(pRoot);

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeInterface(pRoot));

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsFalse(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
		TEST_METHOD(LeakIntrospectionTree)
		{
			const auto heapBefore = pmCreateHeapCheckpoint_();

			const PM_INTROSPECTION_ROOT* pRoot{};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmEnumerateInterface(&pRoot));
			Assert::IsNotNull(pRoot);

			// normally we would free the linked structure here via its root
			// Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeInterface(pRoot));

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsTrue(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
		TEST_METHOD(Introspect)
		{
			// introspection query
			const PM_INTROSPECTION_ROOT* pRoot{};
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmEnumerateInterface(&pRoot));
			Assert::IsNotNull(pRoot);
			Assert::AreEqual(4ull, pRoot->pEnums->size);

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

			// checking 2nd enum
			{
				auto pEnum = static_cast<const PM_INTROSPECTION_ENUM*>(pRoot->pEnums->pData[1]);
				Assert::IsNotNull(pEnum);
				Assert::IsNotNull(pEnum->pSymbol);
				Assert::AreEqual("PM_STAT", pEnum->pSymbol->pData);
				Assert::AreEqual((int)PM_ENUM_STAT, (int)pEnum->id);
				Assert::AreEqual(2ull, pEnum->pKeys->size);
				// 1st key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[0]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_STAT_AVG", pKey->pSymbol->pData);
					Assert::AreEqual("Average", pKey->pName->pData);
					Assert::AreEqual((int)PM_ENUM_STAT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_STAT::PM_STAT_AVG, pKey->value);
				}
				// 2nd key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[1]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_STAT_MIN", pKey->pSymbol->pData);
					Assert::AreEqual("Minimum", pKey->pName->pData);
					Assert::AreEqual((int)PM_ENUM_STAT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_STAT::PM_STAT_MIN, pKey->value);
				}
			}

			// checking 3rd enum
			{
				auto pEnum = static_cast<const PM_INTROSPECTION_ENUM*>(pRoot->pEnums->pData[2]);
				Assert::IsNotNull(pEnum);
				Assert::AreEqual((int)PM_ENUM_UNIT, (int)pEnum->id);
				Assert::AreEqual("PM_UNIT", pEnum->pSymbol->pData);
				Assert::AreEqual("List of all units of measure used for metrics", pEnum->pDescription->pData);
				Assert::AreEqual(5ull, pEnum->pKeys->size);
				// 1st key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[0]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_UNIT_DIMENSIONLESS", pKey->pSymbol->pData);
					Assert::AreEqual("", pKey->pName->pData);
					Assert::AreEqual((int)PM_ENUM_UNIT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_UNIT_DIMENSIONLESS, pKey->value);
				}
				// 5th key
				{
					auto pKey = static_cast<const PM_INTROSPECTION_ENUM_KEY*>(pEnum->pKeys->pData[4]);
					Assert::IsNotNull(pKey);
					Assert::IsNotNull(pKey->pSymbol);
					Assert::AreEqual("PM_UNIT_PERCENT", pKey->pSymbol->pData);
					Assert::AreEqual("%", pKey->pName->pData);
					Assert::AreEqual((int)PM_ENUM_UNIT, (int)pKey->enumId);
					Assert::AreEqual((int)PM_UNIT_PERCENT, pKey->value);
				}
			}

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFreeInterface(pRoot));
		}
	};
}

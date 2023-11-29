#include "CppUnitTest.h"
#include "../PresentMonAPI2/source/PresentMonAPI.h"
#include "../PresentMonAPI2/source/Internal.h"
#include <cstring>
#include <vector>
#include <optional>
#include "Utilities.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2
{
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
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession(4004));
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmMiddlewareSpeak_(buffer));
			Assert::AreEqual("mock-middle", buffer);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
			Assert::AreEqual((int)PM_STATUS_SESSION_NOT_OPEN, (int)pmMiddlewareSpeak_(buffer));
		}
		TEST_METHOD(OpenAndCloseConcreteSession)
		{
			char buffer[256]{};

			pmSetMiddlewareAsMock_(false);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession(4004));
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmMiddlewareSpeak_(buffer));
			Assert::AreEqual("concrete-middle", buffer);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
		}
		TEST_METHOD(FailUsingClosedSession)
		{
			char buffer[256]{};

			pmSetMiddlewareAsMock_(true);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession(4004));
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());
			Assert::AreEqual((int)PM_STATUS_SESSION_NOT_OPEN, (int)pmMiddlewareSpeak_(buffer));
		}
		TEST_METHOD(OpenAndCloseWithoutLeak)
		{
			pmSetMiddlewareAsMock_(true, true);
			const auto heapBefore = pmCreateHeapCheckpoint_();

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession(4004));
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmCloseSession());

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsFalse(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
		TEST_METHOD(OpenWithoutCloseCausesLeak)
		{
			pmSetMiddlewareAsMock_(true, true);
			const auto heapBefore = pmCreateHeapCheckpoint_();

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession(4004));

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsTrue(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
		TEST_METHOD(LeaksArentDetectedWithoutMockSetting)
		{
			pmSetMiddlewareAsMock_(true, false);
			const auto heapBefore = pmCreateHeapCheckpoint_();

			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmOpenSession(4004));

			const auto heapAfter = pmCreateHeapCheckpoint_();
			Assert::IsFalse(CrtDiffHasMemoryLeaks(heapBefore, heapAfter));
		}
	};
}

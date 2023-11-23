#include "CppUnitTest.h"
#include <cstring>
#include <vector>
#include <optional>
#include "Utilities.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/IntrospectionTransfer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentMonAPI2
{
	using namespace pmon;
	TEST_CLASS(InterprocessTests)
	{
	public:
		TEST_METHOD(ApiProbeCloneSize)
		{
			auto pComm = ipc::MakeMiddlewareView();
			auto& root = pComm->GetIntrospectionRoot();
			ipc::intro::ProbeAllocator<void> alloc;
			auto pClone = root.ApiClone(alloc);
			Assert::AreEqual(16589ull, alloc.GetTotalSize());
			Assert::IsNull(pClone.get());
		}
		TEST_METHOD(Padding)
		{
			Assert::AreEqual(4ull, ipc::intro::GetPadding<void*>(4ull));
			Assert::AreEqual(3ull, ipc::intro::GetPadding<PM_ENUM>(41ull));
			Assert::AreEqual(1ull, ipc::intro::GetPadding<uint32_t>(3ull));
		}
	};
}

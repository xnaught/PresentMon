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
		TEST_METHOD(IntrospectionApiClone)
		{
			auto pComm = ipc::MakeMiddlewareView();
			auto& root = pComm->GetIntrospectionRoot();
			ipc::intro::ProbeAllocator<void> alloc;
			auto pClone = root.ApiClone(alloc);
			Assert::AreEqual(69ull, alloc.GetTotalSize());
			Assert::IsNull(pClone.get());
		}
	};
}

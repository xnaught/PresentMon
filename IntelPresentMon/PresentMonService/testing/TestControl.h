#pragma once
#include <thread>

class PresentMon;

namespace pmon::svc::testing
{
	class TestControlModule
	{
	public:
		TestControlModule(const PresentMon* pPresentMon);
	private:
		void Run_();
		const PresentMon* pPresentMon_;
		std::jthread worker_;
	};
}
#pragma once
#include <thread>
#include "../Exception.h"
#include "../log/IdentificationTable.h"
#include "../log/Log.h"

namespace pmon::util::mt
{
	class Thread : public std::jthread
	{
	public:
		Thread() = default;
		template<typename...R, std::invocable<R...> F>
		Thread(F&& func, R&&...args)
			:
			std::jthread{ [](F&& func, R&&...args) {
				InstallSehTranslator();
				std::invoke(std::forward<F>(func), std::forward<R>(args)...);
			}, std::forward<F>(func), std::forward<R>(args)... }
		{}
		template<typename...R, std::invocable<R...> F>
		Thread(std::wstring threadName, F&& func, R&&...args)
			:
			std::jthread{ [](std::wstring threadName, F&& func, R&&...args) {
				InstallSehTranslator();
				log::IdentificationTable::AddThisThread(std::move(threadName));
				std::invoke(std::forward<F>(func), std::forward<R>(args)...);
			}, std::move(threadName), std::forward<F>(func), std::forward<R>(args)... }
		{}
		template<typename...R, std::invocable<R...> F>
		Thread(const std::wstring& threadName, int threadNumber, F&& func, R&&...args)
			:
			Thread{ MakeThreadName_(threadName, threadNumber), std::forward<F>(func), std::forward<R>(args)... }
		{}
	private:
		static std::wstring MakeThreadName_(const std::wstring& name, int number);
	};
}
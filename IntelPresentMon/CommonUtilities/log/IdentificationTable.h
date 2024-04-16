#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>
#include <shared_mutex>
#include <optional>

namespace pmon::util::log
{
	class IdentificationTable
	{
	public:
		// types
		struct Thread
		{
			uint32_t tid;
			uint32_t pid;
			std::wstring name;
		};
		struct Process
		{
			uint32_t pid;
			std::wstring name;
		};
		// functions
		static void AddThread(uint32_t tid, uint32_t pid, std::wstring name) noexcept;
		static void AddProcess(uint32_t pid, std::wstring name) noexcept;
		static void AddThisThread(std::wstring name) noexcept;
		static void AddThisProcess(std::wstring name) noexcept;
		static std::optional<Thread> LookupThread(uint32_t tid) noexcept;
		static std::optional<Process> LookupProcess(uint32_t pid) noexcept;
	private:
		// functions
		IdentificationTable();
		static IdentificationTable& Get_();
		void AddThread_(uint32_t tid, uint32_t pid, std::wstring name);
		void AddProcess_(uint32_t pid, std::wstring name);
		std::optional<Thread> LookupThread_(uint32_t tid) const;
		std::optional<Process> LookupProcess_(uint32_t pid) const;
		// data
		mutable std::shared_mutex mtx_;
		std::unordered_map<uint32_t, Process> processes_;
		std::unordered_map<uint32_t, Thread> threads_;
	};
}

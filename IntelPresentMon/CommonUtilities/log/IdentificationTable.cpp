#include "IdentificationTable.h"
#include "../win/WinAPI.h"

namespace pmon::util::log
{
	void IdentificationTable::AddThread(uint32_t tid, uint32_t pid, std::wstring name) noexcept
	{
		try {
			Get_().AddThread_(tid, pid, std::move(name));
		}
		catch (...) {}
	}
	void IdentificationTable::AddProcess(uint32_t pid, std::wstring name) noexcept
	{
		try {
			Get_().AddProcess_(pid, std::move(name));
		}
		catch (...) {}
	}
	void IdentificationTable::AddThisThread(std::wstring name) noexcept
	{
		AddThread(GetCurrentThreadId(), GetCurrentProcessId(), std::move(name));
	}
	void IdentificationTable::AddThisProcess(std::wstring name) noexcept
	{
		AddProcess(GetCurrentProcessId(), std::move(name));
	}
	std::optional<IdentificationTable::Thread> IdentificationTable::LookupThread(uint32_t tid) noexcept
	{
		try {
			return Get_().LookupThread_(tid);
		}
		catch (...) {
			return {};
		}
	}
	std::optional<IdentificationTable::Process> IdentificationTable::LookupProcess(uint32_t pid) noexcept
	{
		try {
			return Get_().LookupProcess_(pid);
		}
		catch (...) {
			return {};
		}
	}

	IdentificationTable::IdentificationTable() = default;
	IdentificationTable& IdentificationTable::Get_()
	{
		// @SINGLETON
		static IdentificationTable table;
		return table;
	}
	void IdentificationTable::AddThread_(uint32_t tid, uint32_t pid, std::wstring name)
	{
		std::lock_guard lk{ mtx_ };
		threads_.emplace(std::piecewise_construct,
			std::forward_as_tuple(tid),
			std::forward_as_tuple(tid, pid, std::move(name))
		);
	}
	void IdentificationTable::AddProcess_(uint32_t pid, std::wstring name)
	{
		std::lock_guard lk{ mtx_ };
		processes_.emplace(std::piecewise_construct,
			std::forward_as_tuple(pid),
			std::forward_as_tuple(pid, std::move(name))
		);
	}
	std::optional<IdentificationTable::Thread> IdentificationTable::LookupThread_(uint32_t tid) const
	{
		try {
			std::shared_lock lk{ mtx_ };
			return threads_.at(tid);
		}
		catch (...) {
			return {};
		}
	}
	std::optional<IdentificationTable::Process> IdentificationTable::LookupProcess_(uint32_t pid) const
	{
		try {
			std::shared_lock lk{ mtx_ };
			return processes_.at(pid);
		}
		catch (...) {
			return {};
		}
	}
}
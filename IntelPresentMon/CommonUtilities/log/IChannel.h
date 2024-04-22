#pragma once
#include <memory>

namespace pmon::util::log
{
	struct Entry;
	class IDriver;
	class IPolicy;

	class IEntrySink
	{
	public:
		virtual ~IEntrySink() = default;
		virtual void Submit(Entry&&) noexcept = 0;
		virtual void Flush() = 0;
	};

	class IChannel : public IEntrySink
	{
	public:
		virtual void AttachDriver(std::shared_ptr<IDriver>) = 0;
		virtual void AttachPolicy(std::shared_ptr<IPolicy>) = 0;
		virtual void FlushEntryPointExit() = 0;
	};
}
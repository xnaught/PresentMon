#pragma once
#include "EntryBuilder.h"
#include "IChannel.h"
#include "GlobalPolicy.h"
#include <functional>

namespace pmon::util::log
{
	// this function must be implemented by the module using this log utility
	std::shared_ptr<IChannel> GetDefaultChannel() noexcept;
	// eager initialize logging on separate thread
	void BootDefaultChannelEager() noexcept;
	// set the default channel directly
	void InjectDefaultChannel(std::shared_ptr<IChannel>) noexcept;
	// utility that client module can use to help implement GetDefaultChannel
	std::shared_ptr<IChannel> GetDefaultChannelWithFactory(std::function<std::shared_ptr<IChannel>()> factory) noexcept;
	// DefaultChannelManager should be instantiated as early as possible in the entry point of the process/module
	// It acts as a guard to prevent stack trace resolution in the channel worker thread after exiting main
	struct DefaultChannelManager
	{
		DefaultChannelManager();
		~DefaultChannelManager();
	};
}

#ifndef PMLOG_BUILD_LEVEL
#ifndef NDEBUG
#define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Debug
#else
#define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Info
#endif
#endif

#define xinternal_pmlog_(lvl) ((PMLOG_BUILD_LEVEL < lvl) || (::pmon::util::log::GlobalPolicy::GetLogLevel() < lvl)) \
	? (void)0 : (void)::pmon::util::log::EntryBuilder{ lvl, __FILEW__, __FUNCTIONW__, __LINE__ } \
	.to(::pmon::util::log::GetDefaultChannel())
#define pmlog_fatal	xinternal_pmlog_(::pmon::util::log::Level::Fatal).note
#define pmlog_error	xinternal_pmlog_(::pmon::util::log::Level::Error).note
#define pmlog_warn	xinternal_pmlog_(::pmon::util::log::Level::Warn).note
#define pmlog_info	xinternal_pmlog_(::pmon::util::log::Level::Info).note
#define pmlog_dbg	xinternal_pmlog_(::pmon::util::log::Level::Debug).note
#define pmlog_verb(vtag) !vtag ? (void)0 : xinternal_pmlog_(::pmon::util::log::Level::Verbose).note

#define pmwatch(expr) watch(L###expr, (expr))

#define pmlog_setup ::pmon::util::log::DefaultChannelManager xpmlog_scope_sentinel_

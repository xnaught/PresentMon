#pragma once
#include "EntryBuilder.h"
#include "IChannel.h"
#include "GlobalPolicy.h"

namespace pmon::util::log
{
	void BootDefaultChannelEager() noexcept;
	IChannel* GetDefaultChannel() noexcept;
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

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
	// flush channel worker queue and exit thread in preparation for entry point return
	void FlushEntryPoint() noexcept;
}

#ifndef PMLOG_BUILD_LEVEL
#ifndef NDEBUG
#define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Debug
#else
#define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Info
#endif
#endif

#define xinternal_pmlog_(lvl) ((PMLOG_BUILD_LEVEL < lvl) || (::pmon::util::log::GlobalPolicy::Get().GetLogLevel() < lvl)) \
	? (void)0 : (void)::pmon::util::log::EntryBuilder{ lvl, __FILEW__, __FUNCTIONW__, __LINE__ } \
	.to(::pmon::util::log::GetDefaultChannel())
#define pmlog_fatal	xinternal_pmlog_(::pmon::util::log::Level::Fatal).note
#define pmlog_error	xinternal_pmlog_(::pmon::util::log::Level::Error).note
#define pmlog_warn	xinternal_pmlog_(::pmon::util::log::Level::Warning).note
#define pmlog_info	xinternal_pmlog_(::pmon::util::log::Level::Info).note
#define pmlog_dbg	xinternal_pmlog_(::pmon::util::log::Level::Debug).note
#define pmlog_verb(vtag) !vtag ? (void)0 : xinternal_pmlog_(::pmon::util::log::Level::Verbose).note
#define pmlog_perf(ptag) !ptag ? (void)0 : xinternal_pmlog_(::pmon::util::log::Level::Performance).note

#define pmwatch(expr) watch(L###expr, (expr))

#define pmlog_mark ::pmon::util::log::TimePoint

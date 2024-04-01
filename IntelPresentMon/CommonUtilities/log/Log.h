#pragma once
#include "EntryBuilder.h"
#include "IChannel.h"
#include "GlobalPolicy.h"

namespace pmon::util::log
{
	void BootDefaultChannelEager() noexcept;
	IChannel* GetDefaultChannel() noexcept;
}

#ifndef PMLOG_BUILD_LEVEL
#ifndef NDEBUG
#define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Debug
#else
#define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Error
#endif
#endif

#define xinternal_pmlog_(lvl) ((PMLOG_BUILD_LEVEL < lvl) || (::pmon::util::log::globalPolicy.GetLogLevel() < lvl)) \
	? (void)0 : (void)::pmon::util::log::EntryBuilder{ lvl, __FILEW__, __FUNCTIONW__, __LINE__ } \
	.to(::pmon::util::log::GetDefaultChannel())
#define pmlog_fatal	xinternal_pmlog_(::pmon::util::log::Level::Fatal)
#define pmlog_err	xinternal_pmlog_(::pmon::util::log::Level::Error)
#define pmlog_warn	xinternal_pmlog_(::pmon::util::log::Level::Warn)
#define pmlog_info	xinternal_pmlog_(::pmon::util::log::Level::Info)
#define pmlog_dbg	xinternal_pmlog_(::pmon::util::log::Level::Debug)
#define pmlog_verb(vtag) !vtag ? (void)0 : xinternal_pmlog_(::pmon::util::log::Level::Verbose)

#pragma once
#include "EntryBuilder.h"
#include "IChannel.h"
#include "GlobalPolicy.h"

namespace pmon::util::log
{
	IChannel* GetDefaultChannel() noexcept;
}

#ifndef NDEBUG
#define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Info
#else
#define PMLOG_BUILD_LEVEL ::pmon::util::log::Level::Error
#endif

#define internal_pmlog_(lvl) ((PMLOG_BUILD_LEVEL < lvl) || (::pmon::util::log::globalPolicy.GetLogLevel() < lvl)) \
	? (void)0 : (void)::pmon::util::log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ } \
	.to(::pmon::util::log::GetDefaultChannel()).level(lvl)
#define pmlog_err	internal_pmlog_(::pmon::util::log::Level::Error)
#define pmlog_warn	internal_pmlog_(::pmon::util::log::Level::Warn)
#define pmlog_info	internal_pmlog_(::pmon::util::log::Level::Info)

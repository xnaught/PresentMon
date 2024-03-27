#pragma once
#include "EntryBuilder.h"
#include "IChannel.h"

namespace pmon::util::log
{
	IChannel* GetDefaultChannel() noexcept;
}

#define pmlog ::pmon::util::log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ } \
	.to(::pmon::util::log::GetDefaultChannel())
#define pmlog_warn		pmlog.level(::pmon::util::log::Level::Warn)
#define pmlog_info		pmlog.level(::pmon::util::log::Level::Info)
#define pmlog_error		pmlog.level(::pmon::util::log::Level::Error)
#pragma once
#include "EntryBuilder.h"
#include "IChannel.h"

namespace pmon::util::log
{
	class IChannel* GetDefaultChannel() noexcept;
	// To simplify implementation of the free-threaded channel object, ability
	// to swap default channel at runtime has been removed. Can be reconsidered
	// if a compelling use-case is found
	// void InjectDefaultChannel(std::shared_ptr<IChannel>);
}

#define pmlog ::pmon::util::log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ } \
	.to(::pmon::util::log::GetDefaultChannel())
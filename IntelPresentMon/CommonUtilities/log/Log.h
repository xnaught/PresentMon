#pragma once
#include "EntryBuilder.h"
#include "IChannel.h"

namespace pmon::util::log
{
	class IChannel* GetDefaultChannel();
	void InjectChannel(std::shared_ptr<IChannel>);
}

#define pmlog ::pmon::util::log::EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ } \
	.to(::pmon::util::log::GetDefaultChannel())
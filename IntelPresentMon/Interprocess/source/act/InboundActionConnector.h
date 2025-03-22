// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "../../../CommonUtilities/str/String.h"
#include "Transfer.h"
#include "AsyncActionCollection.h"


namespace pmon::ipc::act
{
	class InboundActionConnector
	{
	public:
	private:
		// data
		std::unique_ptr<pipe::DuplexPipe> pPipe_;
	};
}
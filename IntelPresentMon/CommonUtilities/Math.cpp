// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Math.h"

namespace pmon::util
{
	void CalculateEma(double* ema, double newValue, double alpha)
	{

		if (newValue == 0.0) {
			*ema = 0.f;
		}
		else if (*ema == 0.f) {
			*ema = newValue;
		}
		else {
			*ema = *ema + alpha * float(newValue - *ema);
		}
	}
}
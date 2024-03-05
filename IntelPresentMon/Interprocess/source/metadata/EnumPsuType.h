#pragma once
#include "../../../PresentMonAPI2/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_PSU_TYPE(X_) \
		X_(PSU_TYPE, NONE, "None", "", "No power supply information") \
		X_(PSU_TYPE, PCIE, "PCIE", "", "Power supplied from PCIE bus") \
		X_(PSU_TYPE, 6PIN, "6PIN", "", "Power supplied from 6-pin power connector") \
		X_(PSU_TYPE, 8PIN, "8PIN", "", "Power supplied from 8-pin power connector")
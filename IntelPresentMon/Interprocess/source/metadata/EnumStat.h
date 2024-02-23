#pragma once
#include "../../../PresentMonAPI2/source/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_STAT(X_) \
		X_(STAT, NONE, "None", "", "Null stat, typically used when querying static or consuming frame events") \
		X_(STAT, AVG, "Average", "avg", "Average or mean of frame samples over the sliding window") \
		X_(STAT, PERCENTILE_99, "99th Percentile", "99%", "Value below which 99% of the observations within the sliding window fall (worst 1% value)") \
		X_(STAT, PERCENTILE_95, "95th Percentile", "95%", "Value below which 95% of the observations within the sliding window fall (worst 5% value)") \
		X_(STAT, PERCENTILE_90, "90th Percentile", "90%", "Value below which 90% of the observations within the sliding window fall (worst 10% value)") \
		X_(STAT, MAX, "Maximum", "max", "Maximum value of frame samples within the sliding window") \
		X_(STAT, MIN, "Minimum", "min", "Minimum value of frame samples within the sliding window") \
		X_(STAT, MID_POINT, "Midpoint", "raw", "Point sample of the frame data nearest to the middle of the sliding window") \
		X_(STAT, MID_LERP, "Mid Lerp", "mlp", "Linear interpolation between the two points nearest to the middle of the sliding window") \
		X_(STAT, NEWEST_POINT, "Newest Point", "npt", "Value in the most recent frame in the sliding window") \
		X_(STAT, OLDEST_POINT, "Oldest Point", "opt", "Value in the least recent frame in the sliding window") \
		X_(STAT, COUNT, "Count", "cnt", "Count of frames in the sliding window matching a predicate (e.g. counting # of frames for which a field is boolean true)")

#pragma once
//#define PMLOG_BUILD_LEVEL Verbose
//#define VVV_V8ASYNC
#include <CommonUtilities/log/Log.h>


namespace p2c::v
{
#ifndef VVV_WINDOW // system that tracks overlay target process ancestry and windows spawning therein
		inline constexpr bool window = false;
#else
		inline constexpr bool window = true;
#endif
#ifndef VVV_PROCWATCH // system that tracks overlay target process ancestry and windows spawning therein
		inline constexpr bool procwatch = false;
#else
		inline constexpr bool procwatch = true;
#endif		
#ifndef VVV_HOTKEY // system that processes raw keyboard input
		inline constexpr bool hotkey = false;
#else
		inline constexpr bool hotkey = true;
#endif	
#ifndef VVV_OVERLAY // system that controls overlay
		inline constexpr bool overlay = false;
#else
		inline constexpr bool overlay = true;
#endif
#ifndef VVV_METRIC // system for parsing QualifiedMetric lists and building queries, fetchers, and data packs
		inline constexpr bool metric = false;
#else
		inline constexpr bool metric = true;
#endif
#ifndef VVV_V8ASYNC // system for async calls and signals between V8 and C++
		inline constexpr bool v8async = false;
#else
		inline constexpr bool v8async = true;
#endif
}
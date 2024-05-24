#pragma once
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
}
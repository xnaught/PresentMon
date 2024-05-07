#pragma once
#include <CommonUtilities/log/Log.h>

namespace p2c
{
	namespace v // verbose logging module controls
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
	}

	// LogChannelManager should be instantiated as early as possible in the entry point of the process/module
	// It acts as a guard to prevent stack trace resolution in the channel worker thread after exiting main
	struct LogChannelManager
	{
		LogChannelManager() noexcept;
		~LogChannelManager();
	};
	// call after command line arguments have been parsed
	void ConfigureLogging() noexcept;
}
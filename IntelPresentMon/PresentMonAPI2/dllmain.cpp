#include <Windows.h>
#include "../CommonUtilities/log/Log.h"
#include "Log.h"

using namespace pmon::util;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
#ifdef NDEBUG
        // start with logging disabled initially in release builds
        // opt in with internal C++ calls or via C-Api diagnostic layer
        log::GlobalPolicy::Get().SetLogLevel(log::Level::None);
#endif
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


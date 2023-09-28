#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// This DLL is just used to store the Intel-PresentMon ETW Provider resources
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID)
{
    return TRUE;
}


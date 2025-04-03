#include "../CommonUtilities/win/WinAPI.h"
#include <boost/process.hpp>


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	boost::process::child childCef{
		"PresentMon.exe",
		"--p2c-svc-as-child",
		"--p2c-files-working",
		"--p2c-log-level", "debug",
		"--p2c-svc-pipe-enable",
		"--p2c-middleware-dll-path", "PresentMonAPI2.dll",
		"--p2c-log-middleware-copy",
		"--p2c-enable-ui-dev-options"
	};
	childCef.wait();

	return 0;
}
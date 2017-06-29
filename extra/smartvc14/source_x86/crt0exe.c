#include <windows.h>

extern void __cdecl SmartInit();
extern void __cdecl SmartTerm();

static int SmartShowWindow()
{
	STARTUPINFOA si;
	memset(&si, 0, sizeof(si));
	GetStartupInfoA(&si);
	return (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT;
}

void SmartStartup(void)
{
	int ret;
	SmartInit();
	ret = WinMain(GetModuleHandleA(NULL), NULL, NULL, SmartShowWindow());
	SmartTerm();
	ExitProcess(ret);
}

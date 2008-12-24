#include <windows.h>

extern "C"
void SmartStartup(void){
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	CoInitialize(NULL);
	GetStartupInfo(&si);
	int ret = WinMain(GetModuleHandle(NULL), NULL, NULL, si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
	CoUninitialize();
	ExitProcess(ret);
}

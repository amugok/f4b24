#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/gplugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/src/f4b24lx.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(linker, "/EXPORT:GetGPluginInfo=_GetGPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg);
static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode);

static GENERAL_PLUGIN_INFO gpinfo = {
	GPDK_VER,
	HookWndProc,
	OnEvent
};

#ifdef __cplusplus
extern "C"
#endif
GENERAL_PLUGIN_INFO * CALLBACK GetGPluginInfo(void){
	return &gpinfo;
}

static HMODULE m_hDLL = 0;

#include "../../../fittle/src/wastr.cpp"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg) {
	return FALSE;
}

static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode) {
	if (eCode == GENERAL_PLUGIN_EVENT_INIT0) {
		WAIsUnicode();
	} else if (eCode == GENERAL_PLUGIN_EVENT_QUIT) {
	}
	return TRUE;
}

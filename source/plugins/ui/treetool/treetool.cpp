/*
 * treetool
 *
 */

#include <windows.h>
#include <shlwapi.h>
#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/gplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")
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

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static void UpdateMenuItems(HMENU hMenu){
	UINT uState = GetMenuState(hMenu, IDM_TREE_EXPLORE, MF_BYCOMMAND);
	if (uState == (UINT)-1) return;
	uState = GetMenuState(hMenu, IDM_TREE_TOOL, MF_BYCOMMAND);
	if (uState == (UINT)-1){
		int c = GetMenuItemCount(hMenu);
		int i, p = -1;
		for (i = 0; i < c; i++){
			if (GetMenuItemID(hMenu, i) == IDM_TREE_EXPLORE){
				p = i;
				break;
			}
		}
		if (p >= 0){
			InsertMenu(hMenu, p, MF_STRING | MF_BYPOSITION, IDM_TREE_TOOL, TEXT("外部ツール(&X)\tCtrl+X"));
		}else{
			AppendMenu(hMenu, MF_STRING, IDM_TREE_TOOL, TEXT("外部ツール(&X)\tCtrl+X"));
		}
	}else{
		EnableMenuItem(hMenu, IDM_TREE_TOOL, MF_BYCOMMAND | MF_ENABLED);
	}
}

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg) {
	switch(pMsg->msg){
	case WM_INITMENUPOPUP:
		UpdateMenuItems((HMENU)pMsg->wp);
		break;
	}
	return FALSE;
}

static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode) {
	return TRUE;
}


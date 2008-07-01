#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../cplugin.h"
#include "wadsp.rh"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(linker, "/EXPORT:GetCPluginInfo=_GetCPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static ATOM F4B24_WADSP_INVOKE_CONFIG = 0;
static ATOM F4B24_WADSP_GET_LIST = 0;

static HMODULE hDLL = 0;

static DWORD CALLBACK GetConfigPageCount(void);
static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize);

static CONFIG_PLUGIN_INFO cpinfo = {
	0,
	CPDK_VER,
	GetConfigPageCount,
	GetConfigPage
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

static void PostF4B24Message(WPARAM wp, LPARAM lp){
	HWND hFittle = FindWindow(TEXT("Fittle"), NULL);
	if (hFittle && IsWindow(hFittle)) {
		PostMessage(hFittle, WM_F4B24_IPC, wp, lp);
	}
}

static void ViewConfig(HWND hDlg){
	if (F4B24_WADSP_GET_LIST)
		PostF4B24Message((WPARAM)F4B24_WADSP_GET_LIST, (LPARAM)GetDlgItem(hDlg, IDC_LIST1));
}

static BOOL CALLBACK WADSPPageProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp) {
	switch (msg){
	case WM_INITDIALOG:
		ViewConfig(hWnd);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wp) == IDC_BUTTON1){
			LRESULT lSel = SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETCURSEL, 0, 0);
			if (lSel != LB_ERR){
				lSel = SendMessage((HWND)lp, LB_GETITEMDATA, (WPARAM)lSel, (LPARAM)0);
				if (F4B24_WADSP_INVOKE_CONFIG)
					PostF4B24Message((WPARAM)F4B24_WADSP_INVOKE_CONFIG, (LPARAM)lSel);
			}
			return TRUE;
		}
		break;

	case WM_NOTIFY:
		if (((NMHDR *)lp)->code == PSN_APPLY){
		}
		return TRUE;
	}
	return FALSE;
}

static DWORD CALLBACK GetConfigPageCount(void){
	return 1;
}
static HPROPSHEETPAGE CreateConfigPage(){
	PROPSHEETPAGE psp;

	psp.dwSize = sizeof (PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = hDLL;
	psp.pszTemplate = TEXT("WADSP_SHEET");
	psp.pfnDlgProc = (DLGPROC)WADSPPageProc;
	return CreatePropertySheetPage(&psp);
}

static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize){
	if (!F4B24_WADSP_INVOKE_CONFIG) F4B24_WADSP_INVOKE_CONFIG = AddAtom(TEXT("F4B24_WADSP_INVOKE_CONFIG"));
	if (!F4B24_WADSP_GET_LIST) F4B24_WADSP_GET_LIST = AddAtom(TEXT("F4B24_WADSP_GET_LIST"));
	if (nIndex == 0 && nLevel == 1){
		lstrcpynA(pszConfigPath, "plugin/wadsp", nConfigPathSize);
		return CreateConfigPage();
	}
	return 0;
}

#ifdef __cplusplus
extern "C"
#endif
CONFIG_PLUGIN_INFO * CALLBACK GetCPluginInfo(void){
	return &cpinfo;
}

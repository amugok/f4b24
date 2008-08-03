#include "../../../fittle/src/fittle.h"
#include "../cplugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/src/f4b24lx.h"
#include "../../general/lplugin/lplugin.h"

#include "lplugin.rh"
#include <shlwapi.h>

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(linker, "/EXPORT:GetCPluginInfo=_GetCPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif


static HMODULE hDLL = 0;


#include "../../../fittle/src/wastr.cpp"

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

static void AddListItem(HWND hList, int nCode, LPCSTR lpszText) {
	LRESULT lSel = SendMessage(hList, LB_ADDSTRING, (WPARAM)0, (LPARAM)lpszText);
	if (lSel != LB_ERR && lSel != LB_ERRSPACE) {
		SendMessage(hList, LB_SETITEMDATA, (WPARAM)lSel, (LPARAM)nCode);
	}
}

void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}
void HFree(LPVOID pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}

static void ViewConfig(HWND hDlg){
	AddListItem(GetDlgItem(hDlg, IDC_LIST4), 0, "ファイル名");
	AddListItem(GetDlgItem(hDlg, IDC_LIST4), 1, "サイズ");
	AddListItem(GetDlgItem(hDlg, IDC_LIST4), 2, "種類");
	AddListItem(GetDlgItem(hDlg, IDC_LIST4), 3, "更新日時");
}

static BOOL CALLBACK LXPageProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp) {
	switch (msg){
	case WM_INITDIALOG:
		ViewConfig(hWnd);
		return TRUE;
	case WM_COMMAND:
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
	psp.pszTemplate = TEXT("LX_SHEET");
	psp.pfnDlgProc = (DLGPROC)LXPageProc;
	return CreatePropertySheetPage(&psp);
}

static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize){
	if (nIndex == 0 && nLevel == 1){
		WAIsUnicode();
		lstrcpynA(pszConfigPath, "plugin/lplugin", nConfigPathSize);
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

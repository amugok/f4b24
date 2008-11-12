
#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../cplugin.h"
#include "wadsp.rh"
#include <shlwapi.h>

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

static void SendF4B24Message(WPARAM wp, LPARAM lp){
	HWND hFittle = FindWindow(TEXT("Fittle"), NULL);
	if (hFittle && IsWindow(hFittle)){
		SendMessage(hFittle, WM_F4B24_IPC, wp, lp);
	}
}

static void AddListItem(HWND hDlg, int nCode, LPTSTR lpszText) {
	LRESULT lSel = SendDlgItemMessage(hDlg, IDC_LIST1, LB_ADDSTRING, (WPARAM)0, (LPARAM)lpszText);
	if (lSel != LB_ERR && lSel != LB_ERRSPACE) {
		SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETITEMDATA, (WPARAM)lSel, (LPARAM)nCode);
	}
}

void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}
void HFree(LPVOID pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}

static void ViewConfig(HWND hDlg){
	int l;
	LPTSTR p;
	HWND hBuf = GetDlgItem(hDlg, IDC_STATIC1);
	SendMessage(hBuf, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));
	SendF4B24Message((WPARAM)WM_F4B24_IPC_GET_WADSP_LIST, (LPARAM)hBuf);
	l = SendMessage(hBuf, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0) + 1;
	p = (LPTSTR)HAlloc(l * sizeof(TCHAR));
	if (p){
		int i = 0;
		LPTSTR q, r;
		SendMessage(hBuf, WM_GETTEXT, (WPARAM)l, (LPARAM)p);

		q = p;
		r = StrChr(q, TEXT('\n'));
		while (r) {
			*r = TEXT('\0');
			if (*q) AddListItem(hDlg, i, q);
			i++;
			q = r + 1;
			r = StrChr(q, TEXT('\n'));
		};
		if (*q) AddListItem(hDlg, i, q);
		HFree(p);
	}
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
				lSel = SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETITEMDATA, (WPARAM)lSel, (LPARAM)0);
				SendF4B24Message((WPARAM)WM_F4B24_IPC_INVOKE_WADSP_SETUP, (LPARAM)lSel);
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

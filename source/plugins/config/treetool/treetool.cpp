#define WINVER		0x0400	// 98以降
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4以降
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../cplugin.h"
#include "treetool.rh"

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
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static HMODULE m_hDLL = 0;

#include "../../../fittle/src/wastr.h"
#include "../../../fittle/src/wastr.cpp"

/*設定関係*/
static struct {
	WASTR szTreeToolPath;
} m_cfg;

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
		m_hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}


// 設定を読込
static void LoadConfig(){

	WASetIniFile(NULL, "Fittle.ini");


	WAGetIniStr("Tool", "PathT", &m_cfg.szTreeToolPath);
}

// 設定を保存
static void SaveConfig(){

	WASetIniFile(NULL, "Fittle.ini");

	WASetIniStr("Tool", "PathT", &m_cfg.szTreeToolPath);

	WAFlushIni();
}

static void ViewConfig(HWND hDlg){
	WASetDlgItemText(hDlg, IDC_EDIT1, &m_cfg.szTreeToolPath);
}

static BOOL CheckConfig(HWND hDlg){
	WASTR buf;
	WAGetDlgItemText(hDlg, IDC_EDIT1, &buf);
	if (WAstrcmp(&m_cfg.szTreeToolPath, &buf) != 0) return TRUE;
	return FALSE;
}

static void ApplyConfig(HWND hDlg){
	WAGetDlgItemText(hDlg, IDC_EDIT1, &m_cfg.szTreeToolPath);
}

static void PostF4B24Message(WPARAM wp, LPARAM lp){
	HWND hFittle = FindWindowA(TEXT("Fittle"), NULL);
	if (hFittle && IsWindow(hFittle)){
		PostMessageA(hFittle, WM_F4B24_IPC, wp, lp);
	}
}

static void ApplyFittle(){
	PostF4B24Message(WM_F4B24_IPC_APPLY_CONFIG, 0);
}

static BOOL CALLBACK MiniPanePageProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp) {
	switch (msg){
	case WM_INITDIALOG:
		LoadConfig();
		ViewConfig(hWnd);
	case WM_COMMAND:
		if (LOWORD(wp) == IDC_BUTTON1) {
			WASTR buf;
			WAGetDlgItemText(hWnd , IDC_EDIT1, &buf);
			if(WAOpenFilerPath(&buf, hWnd, "フォルダツリー用外部ツールを選択してください", "実行ファイル(*.exe);*.exe;", "exe")){
				WASetDlgItemText(hWnd, IDC_EDIT1, &buf);
			}
			return TRUE;
		}
		if (CheckConfig(hWnd))
			PropSheet_Changed(GetParent(hWnd) , hWnd);
		else
			PropSheet_UnChanged(GetParent(hWnd) , hWnd);
		return TRUE;

	case WM_NOTIFY:
		if (((NMHDR *)lp)->code == PSN_APPLY){
			ApplyConfig(hWnd);
			SaveConfig();
			ApplyFittle();
		}
		return TRUE;
	}
	return FALSE;
}

static DWORD CALLBACK GetConfigPageCount(void){
	return 1;
}

static HPROPSHEETPAGE CreateConfigPage(){
	WAIsUnicode();
	return WACreatePropertySheetPage(m_hDLL, "TREETOOL_SHEET", MiniPanePageProc);
}

static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize){
	if (nIndex == 0 && nLevel == 1){
		lstrcpynA(pszConfigPath, "plugin/treetool", nConfigPathSize);
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


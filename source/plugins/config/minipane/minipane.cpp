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
#include "minipane.rh"

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
#pragma comment(linker, "/EXPORT:OldMode=_OldMode@4")
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
	int nTagReverse;			// タイトル、アーティストを反転
	int nTrayClick[6];			// クリック時の動作
	int nMiniTop;
	int nMiniScroll;
	int nMiniTimeShow;
	int nMiniToolShow;
	WASTR szFontName;
	int nFontHeight;
	int nFontStyle;
} m_cfg;

static struct ControlSheetWork {
	WASTR szFontName;
	int nFontHeight;
	int nFontStyle;
} m_csw;


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
	int i;

	WASetIniFile(NULL, "minipane.ini");

	// クリック時の動作
	for(i=0;i<6;i++){
		CHAR szSec[10];
		wsprintfA(szSec, "Click%d", i);
		m_cfg.nTrayClick[i] = WAGetIniInt("MiniPanel", szSec, "\x0\x6\x8\x0\x5\x0"[i]);
	}

	// タグを反転
	m_cfg.nTagReverse = WAGetIniInt("MiniPanel", "TagReverse", 0);

	m_cfg.nMiniTop = WAGetIniInt("MiniPanel", "TopMost", 1);
	m_cfg.nMiniScroll = WAGetIniInt("MiniPanel", "Scroll", 1);
	m_cfg.nMiniTimeShow = WAGetIniInt("MiniPanel", "TimeShow", 1);
	m_cfg.nMiniToolShow = WAGetIniInt("MiniPanel", "ToolShow", 1);

	WAGetIniStr("MiniPanel", "FontName", &m_cfg.szFontName);
	m_cfg.nFontHeight = WAGetIniInt("MiniPanel", "FontHeight", 9);
	m_cfg.nFontStyle = WAGetIniInt("MiniPanel", "FontStyle", 0);}

// 設定を保存
static void SaveConfig(){
	int i;

	WASetIniFile(NULL, "minipane.ini");

	// タスクトレイを保存
	for(i=0;i<6;i++){
		CHAR szSec[10];
		wsprintfA(szSec, "Click%d", i);
		WASetIniInt("MiniPanel", szSec, m_cfg.nTrayClick[i]); //ホットキー
	}

	WASetIniInt("MiniPanel", "TagReverse", m_cfg.nTagReverse);

	WASetIniInt("MiniPanel", "TopMost", m_cfg.nMiniTop);
	WASetIniInt("MiniPanel", "Scroll", m_cfg.nMiniScroll);
	WASetIniInt("MiniPanel", "TimeShow", m_cfg.nMiniTimeShow);
	WASetIniInt("MiniPanel", "ToolShow", m_cfg.nMiniToolShow);

	WASetIniStr("MiniPanel", "FontName", &m_cfg.szFontName);
	WASetIniInt("MiniPanel", "FontHeight", m_cfg.nFontHeight);
	WASetIniInt("MiniPanel", "FontStyle", m_cfg.nFontStyle);

	WAFlushIni();
}

static void ViewConfig(HWND hDlg){
	int i;
	SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)m_cfg.nMiniTop, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, (WPARAM)m_cfg.nTagReverse, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, (WPARAM)m_cfg.nMiniTimeShow, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, (WPARAM)m_cfg.nMiniScroll, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, (WPARAM)m_cfg.nMiniToolShow, 0);
	for(i=0;i<6;i++){
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)0, (LPARAM)TEXT("なし"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)1, (LPARAM)TEXT("再生"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)2, (LPARAM)TEXT("一時停止"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)3, (LPARAM)TEXT("停止"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)4, (LPARAM)TEXT("前の曲"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)5, (LPARAM)TEXT("次の曲"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)6, (LPARAM)TEXT("ウィンドウ表示/非表示"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)7, (LPARAM)TEXT("終了"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)8, (LPARAM)TEXT("メニュー表示"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)9, (LPARAM)TEXT("再生/一時停止"));
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_SETCURSEL, (WPARAM)m_cfg.nTrayClick[i], (LPARAM)0);
	}
	WAstrcpy(&m_csw.szFontName, &m_cfg.szFontName);
	m_csw.nFontHeight = m_cfg.nFontHeight;
	m_csw.nFontStyle = m_cfg.nFontStyle;
}

static BOOL CheckConfig(HWND hDlg){
	int i;
	if (m_cfg.nMiniTop !=      (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nTagReverse !=   (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nMiniTimeShow != (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nMiniScroll !=   (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nMiniToolShow != (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0)) return TRUE;
	for(i=0;i<6;i++){
		if (m_cfg.nTrayClick[i] != SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) return TRUE;
	}
	if (WAstrcmp(&m_cfg.szFontName, &m_csw.szFontName) != 0) return TRUE;
	if (m_cfg.nFontHeight !=m_csw.nFontHeight) return TRUE;
	if (m_cfg.nFontStyle !=m_csw.nFontStyle) return TRUE;
	return false;
}

static void ApplyConfig(HWND hDlg){
	int i;
	m_cfg.nMiniTop =      (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
	m_cfg.nTagReverse =   (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0);
	m_cfg.nMiniTimeShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0);
	m_cfg.nMiniScroll =   (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0);
	m_cfg.nMiniToolShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0);
	for(i=0;i<6;i++){
		m_cfg.nTrayClick[i] = SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
	}
	m_cfg.nFontHeight = m_csw.nFontHeight;
	m_cfg.nFontStyle = m_csw.nFontStyle;
	WAstrcpy(&m_cfg.szFontName, &m_csw.szFontName);
}

static void ApplyFittle(){
	HWND hFittle = FindWindow(TEXT("Fittle"), NULL);
	if (hFittle && IsWindow(hFittle)){
		HWND hMiniPanel = (HWND)SendMessage(hFittle, WM_FITTLE, GET_MINIPANEL, 0);
		if (hMiniPanel && IsWindow(hMiniPanel)){
			PostMessage(hMiniPanel, WM_F4B24_IPC, WM_F4B24_IPC_APPLY_CONFIG, 0);
		}
	}
}

static BOOL CALLBACK MiniPanePageProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp) {
	switch (msg){
	case WM_INITDIALOG:
		LoadConfig();
		ViewConfig(hWnd);
	case WM_COMMAND:
		if (LOWORD(wp) == IDC_BUTTON2) {
			WAChooseFont(&m_csw.szFontName, &m_csw.nFontHeight, &m_csw.nFontStyle, hWnd);
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
	return WACreatePropertySheetPage(m_hDLL, "MINIPANE_SHEET", MiniPanePageProc);
}

static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize){
	if (nIndex == 0 && nLevel == 1){
		lstrcpynA(pszConfigPath, "plugin/minipane", nConfigPathSize);
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


#ifdef __cplusplus
extern "C"
#endif
void CALLBACK OldMode(HWND hWnd){
	HWND hTab = PropSheet_GetTabControl(hWnd);
	int nTab = TabCtrl_GetItemCount(hTab);
	BOOL fFound = FALSE;
	int i;
	for (i = 0; i < nTab; i++){
		TCHAR szBuf[32];
		TCITEM tci;
		tci.mask = TCIF_TEXT;
		tci.pszText = szBuf;
		tci.cchTextMax = 32;
		if (TabCtrl_GetItem(hTab, i, &tci)){
			if (lstrcmp(tci.pszText, TEXT("ミニパネル")) == 0){
				fFound = TRUE;
			}
		}
	}
	if (!fFound){
		HPROPSHEETPAGE hPage = CreateConfigPage();
		if (hPage){
			PropSheet_AddPage(hWnd, hPage);
		}
	}
}


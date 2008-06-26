#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../cplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(linker, "/EXPORT:GetCPluginInfo=_GetCPluginInfo@0")
#pragma comment(linker, "/EXPORT:OldMode=_OldMode@4")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static HMODULE hDLL = 0;

/*設定関係*/
static struct {
	int nTagReverse;			// タイトル、アーティストを反転
	int nTrayClick[6];			// クリック時の動作
	int nMiniTop;
	int nMiniScroll;
	int nMiniTimeShow;
	int nMiniToolShow;
	TCHAR szFontName[LF_FACESIZE];
	int nFontHeight;
	int nFontStyle;
} m_cfg;

static struct ControlSheetWork {
	TCHAR szFontName[LF_FACESIZE];
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
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

// 実行ファイルのパスを取得
void GetModuleParentDir(char *szParPath){
	char szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98以降
	*PathFindFileName(szParPath) = '\0';
	return;
}

// 整数型で設定ファイル書き込み
static int WritePrivateProfileInt(char *szAppName, char *szKeyName, int nData, char *szINIPath){
	char szTemp[100];

	wsprintf(szTemp, "%d", nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}

// 設定を読込
static void LoadConfig(){
	char m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, "minipane.ini");

	// クリック時の動作
	m_cfg.nTrayClick[0] = GetPrivateProfileInt("MiniPanel", "Click0", 0, m_szINIPath);
	m_cfg.nTrayClick[1] = GetPrivateProfileInt("MiniPanel", "Click1", 6, m_szINIPath);
	m_cfg.nTrayClick[2] = GetPrivateProfileInt("MiniPanel", "Click2", 8, m_szINIPath);
	m_cfg.nTrayClick[3] = GetPrivateProfileInt("MiniPanel", "Click3", 0, m_szINIPath);
	m_cfg.nTrayClick[4] = GetPrivateProfileInt("MiniPanel", "Click4", 5, m_szINIPath);
	m_cfg.nTrayClick[5] = GetPrivateProfileInt("MiniPanel", "Click5", 0, m_szINIPath);

	// タグを反転
	m_cfg.nTagReverse = GetPrivateProfileInt("MiniPanel", "TagReverse", 0, m_szINIPath);

	m_cfg.nMiniTop = (int)GetPrivateProfileInt("MiniPanel", "TopMost", 1, m_szINIPath);
	m_cfg.nMiniScroll = (int)GetPrivateProfileInt("MiniPanel", "Scroll", 1, m_szINIPath);
	m_cfg.nMiniTimeShow = (int)GetPrivateProfileInt("MiniPanel", "TimeShow", 1, m_szINIPath);
	m_cfg.nMiniToolShow = (int)GetPrivateProfileInt("MiniPanel", "ToolShow", 1, m_szINIPath);

	GetPrivateProfileString(TEXT("MiniPanel"), TEXT("FontName"), TEXT(""), m_cfg.szFontName, LF_FACESIZE, m_szINIPath);
	m_cfg.nFontHeight = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("FontHeight"), 9, m_szINIPath);
	m_cfg.nFontStyle = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("FontStyle"), 0, m_szINIPath);
}

// 設定を保存
static void SaveConfig(){
	int i;
	char szSec[10];

	char m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, "minipane.ini");

	// タスクトレイを保存
	for(i=0;i<6;i++){
		wsprintf(szSec, "Click%d", i);
		WritePrivateProfileInt("MiniPanel", szSec, m_cfg.nTrayClick[i], m_szINIPath); //ホットキー
	}

	WritePrivateProfileInt("MiniPanel", "TagReverse", m_cfg.nTagReverse, m_szINIPath);

	WritePrivateProfileInt("MiniPanel", "TopMost", m_cfg.nMiniTop, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "Scroll", m_cfg.nMiniScroll, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "TimeShow", m_cfg.nMiniTimeShow, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "ToolShow", m_cfg.nMiniToolShow, m_szINIPath);

	WritePrivateProfileString(TEXT("MiniPanel"), TEXT("FontName"), m_cfg.szFontName, m_szINIPath);
	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("FontHeight"), m_cfg.nFontHeight, m_szINIPath);
	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("FontStyle"), m_cfg.nFontStyle, m_szINIPath);

	WritePrivateProfileString(NULL, NULL, NULL, m_szINIPath);
}

static void ViewConfig(HWND hDlg){
	int i;
	SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)m_cfg.nMiniTop, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, (WPARAM)m_cfg.nTagReverse, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, (WPARAM)m_cfg.nMiniTimeShow, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, (WPARAM)m_cfg.nMiniScroll, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, (WPARAM)m_cfg.nMiniToolShow, 0);
	for(i=0;i<6;i++){
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)0, (LPARAM)"なし");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)1, (LPARAM)"再生");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)2, (LPARAM)"一時停止");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)3, (LPARAM)"停止");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)4, (LPARAM)"前の曲");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)5, (LPARAM)"次の曲");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)6, (LPARAM)"ウィンドウ表示/非表示");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)7, (LPARAM)"終了");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)8, (LPARAM)"メニュー表示");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)9, (LPARAM)"再生/一時停止");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_SETCURSEL, (WPARAM)m_cfg.nTrayClick[i], (LPARAM)0);
	}
	lstrcpyn(m_csw.szFontName, m_cfg.szFontName, LF_FACESIZE);
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
	if (lstrcmp(m_cfg.szFontName, m_csw.szFontName) != 0) return TRUE;
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
	lstrcpyn(m_cfg.szFontName, m_csw.szFontName, LF_FACESIZE);
}

static void ApplyFittle(){
	HWND hFittle = FindWindow("Fittle", NULL);
	if (hFittle && IsWindow(hFittle)){
		HWND hMiniPanel = (HWND)SendMessage(hFittle, WM_FITTLE, GET_MINIPANEL, 0);
		if (hMiniPanel && IsWindow(hMiniPanel)){
			PostMessage(hMiniPanel, WM_F4B24_IPC, WM_F4B24_IPC_APPLY_CONFIG, 0);
		}
	}
}

static BOOL SelectFont(HWND hDlg){
	CHOOSEFONT cf;
	LOGFONT lf;
	HDC hDC;

	hDC = GetDC(hDlg);
	ZeroMemory(&lf, sizeof(LOGFONT));
	lstrcpyn(lf.lfFaceName, m_csw.szFontName, LF_FACESIZE);
	lf.lfHeight = -MulDiv(m_csw.nFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	lf.lfItalic = (m_cfg.nFontStyle&ITALIC_FONTTYPE?TRUE:FALSE);
	lf.lfWeight = (m_cfg.nFontStyle&BOLD_FONTTYPE?FW_BOLD:0);
	ReleaseDC(hDlg, hDC);

	ZeroMemory(&cf, sizeof(CHOOSEFONT));
	lf.lfCharSet = SHIFTJIS_CHARSET;
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = hDlg;
	cf.lpLogFont = &lf;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
	cf.nFontType = SCREEN_FONTTYPE;
	if(ChooseFont(&cf)){
		lstrcpyn(m_csw.szFontName, lf.lfFaceName, LF_FACESIZE);
		m_csw.nFontStyle = cf.nFontType;
		m_csw.nFontHeight = cf.iPointSize / 10;
		return TRUE;
	}
	return FALSE;
}

static BOOL CALLBACK MiniPanePageProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp) {
	switch (msg){
	case WM_INITDIALOG:
		LoadConfig();
		ViewConfig(hWnd);
	case WM_COMMAND:
		if (LOWORD(wp) == IDC_BUTTON2) {
			if (!SelectFont(hWnd)) return TRUE;
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
	PROPSHEETPAGE psp;

	psp.dwSize = sizeof (PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = hDLL;
	psp.pszTemplate = TEXT("MINIPANE_SHEET");
	psp.pfnDlgProc = (DLGPROC)MiniPanePageProc;
	return CreatePropertySheetPage(&psp);
}

static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize){
	if (nIndex == 0 && nLevel == 1){
		lstrcpyn(pszConfigPath, "plugin/minipane", nConfigPathSize);
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
			if (lstrcmp(tci.pszText, "ミニパネル") == 0){
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


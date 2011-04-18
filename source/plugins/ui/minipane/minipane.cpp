/*
 * minipane
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/gplugin.h"
#include "../../../fittle/src/f4b24.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#ifdef UNICODE
#pragma comment(linker, "/EXPORT:GetGPluginInfo=_GetGPluginInfo@0")
#endif
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#if (_MSC_VER >= 1200) && (_MSC_VER < 1500)
#pragma comment(linker, "/OPT:NOWIN98")
#elif (_MSC_VER >= 1500) && (_MSC_VER < 1700)
#endif
#endif

// ウィンドウをサブクラス化、プロシージャハンドルをウィンドウに関連付ける
#define SET_SUBCLASS(hWnd, Proc) \
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Proc))

static BOOL CALLBACK MiniPanelProc(HWND, UINT, WPARAM, LPARAM);
static void LoadConfig();

#include "../../../fittle/src/wastr.cpp"

#ifdef UNICODE

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

static HWND m_hwndMain = NULL;

#else

static BOOL __cdecl OnInit();
static void __cdecl OnQuit();
static void __cdecl OnTrackChange();
static void __cdecl OnStatusChange();
static void __cdecl OnConfig();

static FITTLE_PLUGIN_INFO fpi = {
	PDK_VER,
	OnInit,
	OnQuit,
	OnTrackChange,
	OnStatusChange,
	OnConfig,
	NULL,
	NULL
};
#define m_hwndMain fpi.hParent

#ifdef __cplusplus
extern "C"
{
#endif
__declspec(dllexport) FITTLE_PLUGIN_INFO *GetPluginInfo(){
	return &fpi;
}
#ifdef __cplusplus
}
#endif

#endif
enum {PM_LIST=0, PM_RANDOM, PM_SINGLE};	// プレイモード

enum {
	WM_F4B24_INTERNAL_ON_PLAY_MODE = 1,
	WM_F4B24_INTERNAL_ON_REPEAT_MODE = 2,
	WM_F4B24_INTERNAL_APPLY_SETTING_OLD = 3
};

static HMENU hMiniMenu = 0;
static HMODULE m_hinstDLL = 0;
static HWND m_hMiniTool = NULL;		// ミニパネルツールバー
static HWND m_hMiniPanel = NULL;	// ミニパネルハンドル
static WNDPROC m_hOldProc = 0;
static HIMAGELIST m_hImageList = NULL;
static HFONT m_hFont = 0;

#ifdef UNICODE
#else
static HMODULE m_hdllConfig = NULL;
static BOOL m_fHookConfig = FALSE;
#endif

static int m_nMiniHeight = 30;
static int m_nTimeWidth = 70;

static int m_nTitleDisplayPos = 1;
static CHAR m_szTime[100] = { 0 };				// 再生時間
static TCHAR m_szTag[MAX_FITTLE_PATH] = { 0 };	// タグ

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
} m_cfg = { 0 };

/*状態保存関係*/
static struct {
	int nMiniPanel_x;
	int nMiniPanel_y;
	int nMiniPanelEnd;
	int nMiniWidth;
} m_sta = { 0 };

// 終了状態を読込
static void LoadState(){
	WASetIniFile(NULL, "minipane.ini");
	m_sta.nMiniPanel_x = (int)WAGetIniInt("MiniPanel", "x", 0);
	m_sta.nMiniPanel_y = (int)WAGetIniInt("MiniPanel", "y", 0);
	m_sta.nMiniPanelEnd = (int)WAGetIniInt("MiniPanel", "End", 0);
	m_sta.nMiniWidth = (int)WAGetIniInt("MiniPanel", "Width", 402);

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

	m_cfg.nMiniTop = (int)WAGetIniInt("MiniPanel", "TopMost", 1);
	m_cfg.nMiniScroll = (int)WAGetIniInt("MiniPanel", "Scroll", 1);
	m_cfg.nMiniTimeShow = (int)WAGetIniInt("MiniPanel", "TimeShow", 1);
	m_cfg.nMiniToolShow = (int)WAGetIniInt("MiniPanel", "ToolShow", 1);

	WAGetIniStr("MiniPanel", "FontName", &m_cfg.szFontName);
	m_cfg.nFontHeight = WAGetIniInt("MiniPanel", "FontHeight", 9);
	m_cfg.nFontStyle = WAGetIniInt("MiniPanel", "FontStyle", 0);
}


// 終了状態を保存
static void SaveState(){
	WASetIniFile(NULL, "minipane.ini");

	WASetIniInt("MiniPanel", "x", m_sta.nMiniPanel_x);
	WASetIniInt("MiniPanel", "y", m_sta.nMiniPanel_y);
	WASetIniInt("MiniPanel", "End", m_sta.nMiniPanelEnd);
	WASetIniInt("MiniPanel", "Width", m_sta.nMiniWidth);
	WAFlushIni();

	WASetIniFile(NULL, "Fittle.ini");

	WASetIniInt("MiniPanel", "End", m_sta.nMiniPanelEnd);
	WAFlushIni();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hinstDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static LRESULT FittlePluginInterface(int nCommand){
	return SendMessage(m_hwndMain, WM_FITTLE, nCommand, 0);
}

static int IsCheckedMainMenu(int nCommand){
	HMENU hMainMenu = (HMENU)FittlePluginInterface(GET_MENU);
	return (GetMenuState(hMainMenu, nCommand, MF_BYCOMMAND) & MF_CHECKED) != 0;
}

static int GetPlayMode(){
	if (IsCheckedMainMenu(IDM_PM_LIST))
		return PM_LIST;
	if (IsCheckedMainMenu(IDM_PM_RANDOM))
		return PM_RANDOM;
	if (IsCheckedMainMenu(IDM_PM_SINGLE))
		return PM_SINGLE;
	/* 不明の再生モード */
	return PM_LIST;
}

static int GetRepeatFlag(){
	return IsCheckedMainMenu(IDM_PM_REPEAT) ? TRUE : FALSE;
}

#ifdef UNICODE
static void GetF4B24String(int nCode, LPTSTR pszBuf, int nBufSize){
	HWND hwndWork = CreateWindow(TEXT("STATIC"),TEXT(""),0,0,0,0,0,NULL,NULL,m_hinstDLL,NULL);
	if (hwndWork){
		SendMessage(m_hwndMain, WM_F4B24_IPC, (WPARAM)nCode, (LPARAM)hwndWork);
		if (nBufSize > 0) pszBuf[0] = TEXT('\0');
		GetWindowText(hwndWork, pszBuf, nBufSize);
		//SendMessage(hwndWork, WM_GETTEXT, (WPARAM)nBufSize, (LPARAM)pszBuf);
		DestroyWindow(hwndWork);
	}
}
#endif

static void UpdatePanelTitle(){
#ifdef UNICODE
	TCHAR ptitle[MAX_FITTLE_PATH];
	TCHAR partist[MAX_FITTLE_PATH];
	GetF4B24String(WM_F4B24_IPC_GET_PLAYING_TITLE, ptitle, MAX_FITTLE_PATH);
	GetF4B24String(WM_F4B24_IPC_GET_PLAYING_ARTIST, partist, MAX_FITTLE_PATH);
	if (!ptitle[0])
		GetF4B24String(WM_F4B24_IPC_GET_PLAYING_PATH, ptitle, MAX_FITTLE_PATH);
#else
	static CHAR szNull[1] = { 0 };
	LPTSTR ptitle = (LPSTR)FittlePluginInterface(GET_TITLE);
	LPTSTR partist = (LPSTR)FittlePluginInterface(GET_ARTIST);
	if (!ptitle || !ptitle[0]) ptitle = (LPSTR)FittlePluginInterface(GET_PLAYING_PATH);
	if (!ptitle) ptitle = szNull;
	if (!partist) partist = szNull;
#endif

	if(!m_cfg.nTagReverse){
		if (!partist[0])
			lstrcpy(m_szTag, ptitle);
		else
			wsprintf(m_szTag, TEXT("%s / %s"), ptitle, partist);
	}else{
		wsprintf(m_szTag, TEXT("%s / %s"), partist, ptitle);
	}
}

static void UpdatePanelTime(){
	int nPos = FittlePluginInterface(GET_POSITION);
	int nLen = FittlePluginInterface(GET_DURATION);
	if (nPos >= 0 && nLen >= 0)
		wsprintfA(m_szTime, "\t%02d:%02d / %02d:%02d", nPos/60, nPos%60, nLen/60, nLen%60);
	else
		m_szTag[0] = 0;
}

static void SendCommandMessage(HWND hwnd, int nCommand){
	if (hwnd && IsWindow(hwnd))
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(nCommand, 0), 0);
}

static void SendCommand(int nCommand){
	SendCommandMessage(m_hwndMain, nCommand);
}
static void PanelClose(){
	SendCommandMessage(m_hMiniPanel, IDCANCEL);
}

static void FreeImageList(){
	if (m_hImageList){
		if (m_hMiniTool && IsWindow(m_hMiniTool)){
			SendMessage(m_hMiniTool, TB_SETIMAGELIST, 0, (LPARAM)0);
		}
		ImageList_Destroy(m_hImageList);
		m_hImageList = NULL;
	}
}

static void FreeMiniMenu(){
	if (hMiniMenu){
		DestroyMenu(hMiniMenu);
		hMiniMenu = 0;
	}
}

static void SetMiniToolCheck(int nCommand, int nSwitch){
	if (m_hMiniTool && IsWindow(m_hMiniTool))
		SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)nCommand, (LPARAM)MAKELONG(nSwitch, 0));
}

#define TB_BTN_NUM 8
#define TB_BMP_NUM 9
// ツールバーの作成
static HWND CreateToolBar(HWND hRebar){
	HBITMAP hBmp;
	TBBUTTON tbb[] = {
		{0, IDM_PLAY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{1, IDM_PAUSE, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
		{2, IDM_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{3, IDM_PREV, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{4, IDM_NEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
		{5, IDM_PM_TOGGLE, TBSTATE_ENABLED, TBSTYLE_DROPDOWN, 0L, 0},
		{6, IDM_PM_REPEAT, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
		//{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	};
	HWND hToolBar = NULL;

	hToolBar = CreateWindowEx(WS_EX_TOOLWINDOW,
		TOOLBARCLASSNAME,
		NULL,
		WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_TOOLTIPS,// | WS_CLIPSIBLINGS,// | WS_CLIPCHILDREN, 
		0, 0, 100, 100,
		hRebar,
		(HMENU)ID_TOOLBAR, m_hinstDLL, NULL);
	if(!hToolBar) return NULL;
	SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	hBmp = (HBITMAP)LoadImage(m_hinstDLL, TEXT("toolbarex.bmp"), IMAGE_BITMAP, 16*TB_BMP_NUM, 16, LR_LOADFROMFILE | LR_SHARED);
	if(hBmp==NULL)	hBmp = LoadBitmap(m_hinstDLL, MAKEINTRESOURCE(IDR_TOOLBAR1));
	
	//SendMessage(hToolBar, TB_AUTOSIZE, 0, 0) ;

	FreeImageList();
	m_hImageList = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, TB_BMP_NUM, 0);
	ImageList_AddMasked(m_hImageList, hBmp, RGB(0,255,0)); //緑を背景色に

	DeleteObject(hBmp);
	SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)m_hImageList);
	
	SendMessage(hToolBar, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
	SendMessage(hToolBar, TB_ADDBUTTONS, (WPARAM)TB_BTN_NUM, (LPARAM)&tbb);
	return hToolBar;
}


// ミニパネルのツールバーのプロシージャ
static LRESULT CALLBACK NewMiniToolProc(HWND hMT, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		//case WM_LBUTTONDOWN:
		//case WM_LBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
			SendMessage(GetParent(hMT), msg, wp, lp);
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hMT, GWLP_USERDATA), hMT, msg, wp, lp);
}

// ツールバーの幅を取得（ドロップダウンがあってもうまく行きます）
static LONG GetToolbarTrueWidth(HWND hToolbar){
	RECT rct;

	SendMessage(hToolbar, TB_GETITEMRECT, SendMessage(hToolbar, TB_BUTTONCOUNT, 0, 0)-1, (LPARAM)&rct);
	return rct.right;
}

static void PopupPlayModeMenu(HWND hWnd, NMTOOLBAR *lpnmtb){
	RECT rc;
	MENUITEMINFO mii;
	HMENU hPopMenu;

	int m_nPlayMode = GetPlayMode();
	SendMessage(hWnd, TB_GETRECT, (WPARAM)lpnmtb->iItem, (LPARAM)&rc);
	MapWindowPoints(hWnd, HWND_DESKTOP, (LPPOINT)&rc, 2);
	hPopMenu = CreatePopupMenu();
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.cch = 100;
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	mii.fType = MFT_STRING;
	mii.dwTypeData = TEXT("リスト(&L)");
	mii.wID = IDM_PM_LIST;
	if(m_nPlayMode==PM_LIST) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, 0, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = TEXT("ランダム(&R)");
	mii.wID = IDM_PM_RANDOM;
	if(m_nPlayMode==PM_RANDOM) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, IDM_PM_LIST, TRUE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = TEXT("シングル(&S)");
	mii.wID = IDM_PM_SINGLE;
	if(m_nPlayMode==PM_SINGLE) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, IDM_PM_RANDOM, TRUE, &mii);
	mii.fState = MFS_UNCHECKED;
	TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, rc.left, rc.bottom, 0, hWnd, NULL);
	DestroyMenu(hPopMenu);
	return;
}

static void PopupTrayMenu(HWND hWnd){
	POINT pt;
	HMENU m_hTrayMenu = GetSubMenu(hMiniMenu, 0);
	GetCursorPos(&pt);
	SetForegroundWindow(hWnd);
	TrackPopupMenu(m_hTrayMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
	PostMessage(hWnd, WM_NULL, 0, 0);
	return;
}

static void DoTrayClickAction(HWND hWnd, int nKind){
	switch(m_cfg.nTrayClick[nKind]){
	case 1:
		SendCommand(IDM_PLAY);
		break;
	case 2:
		SendCommand(IDM_PAUSE);
		break;
	case 3:
		SendCommand(IDM_STOP);
		break;
	case 4:
		SendCommand(IDM_PREV);
		break;
	case 5:
		SendCommand(IDM_NEXT);
		break;
	case 6:
		SendCommand(IDM_TRAY_WINVIEW);
		break;
	case 7:
		SendCommand(IDM_END);
		break;
	case 8:
		PopupTrayMenu(m_hwndMain);
		break;
	case 9:
		SendCommand(IDM_PLAYPAUSE);
		break;
	}
	return;
}

static void UpdateMenuPlayMode(){
	int m_nPlayMode = GetPlayMode();
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	SendMessage(m_hMiniTool,  TB_CHANGEBITMAP, (WPARAM)IDM_PM_TOGGLE, (LPARAM)MAKELONG(m_nPlayMode==PM_LIST?5:(m_nPlayMode==PM_RANDOM?7:8), 0));
	mii.fState = (m_nPlayMode == PM_LIST) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hMiniMenu, IDM_PM_LIST, FALSE, &mii);
	mii.fState = (m_nPlayMode == PM_RANDOM) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hMiniMenu, IDM_PM_RANDOM, FALSE, &mii);
	mii.fState = (m_nPlayMode == PM_SINGLE) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hMiniMenu, IDM_PM_SINGLE, FALSE, &mii);

	if (m_hMiniTool) SendMessage(m_hMiniTool,  TB_CHANGEBITMAP, (WPARAM)IDM_PM_TOGGLE, (LPARAM)MAKELONG(m_nPlayMode==PM_LIST?5:(m_nPlayMode==PM_RANDOM?7:8), 0));
}

static void UpdateMenuRepeatMode(){
	int m_nRepeatFlag = GetRepeatFlag();
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)IDM_PM_REPEAT, (LPARAM)MAKELONG(m_nRepeatFlag, 0));
	mii.fState = m_nRepeatFlag ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hMiniMenu, IDM_PM_REPEAT, FALSE, &mii);

	if (m_hMiniTool) SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)IDM_PM_REPEAT, (LPARAM)MAKELONG(m_nRepeatFlag, 0));
}

/*  既に実行中のFittleにパラメータを送信する */
static void SendCopyData(HWND hFittle, int iType, LPTSTR lpszString){
#ifdef UNICODE
	CHAR nameA[MAX_FITTLE_PATH];
	LPBYTE lpWork;
	COPYDATASTRUCT cds;
	DWORD la, lw, cbData;
	WideCharToMultiByte(CP_ACP, 0, lpszString, -1, nameA, MAX_FITTLE_PATH, NULL, NULL);
	la = lstrlenA(nameA) + 1;
	if (la & 1) la++; /* WORD align */
	lw = lstrlenW(lpszString) + 1;
	cbData = la * sizeof(CHAR) + lw * sizeof(WCHAR);
	lpWork = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbData);
	if (!lpWork) return;
	lstrcpyA((LPSTR)(lpWork), nameA);
	lstrcpyW((LPWSTR)(lpWork + la), lpszString);
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpWork;
	cds.cbData = cbData;
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
	HeapFree(GetProcessHeap(), 0, lpWork);
#else
	COPYDATASTRUCT cds;
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpszString;
	cds.cbData = (lstrlenA(lpszString) + 1) * sizeof(CHAR);
	// 文字列送信
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
#endif
}

#define MINIPANEL_SEPARATOR 4

static void UpdateFont(HWND hDlg){
	TCHAR szTime[] = TEXT("\t88:88 / 88:88");
	HDC hDC = GetDC(hDlg);
	HGDIOBJ hOldFont;
	LOGFONT lf;
	SIZE size;
	int nWidth = MulDiv(m_cfg.nFontHeight, GetDeviceCaps(hDC, LOGPIXELSX), 72);
	int nHeight = MulDiv(m_cfg.nFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	if (m_hFont) {
		DeleteObject(m_hFont);
		m_hFont = 0;
	}
	ZeroMemory(&lf, sizeof(LOGFONT));
	if (WAstrlen(&m_cfg.szFontName)) {
		WAstrcpyt(lf.lfFaceName, &m_cfg.szFontName, LF_FACESIZE);
	} else {
#ifdef UNICODE
		lstrcpy(lf.lfFaceName, TEXT("MS UI Gothic"));
#else
		lstrcpy(lf.lfFaceName, TEXT("ＭＳ Ｐゴシック"));
#endif
	}
	lf.lfItalic = (m_cfg.nFontStyle&ITALIC_FONTTYPE?TRUE:FALSE);
	lf.lfWeight = (m_cfg.nFontStyle&BOLD_FONTTYPE?FW_BOLD:0);
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfHeight = -nHeight;
	m_hFont = CreateFontIndirect(&lf);
	m_nMiniHeight = ((GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYEDGE)) << 1) + (nHeight > 16 ? nHeight : 16) + 1;
	hOldFont = SelectObject(hDC, (HGDIOBJ)m_hFont);
	GetTextExtentPoint32(hDC, szTime, lstrlen(szTime), &size);
	SelectObject(hDC, hOldFont);
	m_nTimeWidth = size.cx;
	ReleaseDC(hDlg, hDC);
}

static BOOL CALLBACK MiniPanelProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	HDC hDC;
	HGDIOBJ hOldFont;
	RECT rc;

	static int s_nToolWidth = 0;

	switch (msg) {
		case WM_INITDIALOG:	// 初期化

			LoadConfig();

			m_hMiniTool = CreateToolBar(hDlg);
			if(m_cfg.nMiniToolShow){
				s_nToolWidth = GetToolbarTrueWidth(m_hMiniTool) + GetSystemMetrics(SM_CXDLGFRAME)*2;
				ShowWindow(m_hMiniTool, SW_SHOWNA);
			}else{
				s_nToolWidth = GetSystemMetrics(SM_CXDLGFRAME)*2;
				ShowWindow(m_hMiniTool, SW_HIDE);
			}

			UpdateFont(hDlg);

			UpdateMenuPlayMode();
			UpdateMenuRepeatMode();
			UpdatePanelTitle();
			UpdatePanelTime();

			MoveWindow(hDlg, m_sta.nMiniPanel_x, m_sta.nMiniPanel_y, m_sta.nMiniWidth, m_nMiniHeight, FALSE);
			SetWindowPos(hDlg, (m_cfg.nMiniTop?HWND_TOPMOST:HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

			SET_SUBCLASS(m_hMiniTool, NewMiniToolProc);	// 仕方ないからサブクラス化

			m_nTitleDisplayPos = 1;
			switch (FittlePluginInterface(GET_STATUS)){
			case 0/*BASS_ACTIVE_STOPPED*/:
				SetMiniToolCheck(IDM_PAUSE, FALSE);
				break;
			case 1/*BASS_ACTIVE_PLAYING*/:
			case 2/*BASS_ACTIVE_STALLED*/:
				SetMiniToolCheck(IDM_PAUSE, FALSE);
				SetTimer(hDlg, ID_SEEKTIMER, 500, 0);
				break;
			case 3/*BASS_ACTIVE_PAUSED*/:
				SetMiniToolCheck(IDM_PAUSE, TRUE);
				break;
			}
			return TRUE;

		case WM_SIZE:
			m_sta.nMiniWidth = LOWORD(lp) + GetSystemMetrics(SM_CXDLGFRAME)*2;
			MoveWindow(m_hMiniTool, m_sta.nMiniWidth - s_nToolWidth, 1, 240, m_nMiniHeight, TRUE);
			return FALSE;

		case WM_GETMINMAXINFO:
			((LPMINMAXINFO)lp)->ptMaxTrackSize.y = m_nMiniHeight;
			((LPMINMAXINFO)lp)->ptMinTrackSize.y = m_nMiniHeight;
			((LPMINMAXINFO)lp)->ptMinTrackSize.x = s_nToolWidth + MINIPANEL_SEPARATOR;
			return FALSE;

		case WM_PAINT:
			PAINTSTRUCT ps;

			hDC = BeginPaint(hDlg, &ps);
			hOldFont = SelectObject(hDC, (HGDIOBJ)m_hFont);
			SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));

			// タイトル表示
			rc.left = 0;
			rc.top = 0;
			rc.bottom = m_nMiniHeight;
			rc.right = m_sta.nMiniWidth;
			FillRect(hDC, &rc, (HBRUSH)(COLOR_BTNFACE+1));
			TextOut(hDC, m_nTitleDisplayPos, 6, m_szTag, lstrlen(m_szTag));

			// 時間表示
			if(m_cfg.nMiniTimeShow){
				UpdatePanelTime();

				rc.left = m_sta.nMiniWidth - s_nToolWidth - m_nTimeWidth - MINIPANEL_SEPARATOR;
				FillRect(hDC, &rc, (HBRUSH)(COLOR_BTNFACE+1));
				TextOutA(hDC, rc.left + MINIPANEL_SEPARATOR, 6, m_szTime+1, lstrlenA(m_szTime+1));
			}

			SelectObject(hDC, hOldFont);
			EndPaint(hDlg, &ps);
			return TRUE;

		case WM_TIMER:
			switch(wp){
				case ID_SEEKTIMER:
					SIZE size;

					// 文字列幅取得
					hDC = GetDC(hDlg);
					hOldFont = SelectObject(hDC, (HGDIOBJ)m_hFont);
					GetTextExtentPoint32(hDC, m_szTag, lstrlen(m_szTag), &size);
					SelectObject(hDC, hOldFont);
					ReleaseDC(hDlg, hDC);

					// 文字列位置決定
					if(m_cfg.nMiniScroll && size.cx>=m_sta.nMiniWidth - s_nToolWidth - (m_cfg.nMiniTimeShow?m_nTimeWidth:0) - MINIPANEL_SEPARATOR){
						if(size.cx+m_nTitleDisplayPos<=1){
							m_nTitleDisplayPos = m_sta.nMiniWidth - s_nToolWidth - (m_cfg.nMiniTimeShow?m_nTimeWidth:0) - MINIPANEL_SEPARATOR;/*4*/;	// スクロールリセット
						}else{
							m_nTitleDisplayPos -= 3;	// スクロール
						}
					}else{
						m_nTitleDisplayPos = 1;		// 十分な表示領域があるので固定
					}

					// 再描画
					rc.left = 0;
					rc.top = 0;
					rc.bottom = m_nMiniHeight;
					rc.right = m_sta.nMiniWidth - s_nToolWidth;
					InvalidateRect(hDlg, &rc, FALSE);
					return TRUE;

				case ID_RBTNCLKTIMER:
					KillTimer(hDlg, ID_RBTNCLKTIMER);
					DoTrayClickAction(m_hwndMain, 2);
					return TRUE;

				case ID_MBTNCLKTIMER:
					KillTimer(hDlg, ID_MBTNCLKTIMER);
					DoTrayClickAction(m_hwndMain, 4);
					return TRUE;

			}
			return FALSE;

		case WM_MOVE:
			GetWindowRect(hDlg, &rc);
			m_sta.nMiniPanel_x = rc.left;
			m_sta.nMiniPanel_y = rc.top;
			return TRUE;
	
		case WM_DESTROY:
			if (m_hFont){
				DeleteObject(m_hFont);
				m_hFont = 0;
			}
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp)){
				case IDCANCEL:

					KillTimer(hDlg, ID_SEEKTIMER);
					m_sta.nMiniPanelEnd = 0;
					m_hMiniPanel = NULL;
					m_hMiniTool = NULL;

					ShowWindow(m_hwndMain, SW_SHOW);
					DestroyWindow(hDlg);
					return TRUE;

				case IDM_PLAY:
				case IDM_PAUSE:
				case IDM_PREV:
				case IDM_STOP:
				case IDM_NEXT:
				case IDM_PM_REPEAT:
				case IDM_PM_TOGGLE:
				case IDM_PM_LIST:
				case IDM_PM_RANDOM:
				case IDM_PM_SINGLE:
					SendCommand(wp);
					return TRUE;
			}
			return FALSE;

		/*-- こっからマウスイベント --*/

		case WM_RBUTTONDOWN:
			SetTimer(hDlg, ID_RBTNCLKTIMER, (m_cfg.nTrayClick[3]?GetDoubleClickTime():0), NULL);
			return FALSE;

		case WM_MBUTTONDOWN:
			SetTimer(hDlg, ID_MBTNCLKTIMER, (m_cfg.nTrayClick[5]?GetDoubleClickTime():0), NULL);
			return FALSE;;

		case WM_RBUTTONDBLCLK:
			KillTimer(hDlg, ID_RBTNCLKTIMER);
			DoTrayClickAction(m_hwndMain, 3);
			return TRUE;

		case WM_MBUTTONDBLCLK:
			KillTimer(hDlg, ID_MBTNCLKTIMER);
			DoTrayClickAction(m_hwndMain, 5);
			return TRUE;

		case WM_LBUTTONDOWN:
			SendMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);
			return FALSE;

		case WM_LBUTTONDBLCLK:
			DoTrayClickAction(m_hwndMain, 1);
			return TRUE;

		case WM_MOUSEWHEEL:
			if(GetKeyState(VK_SHIFT)<0){
				if((short)HIWORD(wp)<0){
					SendCommand(IDM_SEEKFRONT);
				}else{
					SendCommand(IDM_SEEKBACK);
				}
			}else{
				if((short)HIWORD(wp)<0){
					SendCommand(IDM_VOLDOWN);
				}else{
					SendCommand(IDM_VOLUP);
				}
			}
			return TRUE;

		/*-- ここまでマウスイベント --*/

		case WM_NOTIFY:
			NMHDR *pnmhdr;

			pnmhdr = (LPNMHDR)lp;
			if(pnmhdr->hwndFrom==m_hMiniTool && pnmhdr->code==TBN_DROPDOWN){
				PopupPlayModeMenu(pnmhdr->hwndFrom, (NMTOOLBAR *)lp);
				return TRUE;
			}
			return FALSE;

		case WM_DROPFILES:
			HDROP hDrop;
			TCHAR szPath[MAX_FITTLE_PATH];

			hDrop = (HDROP)wp;
			DragQueryFile(hDrop, 0, szPath, MAX_FITTLE_PATH);
			DragFinish(hDrop);
			SendCopyData(m_hwndMain, 0, szPath);

			return TRUE;

		case WM_F4B24_IPC:
			if (wp == WM_F4B24_INTERNAL_ON_PLAY_MODE){
				UpdateMenuPlayMode();
			}else if (wp == WM_F4B24_INTERNAL_ON_REPEAT_MODE){
				UpdateMenuRepeatMode();
			}else if (wp == WM_F4B24_INTERNAL_APPLY_SETTING_OLD || wp == WM_F4B24_IPC_APPLY_CONFIG){
				LoadConfig();

				UpdateFont(hDlg);

				MoveWindow(hDlg, m_sta.nMiniPanel_x, m_sta.nMiniPanel_y, m_sta.nMiniWidth, m_nMiniHeight, TRUE);

				SetWindowPos(hDlg, (m_cfg.nMiniTop?HWND_TOPMOST:HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				if(m_cfg.nMiniToolShow){
					s_nToolWidth = GetToolbarTrueWidth(m_hMiniTool) + GetSystemMetrics(SM_CXDLGFRAME)*2;
					ShowWindow(m_hMiniTool, SW_SHOWNA);
				}else{
					s_nToolWidth = GetSystemMetrics(SM_CXDLGFRAME)*2;
					ShowWindow(m_hMiniTool, SW_HIDE);
				}

				UpdatePanelTitle();
				UpdatePanelTime();

				InvalidateRect(hDlg, NULL, TRUE);
			}
			return TRUE;

		default:
			return FALSE;
	}
}


static void OnInitSub(){
	HMENU hMainMenu = (HMENU)FittlePluginInterface(GET_MENU);
	EnableMenuItem(hMainMenu, IDM_MINIPANEL, MF_BYCOMMAND | MF_ENABLED);

#ifdef UNICODE
#else
	if (SendMessage(m_hwndMain, WM_F4B24_IPC, WM_F4B24_IPC_GET_IF_VERSION, 0) < 12){
		TCHAR m_szPathFCP[MAX_FITTLE_PATH];
		GetModuleFileName(m_hinstDLL, m_szPathFCP, MAX_FITTLE_PATH);
		lstrcpy(PathFindExtension(PathFindFileName(m_szPathFCP)), TEXT(".fcp"));
		m_hdllConfig = LoadLibrary(m_szPathFCP);
		m_fHookConfig = (m_hdllConfig != NULL);
	}
#endif
	
	hMiniMenu = LoadMenu(m_hinstDLL, TEXT("TRAYMENU"));

	LoadState();
}

/* 終了時に一度だけ呼ばれます */
static void __cdecl OnQuit(){
	SaveState();
	FreeImageList();
	FreeMiniMenu();
	return;
}

/* 曲が変わる時に呼ばれます */
static void __cdecl OnTrackChange(){
	m_nTitleDisplayPos = 1;	// 表示位置リセット
	UpdatePanelTitle();
	UpdatePanelTime();
	if (m_hMiniPanel) InvalidateRect(m_hMiniPanel, NULL, FALSE);
}

/* 再生状態が変わる時に呼ばれます */
static void __cdecl  OnStatusChange(){
	if (m_hMiniPanel)
	{
		switch (FittlePluginInterface(GET_STATUS)){
		case 0/*BASS_ACTIVE_STOPPED*/:
			SetMiniToolCheck(IDM_PAUSE, FALSE);
			KillTimer(m_hMiniPanel, ID_SEEKTIMER);
			InvalidateRect(m_hMiniPanel, NULL, TRUE);
			m_szTag[0] = 0;
			break;
		case 1/*BASS_ACTIVE_PLAYING*/:
		case 2/*BASS_ACTIVE_STALLED*/:
			SetMiniToolCheck(IDM_PAUSE, FALSE);
			SetTimer(m_hMiniPanel, ID_SEEKTIMER, 500, 0);
			InvalidateRect(m_hMiniPanel, NULL, FALSE);
			break;
		case 3/*BASS_ACTIVE_PAUSED*/:
			SetMiniToolCheck(IDM_PAUSE, TRUE);
			KillTimer(m_hMiniPanel, ID_SEEKTIMER);
			break;
		}
	}
}

/* 設定画面を呼び出します（未実装）*/
static void __cdecl OnConfig(){
}

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg) {
	switch(pMsg->msg){
	case WM_SETFOCUS:
		if(!m_hMiniPanel) break;
		SetFocus(m_hMiniPanel);
		pMsg->lMsgResult = TRUE;
		return TRUE;

	case WM_SYSCOMMAND:
	case WM_COMMAND:
		switch (LOWORD(pMsg->wp)){
		case IDM_PM_TOGGLE:
		case IDM_PM_LIST:
		case IDM_PM_RANDOM:
		case IDM_PM_SINGLE:
			PostMessage(m_hMiniPanel, WM_F4B24_IPC, WM_F4B24_INTERNAL_ON_PLAY_MODE, 0);
			break;
		case IDM_PM_REPEAT:
			PostMessage(m_hMiniPanel, WM_F4B24_IPC, WM_F4B24_INTERNAL_ON_REPEAT_MODE, 0);
			break;
		case IDM_MINIPANEL:
			if(!m_hMiniPanel){
				ShowWindow(pMsg->hWnd, SW_HIDE);
				m_sta.nMiniPanelEnd = 1;
				m_hMiniPanel = CreateDialogParam(m_hinstDLL, TEXT("MINI_PANEL"), pMsg->hWnd, MiniPanelProc, 0);
			}else{
				PanelClose();
			}
			pMsg->lMsgResult = TRUE;
			return TRUE;
		case IDM_TRAY_WINVIEW:
			if(m_hMiniPanel){
				PanelClose();
				pMsg->lMsgResult = TRUE;
				return TRUE;
			}
			break;
		}
		break;
	case WM_FITTLE:
		if (pMsg->wp == GET_MINIPANEL) {
			pMsg->lMsgResult = (m_hMiniPanel && IsWindow(m_hMiniPanel)) ? (LRESULT)m_hMiniPanel : (LRESULT)0;
			return TRUE;
		}
		break;
	case WM_INITMENUPOPUP:
		EnableMenuItem((HMENU)pMsg->wp, IDM_MINIPANEL, MF_BYCOMMAND | MF_ENABLED);
		break;
	}
	return FALSE;
}

#ifdef UNICODE

static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode) {
	if (eCode == GENERAL_PLUGIN_EVENT_INIT) {
		WAIsUnicode();
		m_hwndMain = hWnd;
		OnInitSub();
	} else if (eCode == GENERAL_PLUGIN_EVENT_QUIT) {
		OnQuit();
	} else if (eCode == GENERAL_PLUGIN_EVENT_TRACK_CHANGE) {
		OnTrackChange();
	} else if (eCode == GENERAL_PLUGIN_EVENT_STATUS_CHANGE) {
		OnStatusChange();
	}
	return TRUE;
}


#else

// サブクラス化したウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	GENERAL_PLUGIN_HOOK_WNDPROC hwk = { hWnd, msg, wp, lp, 0};
	if (msg ==  WM_ACTIVATE) {
		HWND hConfig = FindWindowEx(hWnd, NULL, TEXT("#32770"), TEXT("設定"));
//		HWND hConfig = FindWindow(TEXT("#32770"), TEXT("設定"));
		if (hConfig && IsWindow(hConfig) && GetWindowThreadProcessId(hWnd, NULL) == GetWindowThreadProcessId(hConfig, NULL)){
			typedef void (CALLBACK * LPFNOLDMODE)(HWND hwnd);
			LPFNOLDMODE pfnoldmode = (LPFNOLDMODE)GetProcAddress(m_hdllConfig, "OldMode");
			if (pfnoldmode){
				pfnoldmode(hConfig);
			}
		}
	}
	if (HookWndProc(&hwk)) return hwk.lMsgResult;
	return CallWindowProc(m_hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL __cdecl OnInit(){
	WAIsUnicode();
	m_hOldProc = (WNDPROC)SetWindowLong(m_hwndMain, GWL_WNDPROC, (LONG)WndProc);
	OnInitSub();
	return TRUE;
}



#endif


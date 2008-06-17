/*
 * minipane
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

// ウィンドウをサブクラス化、プロシージャハンドルをウィンドウに関連付ける
#define SET_SUBCLASS(hWnd, Proc) \
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Proc))

static BOOL CALLBACK MiniPanelProc(HWND, UINT, WPARAM, LPARAM);
static void LoadConfig();
static BOOL OnInit();
static void OnQuit();
static void OnTrackChange();
static void OnStatusChange();
static void OnConfig();

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
static HMODULE m_hdllConfig = NULL;
static BOOL m_fHookConfig = FALSE;

static int m_nTitleDisplayPos = 1;
static TCHAR m_szTime[100];			// 再生時間
static TCHAR m_szTag[MAX_FITTLE_PATH];	// タグ

/*設定関係*/
static int nTagReverse;			// タイトル、アーティストを反転
static int nTrayClick[6];			// クリック時の動作
static int nMiniPanel_x;
static int nMiniPanel_y;
static int nMiniPanelEnd;
static int nMiniTop;
static int nMiniWidth;
static int nMiniScroll;
static int nMiniTimeShow;
static int nMiniToolShow;

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

void GetModuleParentDir(LPTSTR szParPath){
	TCHAR szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98以降
	*PathFindFileName(szParPath) = '\0';
	return;
}

// 終了状態を読込
static void LoadState(){
	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("minipane.ini"));

	nMiniPanel_x = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("x"), 0, m_szINIPath);
	nMiniPanel_y = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("y"), 0, m_szINIPath);
	nMiniPanelEnd = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("End"), 0, m_szINIPath);
	nMiniWidth = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("Width"), 402, m_szINIPath);

}

// 設定を読込
static void LoadConfig(){
	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("minipane.ini"));

	// クリック時の動作
	nTrayClick[0] = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("Click0"), 0, m_szINIPath);
	nTrayClick[1] = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("Click1"), 6, m_szINIPath);
	nTrayClick[2] = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("Click2"), 8, m_szINIPath);
	nTrayClick[3] = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("Click3"), 0, m_szINIPath);
	nTrayClick[4] = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("Click4"), 5, m_szINIPath);
	nTrayClick[5] = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("Click5"), 0, m_szINIPath);

	// タグを反転
	nTagReverse = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("TagReverse"), 0, m_szINIPath);

	nMiniTop = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("TopMost"), 1, m_szINIPath);
	nMiniScroll = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("Scroll"), 1, m_szINIPath);
	nMiniTimeShow = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("TimeShow"), 1, m_szINIPath);
	nMiniToolShow = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("ToolShow"), 1, m_szINIPath);
}

//Int型で設定ファイル書き込み
static int WritePrivateProfileInt(LPTSTR szAppName, LPTSTR szKeyName, int nData, LPTSTR szINIPath){
	TCHAR szTemp[100];

	wsprintf(szTemp, TEXT("%d"), nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}

// 終了状態を保存
static void SaveState(){
	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("minipane.ini"));


	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("x"), nMiniPanel_x, m_szINIPath);
	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("y"), nMiniPanel_y, m_szINIPath);
	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("End"), nMiniPanelEnd, m_szINIPath);
	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("Width"), nMiniWidth, m_szINIPath);
	WritePrivateProfileString(NULL, NULL, NULL, m_szINIPath);

	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("Fittle.ini"));
	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("End"), nMiniPanelEnd, m_szINIPath);
	WritePrivateProfileString(NULL, NULL, NULL, m_szINIPath);
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
	return SendMessage(fpi.hParent, WM_FITTLE, nCommand, 0);
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
		SendMessage(fpi.hParent, WM_F4B24_IPC, (WPARAM)nCode, (LPARAM)hwndWork);
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

	if(!nTagReverse){
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
		wsprintf(m_szTime, TEXT("\t%02d:%02d / %02d:%02d"), nPos/60, nPos%60, nLen/60, nLen%60);
	else
		m_szTag[0] = 0;
}

static void SendCommandMessage(HWND hwnd, int nCommand){
	if (hwnd && IsWindow(hwnd))
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(nCommand, 0), 0);
}

static void SendCommand(int nCommand){
	SendCommandMessage(fpi.hParent, nCommand);
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

// サブクラス化したウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_ACTIVATE:
		{
//			HWND hConfig = FindWindowEx(hWnd, NULL, TEXT("#32770"), TEXT("設定"));
			HWND hConfig = FindWindow(TEXT("#32770"), TEXT("設定"));
			if (hConfig && IsWindow(hConfig) && GetWindowThreadProcessId(hWnd, NULL) == GetWindowThreadProcessId(hConfig, NULL)){
				typedef void (CALLBACK * LPFNOLDMODE)(HWND hwnd);
				LPFNOLDMODE pfnoldmode = (LPFNOLDMODE)GetProcAddress(m_hdllConfig, "OldMode");
				if (pfnoldmode){
					pfnoldmode(hConfig);
				}
			}
		}
		break;
	case WM_SETFOCUS:
		if(!m_hMiniPanel) break;
		SetFocus(m_hMiniPanel);
		return TRUE;

	case WM_SYSCOMMAND:
	case WM_COMMAND:
		switch (LOWORD(wp)){
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
				ShowWindow(hWnd, SW_HIDE);
				nMiniPanelEnd = 1;
				m_hMiniPanel = CreateDialogParam(m_hinstDLL, TEXT("MINI_PANEL"), hWnd, MiniPanelProc, 0);
			}else{
				PanelClose();
			}
			return TRUE;
		case IDM_TRAY_WINVIEW:
			if(m_hMiniPanel){
				PanelClose();
				return TRUE;
			}
			break;
		}
	case WM_FITTLE:
		if (wp == GET_MINIPANEL)		{
			return (m_hMiniPanel && IsWindow(m_hMiniPanel)) ? (LRESULT)m_hMiniPanel : (LRESULT)0;
		}
		break;
	case WM_INITMENUPOPUP:
		EnableMenuItem((HMENU)wp, IDM_MINIPANEL, MF_BYCOMMAND | MF_ENABLED);
		break;
	}
	return CallWindowProc(m_hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	HMENU hMainMenu = (HMENU)FittlePluginInterface(GET_MENU);
	EnableMenuItem(hMainMenu, IDM_MINIPANEL, MF_BYCOMMAND | MF_ENABLED);

	if (SendMessage(fpi.hParent, WM_F4B24_IPC, WM_F4B24_IPC_GET_IF_VERSION, 0) < 12){
		TCHAR m_szPathFCP[MAX_FITTLE_PATH];
		GetModuleFileName(m_hinstDLL, m_szPathFCP, MAX_FITTLE_PATH);
		lstrcpy(PathFindExtension(PathFindFileName(m_szPathFCP)), TEXT(".fcp"));
		m_hdllConfig = LoadLibrary(m_szPathFCP);
		m_fHookConfig = (m_hdllConfig != NULL);
	}

	hMiniMenu = LoadMenu(m_hinstDLL, TEXT("TRAYMENU"));
	m_hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);

	LoadState();

	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
static void OnQuit(){
	SaveState();
	FreeImageList();
	FreeMiniMenu();
	return;
}

/* 曲が変わる時に呼ばれます */
static void OnTrackChange(){
	m_nTitleDisplayPos = 1;	// 表示位置リセット
	UpdatePanelTitle();
	UpdatePanelTime();
	if (m_hMiniPanel) InvalidateRect(m_hMiniPanel, NULL, FALSE);
}

/* 再生状態が変わる時に呼ばれます */
static void OnStatusChange(){
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
static void OnConfig(){
}

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
	switch(nTrayClick[nKind]){
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
		PopupTrayMenu(fpi.hParent);
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

#define MINIPANEL_HEIGHT 30
#define MINIPANEL_TIME_WIDTH 70
#define MINIPANEL_SEPARATOR 4

static BOOL CALLBACK MiniPanelProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	HGDIOBJ hOldFont;
	static HFONT s_hFont = 0;
	static int s_nToolWidth = 0;

	switch (msg)
	{
		case WM_INITDIALOG:	// 初期化
			LOGFONT lf;
			HDC hDC;

			LoadConfig();

			m_hMiniTool = CreateToolBar(hDlg);
			if(nMiniToolShow){
				s_nToolWidth = GetToolbarTrueWidth(m_hMiniTool) + GetSystemMetrics(SM_CXDLGFRAME)*2;
				ShowWindow(m_hMiniTool, SW_SHOWNA);
			}else{
				s_nToolWidth = GetSystemMetrics(SM_CXDLGFRAME)*2;
				ShowWindow(m_hMiniTool, SW_HIDE);
			}

			hDC = GetDC(hDlg);
			ZeroMemory(&lf, sizeof(LOGFONT));
			lstrcpy(lf.lfFaceName, TEXT("ＭＳ Ｐゴシック"));
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfHeight = -MulDiv(9, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			s_hFont = CreateFontIndirect(&lf);
			ReleaseDC(hDlg, hDC);

			UpdateMenuPlayMode();
			UpdateMenuRepeatMode();
			UpdatePanelTitle();
			UpdatePanelTime();

			MoveWindow(hDlg, nMiniPanel_x, nMiniPanel_y, nMiniWidth, MINIPANEL_HEIGHT, FALSE);
			SetWindowPos(hDlg, (nMiniTop?HWND_TOPMOST:HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

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
			nMiniWidth = LOWORD(lp) + GetSystemMetrics(SM_CXDLGFRAME)*2;
			MoveWindow(m_hMiniTool, nMiniWidth - s_nToolWidth, 1, 240, 36, TRUE);
			return FALSE;

		case WM_GETMINMAXINFO:
			((LPMINMAXINFO)lp)->ptMaxTrackSize.y = MINIPANEL_HEIGHT;
			((LPMINMAXINFO)lp)->ptMinTrackSize.y = MINIPANEL_HEIGHT;
			((LPMINMAXINFO)lp)->ptMinTrackSize.x = s_nToolWidth + MINIPANEL_SEPARATOR;
			return FALSE;

		case WM_PAINT:
			PAINTSTRUCT ps;
			RECT rc;

			hDC = BeginPaint(hDlg, &ps);
			hOldFont = SelectObject(hDC, (HGDIOBJ)s_hFont);
			SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));

			// タイトル表示
			rc.left = 0;
			rc.top = 0;
			rc.bottom = MINIPANEL_HEIGHT;
			rc.right = nMiniWidth;
			FillRect(hDC, &rc, (HBRUSH)(COLOR_BTNFACE+1));
			TextOut(hDC, m_nTitleDisplayPos, 6, m_szTag, lstrlen(m_szTag));

			// 時間表示
			if(nMiniTimeShow){
				UpdatePanelTime();

				rc.left = nMiniWidth - s_nToolWidth - MINIPANEL_TIME_WIDTH - MINIPANEL_SEPARATOR;
				FillRect(hDC, &rc, (HBRUSH)(COLOR_BTNFACE+1));
				TextOut(hDC, rc.left + MINIPANEL_SEPARATOR, 6, m_szTime+1, lstrlen(m_szTime+1));
			}

			SelectObject(hDC, hOldFont);
			EndPaint(hDlg, &ps);
			return TRUE;

		case WM_TIMER:
			switch(wp){
				case ID_SEEKTIMER:
					HDC hDC;
					SIZE size;

					// 文字列幅取得
					hDC = GetDC(hDlg);
					hOldFont = SelectObject(hDC, (HGDIOBJ)s_hFont);
					GetTextExtentPoint32(hDC, m_szTag, lstrlen(m_szTag), &size);
					SelectObject(hDC, hOldFont);
					ReleaseDC(hDlg, hDC);

					// 文字列位置決定
					if(nMiniScroll && size.cx>=nMiniWidth - s_nToolWidth - (nMiniTimeShow?MINIPANEL_TIME_WIDTH:0) - MINIPANEL_SEPARATOR){
						if(size.cx+m_nTitleDisplayPos<=1){
							m_nTitleDisplayPos = nMiniWidth - s_nToolWidth - (nMiniTimeShow?MINIPANEL_TIME_WIDTH:0) - MINIPANEL_SEPARATOR;/*4*/;	// スクロールリセット
						}else{
							m_nTitleDisplayPos -= 3;	// スクロール
						}
					}else{
						m_nTitleDisplayPos = 1;		// 十分な表示領域があるので固定
					}

					// 再描画
					rc.left = 0;
					rc.top = 0;
					rc.bottom = MINIPANEL_HEIGHT;
					rc.right = nMiniWidth - s_nToolWidth;
					InvalidateRect(hDlg, &rc, FALSE);
					return TRUE;

				case ID_RBTNCLKTIMER:
					KillTimer(hDlg, ID_RBTNCLKTIMER);
					DoTrayClickAction(fpi.hParent, 2);
					return TRUE;

				case ID_MBTNCLKTIMER:
					KillTimer(hDlg, ID_MBTNCLKTIMER);
					DoTrayClickAction(fpi.hParent, 4);
					return TRUE;

			}
			return FALSE;

		case WM_MOVE:
			{
				RECT rc;
				GetWindowRect(hDlg, &rc);
				nMiniPanel_x = rc.left;
				nMiniPanel_y = rc.top;
			}
			return TRUE;
	
		case WM_DESTROY:
			if (s_hFont){
				DeleteObject(s_hFont);
				s_hFont = 0;
			}
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp)){
				case IDCANCEL:

					KillTimer(hDlg, ID_SEEKTIMER);
					nMiniPanelEnd = 0;
					m_hMiniPanel = NULL;
					m_hMiniTool = NULL;

					ShowWindow(fpi.hParent, SW_SHOW);
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
			SetTimer(hDlg, ID_RBTNCLKTIMER, (nTrayClick[3]?GetDoubleClickTime():0), NULL);
			return FALSE;

		case WM_MBUTTONDOWN:
			SetTimer(hDlg, ID_MBTNCLKTIMER, (nTrayClick[5]?GetDoubleClickTime():0), NULL);
			return FALSE;;

		case WM_RBUTTONDBLCLK:
			KillTimer(hDlg, ID_RBTNCLKTIMER);
			DoTrayClickAction(fpi.hParent, 3);
			return TRUE;

		case WM_MBUTTONDBLCLK:
			KillTimer(hDlg, ID_MBTNCLKTIMER);
			DoTrayClickAction(fpi.hParent, 5);
			return TRUE;

		case WM_LBUTTONDOWN:
			SendMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);
			return FALSE;

		case WM_LBUTTONDBLCLK:
			DoTrayClickAction(fpi.hParent, 1);
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
			SendCopyData(fpi.hParent, 0, szPath);

			return TRUE;

		case WM_F4B24_IPC:
			if (wp == WM_F4B24_INTERNAL_ON_PLAY_MODE){
				UpdateMenuPlayMode();
			}else if (wp == WM_F4B24_INTERNAL_ON_REPEAT_MODE){
				UpdateMenuRepeatMode();
			}else if (wp == WM_F4B24_INTERNAL_APPLY_SETTING_OLD || wp == WM_F4B24_IPC_APPLY_CONFIG){
				LoadConfig();

				SetWindowPos(hDlg, (nMiniTop?HWND_TOPMOST:HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				if(nMiniToolShow){
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

/*
 * minipane
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../fittle/resource/resource.h"
#include "../fittle/src/fittle.h"
#include "../fittle/src/plugin.h"

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


#define WM_USER_MINIPANEL (WM_USER + 88)

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

static HMENU hMiniMenu = 0;
static HMODULE m_hinstDLL = 0;
static HWND m_hMiniTool = NULL;		// ミニパネルツールバー
static HWND m_hMiniPanel = NULL;	// ミニパネルハンドル
static WNDPROC m_hOldProc = 0;

static int m_nTitleDisplayPos = 1;
static char m_szTime[100];			// 再生時間
static char m_szTag[MAX_FITTLE_PATH];	// タグ

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

void GetModuleParentDir(char *szParPath){
	char szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98以降
	*PathFindFileName(szParPath) = '\0';
	return;
}

// 設定を読込
static void LoadConfig(){
	char m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, "minipane.ini");

	// クリック時の動作
	nTrayClick[0] = GetPrivateProfileInt("TaskTray", "Click0", 6, m_szINIPath);
	nTrayClick[1] = GetPrivateProfileInt("TaskTray", "Click1", 0, m_szINIPath);
	nTrayClick[2] = GetPrivateProfileInt("TaskTray", "Click2", 8, m_szINIPath);
	nTrayClick[3] = GetPrivateProfileInt("TaskTray", "Click3", 0, m_szINIPath);
	nTrayClick[4] = GetPrivateProfileInt("TaskTray", "Click4", 5, m_szINIPath);
	nTrayClick[5] = GetPrivateProfileInt("TaskTray", "Click5", 0, m_szINIPath);

	// タグを反転
	nTagReverse = GetPrivateProfileInt("Main", "TagReverse", 0, m_szINIPath);

	nMiniPanel_x = (int)GetPrivateProfileInt("MiniPanel", "x", 0, m_szINIPath);
	nMiniPanel_y = (int)GetPrivateProfileInt("MiniPanel", "y", 0, m_szINIPath);
	nMiniPanelEnd = (int)GetPrivateProfileInt("MiniPanel", "End", 0, m_szINIPath);
	nMiniTop = (int)GetPrivateProfileInt("MiniPanel", "TopMost", 1, m_szINIPath);
	nMiniWidth = (int)GetPrivateProfileInt("MiniPanel", "Width", 402, m_szINIPath);
	nMiniScroll = (int)GetPrivateProfileInt("MiniPanel", "Scroll", 1, m_szINIPath);
	nMiniTimeShow = (int)GetPrivateProfileInt("MiniPanel", "TimeShow", 1, m_szINIPath);
	nMiniToolShow = (int)GetPrivateProfileInt("MiniPanel", "ToolShow", 1, m_szINIPath);
}

//Int型で設定ファイル書き込み
static int WritePrivateProfileInt(char *szAppName, char *szKeyName, int nData, char *szINIPath){
	char szTemp[100];

	wsprintf(szTemp, "%d", nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
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
		WritePrivateProfileInt("TaskTray", szSec, nTrayClick[i], m_szINIPath); //ホットキー
	}

	WritePrivateProfileInt("Main", "TagReverse", nTagReverse, m_szINIPath);

	WritePrivateProfileInt("MiniPanel", "x", nMiniPanel_x, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "y", nMiniPanel_y, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "End", nMiniPanelEnd, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "TopMost", nMiniTop, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "Width", nMiniWidth, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "Scroll", nMiniScroll, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "TimeShow", nMiniTimeShow, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "ToolShow", nMiniToolShow, m_szINIPath);

	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, "Fittle.ini");
	WritePrivateProfileInt("MiniPanel", "End", nMiniPanelEnd, m_szINIPath);
}

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

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hinstDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}



static int GetPlayMode()
{
	HMENU hMainMenu = (HMENU)SendMessage(fpi.hParent, WM_FITTLE, GET_MENU, 0);
	if (GetMenuState(hMainMenu, IDM_PM_LIST, MF_BYCOMMAND) & MF_CHECKED)
		return PM_LIST;
	if (GetMenuState(hMainMenu, IDM_PM_RANDOM, MF_BYCOMMAND) & MF_CHECKED)
		return PM_RANDOM;
	if (GetMenuState(hMainMenu, IDM_PM_SINGLE, MF_BYCOMMAND) & MF_CHECKED)
		return PM_SINGLE;
	/* 不明の再生モード */
	return PM_LIST;
}

static int GetRepeatFlag()
{
	HMENU hMainMenu = (HMENU)SendMessage(fpi.hParent, WM_FITTLE, GET_MENU, 0);
	if (GetMenuState(hMainMenu, IDM_PM_REPEAT, MF_BYCOMMAND) & MF_CHECKED)
		return TRUE;
	return FALSE;
}

static void UpdatePanelTitle()
{
	char *ptitle = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);
	char *partist = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_ARTIST, 0);
	if (!ptitle || !partist || (!ptitle[0] && !partist[0])){
		char *ppath = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);
		if (!ppath || !ppath[0])
			m_szTag[0] = 0;
		else
		lstrcpy(m_szTag, ppath);
	}else if(!nTagReverse){
		wsprintf(m_szTag, "%s / %s", ptitle, partist);
	}else{
		wsprintf(m_szTag, "%s / %s", partist, ptitle);
	}
}

static void UpdatePanelTime()
{
	int nPos = SendMessage(fpi.hParent, WM_FITTLE, GET_POSITION, 0);
	int nLen = SendMessage(fpi.hParent, WM_FITTLE, GET_DURATION, 0);
	if (nPos >= 0 && nLen >= 0)
		wsprintf(m_szTime, "\t%02d:%02d / %02d:%02d", nPos/60, nPos%60, nLen/60, nLen%60);
	else
		wsprintf(m_szTime, "\t--:-- / --:--");
}

static void SendCommand(int nCommand)
{
	SendMessage(fpi.hParent, WM_COMMAND, MAKEWPARAM(nCommand, 0), 0);
}
static void PanelClose()
{
	if (m_hMiniPanel) SendMessage(m_hMiniPanel, WM_COMMAND, MAKEWPARAM(IDCANCEL,0), 0);
}

// サブクラス化したウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
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
			PostMessage(m_hMiniPanel, WM_USER_MINIPANEL, 1, 0);
			break;
		case IDM_PM_REPEAT:
			PostMessage(m_hMiniPanel, WM_USER_MINIPANEL, 2, 0);
			break;
		case IDM_MINIPANEL:
			if(!m_hMiniPanel){
				ShowWindow(hWnd, SW_HIDE);
				nMiniPanelEnd = 1;
				m_hMiniPanel = CreateDialogParam(m_hinstDLL, "MINI_PANEL", hWnd, MiniPanelProc, 0);
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
	case WM_INITMENUPOPUP:
		EnableMenuItem((HMENU)wp, IDM_MINIPANEL, MF_BYCOMMAND | MF_ENABLED);
		break;
	}
	return CallWindowProc(m_hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	HMENU hMainMenu = (HMENU)SendMessage(fpi.hParent, WM_FITTLE, GET_MENU, 0);
	EnableMenuItem(hMainMenu, IDM_MINIPANEL, MF_BYCOMMAND | MF_ENABLED);

	hMiniMenu = LoadMenu(m_hinstDLL, "TRAYMENU");

	m_hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);

	LoadConfig();

	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
static void OnQuit(){
	SaveConfig();
	if (hMiniMenu){
		DestroyMenu(hMiniMenu);
		hMiniMenu = 0;
	}
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
		switch (SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0)){
		case 0/*BASS_ACTIVE_STOPPED*/:
			if (m_hMiniTool) SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
			KillTimer(m_hMiniPanel, ID_SEEKTIMER);
			InvalidateRect(m_hMiniPanel, NULL, TRUE);
			m_szTag[0] = 0;
			break;
		case 1/*BASS_ACTIVE_PLAYING*/:
		case 2/*BASS_ACTIVE_STALLED*/:
			if (m_hMiniTool) SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
			SetTimer(m_hMiniPanel, ID_SEEKTIMER, 500, 0);
			InvalidateRect(m_hMiniPanel, NULL, FALSE);
			break;
		case 3/*BASS_ACTIVE_PAUSED*/:
			if (m_hMiniTool) SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(TRUE, 0));
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
	HIMAGELIST hToolImage;
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
	hBmp = (HBITMAP)LoadImage(m_hinstDLL, "toolbarex.bmp", IMAGE_BITMAP, 16*TB_BMP_NUM, 16, LR_LOADFROMFILE | LR_SHARED);
	if(hBmp==NULL)	hBmp = LoadBitmap(m_hinstDLL, (char *)IDR_TOOLBAR1);
	
	//SendMessage(hToolBar, TB_AUTOSIZE, 0, 0) ;
	
	hToolImage = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, TB_BMP_NUM, 0);
	ImageList_AddMasked(hToolImage, hBmp, RGB(0,255,0)); //緑を背景色に

	DeleteObject(hBmp);
	SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)hToolImage);
	
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
	mii.dwTypeData = "リスト(&L)";
	mii.wID = IDM_PM_LIST;
	if(m_nPlayMode==PM_LIST) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, 0, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = "ランダム(&R)";
	mii.wID = IDM_PM_RANDOM;
	if(m_nPlayMode==PM_RANDOM) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, IDM_PM_LIST, TRUE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = "シングル(&S)";
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

#define MINIPANEL_HEIGHT 30
#define MINIPANEL_TIME_WIDTH 70
#define MINIPANEL_SEPARATOR 4

static BOOL CALLBACK MiniPanelProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	static HFONT s_hFont;
	static int s_nToolWidth = 0;

	switch (msg)
	{
		case WM_INITDIALOG:	// 初期化
			LOGFONT lf;
			HDC hDC;

			m_hMiniTool = CreateToolBar(hDlg);
			if(nMiniToolShow){
				s_nToolWidth = GetToolbarTrueWidth(m_hMiniTool) + GetSystemMetrics(SM_CXDLGFRAME)*2;
			}else{
				s_nToolWidth = GetSystemMetrics(SM_CXDLGFRAME)*2;
				ShowWindow(m_hMiniTool, SW_HIDE);
			}

			hDC = GetDC(hDlg);
			ZeroMemory(&lf, sizeof(LOGFONT));
			lstrcpy(lf.lfFaceName, "ＭＳ Ｐゴシック");
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfHeight = -MulDiv(9, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			s_hFont = CreateFontIndirect(&lf);
			ReleaseDC(hDlg, hDC);

			UpdateMenuPlayMode();
			UpdateMenuRepeatMode();
			UpdatePanelTitle();
			UpdatePanelTime();

			SetWindowPos(hDlg, (nMiniTop?HWND_TOPMOST:0), nMiniPanel_x, nMiniPanel_y, nMiniWidth, MINIPANEL_HEIGHT, SWP_SHOWWINDOW);

			SET_SUBCLASS(m_hMiniTool, NewMiniToolProc);	// 仕方ないからサブクラス化

			m_nTitleDisplayPos = 1;
			switch (SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0)){
			case 0/*BASS_ACTIVE_STOPPED*/:
				SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
				break;
			case 1/*BASS_ACTIVE_PLAYING*/:
			case 2/*BASS_ACTIVE_STALLED*/:
				SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
				SetTimer(hDlg, ID_SEEKTIMER, 500, 0);
				break;
			case 3/*BASS_ACTIVE_PAUSED*/:
				SendMessage(m_hMiniTool,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(TRUE, 0));
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
			SelectObject(hDC, (HGDIOBJ)s_hFont);
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
			
			EndPaint(hDlg, &ps);
			return TRUE;

		case WM_TIMER:
			switch(wp){
				case ID_SEEKTIMER:
					HDC hDC;
					SIZE size;

					// 文字列幅取得
					hDC = GetDC(hDlg);
					SelectObject(hDC, (HGDIOBJ)s_hFont);
					GetTextExtentPoint32(hDC, m_szTag, lstrlen(m_szTag), &size);
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

		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDCANCEL:
					RECT rcMini;
					nMiniPanelEnd = 0;
					GetWindowRect(hDlg, &rcMini);
					nMiniPanel_x = rcMini.left;
					nMiniPanel_y = rcMini.top;
					DeleteObject(s_hFont);
					KillTimer(hDlg, ID_SEEKTIMER);
					m_hMiniPanel = NULL;
					m_hMiniTool = NULL;
					ShowWindow(fpi.hParent, SW_SHOW);
					EndDialog(hDlg, 0);
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
			PanelClose();
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
			char szPath[MAX_FITTLE_PATH];
			COPYDATASTRUCT cds;

			hDrop = (HDROP)wp;
			DragQueryFile(hDrop, 0, szPath, MAX_FITTLE_PATH);
			DragFinish(hDrop);
			cds.dwData = 0;
			cds.lpData = (void *)szPath;
			cds.cbData = lstrlen(szPath) + 1;
			SendMessage(fpi.hParent, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
			return TRUE;

		case WM_USER_MINIPANEL:
			if (wp == 1){
				UpdateMenuPlayMode();
			}
			if (wp == 2){
				UpdateMenuRepeatMode();
			}
			return TRUE;

		default:
			return FALSE;
	}
}


/*
 * jacket_panel
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include <gdiplus.h>
#include <shlwapi.h>

#define IDC_PANEL 200

using namespace Gdiplus;

BOOL OnInit();
void OnQuit();
void OnTrackChange();
void OnStatusChange();
void OnConfig();

FITTLE_PLUGIN_INFO fpi = {
	PDK_VER,
	OnInit,
	OnQuit,
	OnTrackChange,
	OnStatusChange,
	OnConfig,
	NULL,
	NULL
};

typedef struct{
	char szTitle[256];
	char szArtist[256];
	char szAlbum[256];
	char szTrack[10];
}TAGINFO;

static HWND hTree;
static WNDPROC hOldWndProc;
static WNDPROC hOldPanelProc;
//static WNDPROC hOldTreeProc;
static HWND hPanel;
static RECT rcPanel;
static ULONG_PTR gdiToken;
static TCHAR str[MAX_PATH*2] = {0}; 

#define ODS(X) OutputDebugString(X); OutputDebugString("\n"); 

void ShowPanel(BOOL bShow){
	RECT rc;
	int h = 0;
	int height;
	int width;

	// GetClientRect(fpi.hParent, &rc);
	// SendMessage(fpi.hParent, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));

	if(bShow){
		GetWindowRect(hTree, &rc);
		height = rc.bottom - rc.top;
		width = rc.right - rc.left;

		h = height / 2;					

		rcPanel.left = rc.left;
		rcPanel.top = rc.top + h;
		rcPanel.right = rc.right;
		rcPanel.bottom = rcPanel.top + height;
		MapWindowPoints(HWND_DESKTOP, fpi.hParent, (LPPOINT)&rcPanel, 2);
		SetWindowPos(hTree, NULL, rcPanel.left, rcPanel.top - h, width, h, SWP_SHOWWINDOW);
		SetWindowPos(hPanel, HWND_TOP, rcPanel.left, rcPanel.top, width, h, SWP_SHOWWINDOW);
	}else{
		ShowWindow(hPanel, SW_HIDE);
	}
	return;
}

/*LRESULT CALLBACK TreeProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_SIZE:
			ShowPanel(FALSE);
			break;

		default:
			break;
	}
	return CallWindowProc(hOldTreeProc, hWnd, msg, wp, lp);
}*/

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_SIZE:
			LockWindowUpdate(fpi.hParent);
			CallWindowProc(hOldWndProc, hWnd, msg, wp, lp);
			if(PathFileExists(str))
				ShowPanel(TRUE);
			else
				ShowPanel(FALSE);
			LockWindowUpdate(NULL);
			return 0;

		default:
			break;
	}
	return CallWindowProc(hOldWndProc, hWnd, msg, wp, lp);
}

void OnPaint(HWND hWnd){
	HDC hDC;
	PAINTSTRUCT ps;

	hDC = BeginPaint(hWnd, &ps);

	WCHAR wstr[MAX_PATH*2];
	RECT rc;

	MultiByteToWideChar(CP_ACP, 0, str,  -1, wstr, MAX_PATH);	// S-JIS -> Unicode
	Image myImage(wstr);
				
	GetClientRect(hPanel, &rc);
	FillRect(hDC, &rc, (HBRUSH)(COLOR_BTNFACE+1));

	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;
	if(width>height){
		rc.left += (width - height) / 2;
		width = height;
	}else{
		rc.top += (height - width) / 2;
		height = width;
	}

	Graphics myGraphics(hDC);
	myGraphics.DrawImage(&myImage, rc.left, rc.top, width, height);

	EndPaint(hWnd, &ps);

	return;
}

/* パネルプロシージャ */
LRESULT CALLBACK PanelProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_PAINT:
			OnPaint(hWnd);
			break;
	}
	return CallWindowProc(hOldPanelProc, hWnd, msg, wp, lp);
}

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE /*hModule*/, DWORD /*dwFunction*/, LPVOID /*lpNot*/){
    return TRUE;
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	// GDI+の初期化
	GdiplusStartupInput gdiSI;
	GdiplusStartup(&gdiToken, &gdiSI, NULL);

	// ツリーのサブクラス化
	hTree = (HWND)SendMessage(fpi.hParent, WM_FITTLE, GET_CONTROL, 102); 	
	//hOldTreeProc = (WNDPROC)SetWindowLong(hTree, GWL_WNDPROC, (LONG)TreeProc);

	hOldWndProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);


	// パネル作成
	hPanel = CreateWindowEx(0,
							"STATIC",
							NULL,
							WS_CHILD | WS_VISIBLE | SS_NOTIFY,// | SS_WHITERECT,
							0, 0, 0, 0,
							fpi.hParent,
							(HMENU)IDC_PANEL,
							(HINSTANCE)GetWindowLongPtr(fpi.hParent, GWLP_HINSTANCE),
							0);
	hOldPanelProc = (WNDPROC)SetWindowLong(hPanel, GWL_WNDPROC, (LONG)PanelProc);

	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	GdiplusShutdown(gdiToken);
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	LPSTR pszJacketPath;
	pszJacketPath= (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);
	ODS(pszJacketPath);

	TCHAR szPath[MAX_PATH*2];
	lstrcpy(szPath, pszJacketPath);
	ODS(szPath);

	LPSTR pszFileName;
	pszFileName = PathFindFileName(szPath);
	ODS(pszFileName);

	TAGINFO *pTag = (TAGINFO *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);
	//lstrcpy(pszFileName, pTag->szAlbum);
	lstrcpy(pszFileName, "folder");
	lstrcat(pszFileName, ".jpg");

	lstrcpy(str, szPath);	
	ODS(str);
	
	//ShowPanel(TRUE);
	//InvalidateRect(hPanel, NULL, TRUE);
	RECT rc;
	GetClientRect(fpi.hParent, &rc);
	SendMessage(fpi.hParent, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
	return;
}

/* 再生状態が変わる時に呼ばれます */
void OnStatusChange(){
	return;
}

/* 設定画面を呼び出します（未実装）*/
void OnConfig(){
	return;
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
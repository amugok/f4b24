/*
 * opentext
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */


#ifdef _WIN64
#pragma comment(lib,"..\\..\\..\\..\\extra\\smartvc14\\smartvc14_x64.lib")
#pragma comment(linker,"/ENTRY:SmartDllStartup")
#else
#pragma comment(lib,"..\\..\\..\\..\\extra\\smartvc14\\smartvc14_x86.lib")
#pragma comment(linker,"/ENTRY:SmartDllStartup@12")
#endif

#include "../../../fittle/src/plugin.h"
#include <shlwapi.h>
#include <commctrl.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

#define IDM_OPENTEXT 45111	// 値は適当。かぶらないようにする機構が欲しいところ
#define ID_TOOLBAR	106

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

static WNDPROC hOldProc;

// サブクラス化したウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_COMMAND:
			if(LOWORD(wp)==IDM_OPENTEXT){
				char *pszPath = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);
				char szPath[MAX_PATH*2];
				lstrcpyn(szPath, pszPath, MAX_PATH*2);
				PathRenameExtension(szPath, ".txt");
				if(PathFileExists(szPath)){
					ShellExecute(fpi.hParent, "open", szPath, NULL, NULL, SW_SHOWNORMAL);
				}
				return 0;
			}
			break;
	}
	return CallWindowProc(hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	hOldProc = (WNDPROC)SetWindowLongPtr(fpi.hParent, GWLP_WNDPROC, (LONG_PTR)WndProc);
	HWND hToolbar = (HWND)SendMessage(fpi.hParent, WM_FITTLE, GET_CONTROL, ID_TOOLBAR);

	TBADDBITMAP tbab = {HINST_COMMCTRL, IDB_STD_SMALL_COLOR};
	SendMessage(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);

	TBBUTTON tbb[2] = {
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
		{9+STD_FILENEW, IDM_OPENTEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	};
	SendMessage(hToolbar, TB_ADDBUTTONS, 2, (LPARAM)&tbb);

	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
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
/*
 * timecap
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

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

#include "../../../fittle/src/plugin.h"

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

#define ID_TIMER 1000

static char szTitle[MAX_PATH];
static WNDPROC hOldProc;

// サブクラス化したウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	char szCaption[MAX_PATH];
	int nPos;

	switch(msg){
		case WM_TIMER:
			if(wp==ID_TIMER){
				nPos = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_POSITION, 0);
				if(nPos>=0){
					wsprintf(szCaption, "%d:%02d %s", nPos/60, nPos%60, szTitle);
					SetWindowText(fpi.hParent, szCaption);
				}
				return 0;
			}
			break;
	}
	return CallWindowProc(hOldProc, hWnd, msg, wp, lp);
}

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
    return TRUE;
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	hOldProc = (WNDPROC)SetWindowLongPtr(fpi.hParent, GWLP_WNDPROC, (LONG_PTR)WndProc);
	return (hOldProc != NULL) ? TRUE : FALSE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	char szCaption[MAX_PATH];

	GetWindowText(fpi.hParent, szTitle, MAX_PATH);
	wsprintf(szCaption, "0:00 %s", szTitle);	// 再生開始時のディレイを軽減
	SetWindowText(fpi.hParent, szCaption);
	return;
}

/* 再生状態が変わる時に呼ばれます */
void OnStatusChange(){
	int n;
	switch(n = SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0)){
		case 0: // 停止
		case 3: // 一時停止
			KillTimer(fpi.hParent, ID_TIMER);
			break;
		case 1: // 再生
			SetTimer(fpi.hParent, ID_TIMER, 500, NULL);
			break;
	}
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
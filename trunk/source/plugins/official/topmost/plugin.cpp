/*
 * topmost
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"

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
	static BOOL s_bTopMost = FALSE;
	switch(msg){
		case WM_NCMBUTTONUP:
			s_bTopMost = !s_bTopMost;
			SetWindowPos(fpi.hParent, s_bTopMost?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);			
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

	hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);

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
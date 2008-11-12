/*
 * plugin.cpp - mixi_music_winamp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include "MainWnd.h"

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

HINSTANCE g_hInst = NULL;;
HWND g_hMainWnd = NULL;
HANDLE hThread  = NULL;

/* スレッド */
DWORD WINAPI ThreadFunc(LPVOID /*param*/)
{
	return MyWinMain();
}

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE /*hModule*/, DWORD /*dwFunction*/, LPVOID /*lpNot*/){
    return TRUE;
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	// ダミーウィンドウを作って抜ける
	DWORD tid;
	hThread = CreateThread(NULL, 0, ThreadFunc, 0, 0, &tid);
	g_hInst = fpi.hDllInst;
	return hThread?TRUE:FALSE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	// 後処理です
	SendMessage(g_hMainWnd, WM_DESTROY, 0, 0);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	// gen_mixi_for_winamp.dllはタイトルバーのテキストの切り替わりで切り替わりの感知してるっぽい
	TAGINFO *ti = (TAGINFO *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);
	char szBuff[512] = {0};
	wsprintf(szBuff, "%s - %s - Winamp", ti->szArtist, ti->szTitle);
	SetWindowText(g_hMainWnd, szBuff);
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
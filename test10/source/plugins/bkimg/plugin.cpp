/*
 * bkimg
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include <commctrl.h>

#define FILE_EXIST(X) (GetFileAttributes(X)==0xFFFFFFFF ? FALSE : TRUE)

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

static HWND hLV;

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
    return TRUE;
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	LVBKIMAGE lvbki;
	char szPath[MAX_PATH];
	int nLen;

	CoInitialize(NULL);

	nLen = GetModuleFileName(fpi.hDllInst, szPath, MAX_PATH);
	lstrcpy(&szPath[nLen-3], "bmp");

	if(!FILE_EXIST(szPath)){
		CoUninitialize();
		return FALSE;
	}

	ZeroMemory(&lvbki, sizeof(LVBKIMAGE));
	lvbki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
	lvbki.pszImage = szPath;

	hLV = (HWND)SendMessage(fpi.hParent, WM_FITTLE, GET_LISTVIEW, 0);

	ListView_SetBkImage(hLV, &lvbki);
	ListView_SetTextBkColor(hLV, CLR_NONE);

	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	LVBKIMAGE lvbki;

	// 背景を元に戻さないと落ちる
	ZeroMemory(&lvbki, sizeof(LVBKIMAGE));
	lvbki.ulFlags = LVBKIF_SOURCE_NONE;
	ListView_SetBkImage(hLV, &lvbki);

	CoUninitialize();
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
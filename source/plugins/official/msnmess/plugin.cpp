/*
 * msnmess
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include "shlwapi.h"

#pragma comment(lib, "shlwapi.lib")

#define LIVEMESS	// これを定義するとLive Messenger用になります

#define MAX_TAG_SIZE 256

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

BOOL bTagReverse;	// アーティストとタイトルを逆にする。Fittle.iniから設定を読み取ります

// MSN Messenger Current Playing Song
void ChangeMSNWindow(LPSTR pszTitle, LPSTR pszArtist, LPSTR pszAlbum, BOOL bShow){
	COPYDATASTRUCT msndata;
	WCHAR buffer[MAX_TAG_SIZE*3];
	WCHAR wArtist[MAX_TAG_SIZE], wTitle[MAX_TAG_SIZE], wAlbum[MAX_TAG_SIZE];
	char *pszPath;

	// S-JIS -> Unicode
	MultiByteToWideChar(CP_ACP, 0, pszArtist,  -1, wArtist, MAX_TAG_SIZE); 
	MultiByteToWideChar(CP_ACP, 0, pszTitle, -1, wTitle, MAX_TAG_SIZE);
	MultiByteToWideChar(CP_ACP, 0, pszAlbum, -1, wAlbum, MAX_TAG_SIZE);

	// ファイル名のみなら拡張子を削除
	if(lstrlen(pszArtist)==0 && lstrlen(pszAlbum)==0){
		pszPath = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);
		if(!lstrcmpi(PathFindFileName(pszPath), pszTitle)){
            WCHAR *p = PathFindExtensionW(wTitle);
			if (p) *p = L'\0';
		}
	}

	// 送信バッファを作成
#ifdef LIVEMESS	// Windows Live Messenger
	wsprintfW(buffer, L"\\0Music\\0%d\\0%s\\0%s\\0%s\\0%s\\0", bShow, wTitle, wArtist, wAlbum, L"WMContentID");
#else			// MSN Messenger
	wsprintfW(buffer, L"\\0Music\\0%d\\0%s\\0%s\\0%s\\0%s\\0%s\\0", bShow, L"{0} - {1}", wTitle, wArtist, wAlbum, L"WMContentID");
#endif

	// メッセに送信
	HWND msnui = NULL;
	msndata.dwData = 0x547;
	msndata.lpData = &buffer;
	msndata.cbData = (lstrlenW(buffer)*2)+2;
	do{
		msnui = FindWindowEx(NULL, msnui, "MsnMsgrUIManager", NULL);
		if(msnui) SendMessage(msnui, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&msndata);
	}while(msnui);
	return;
}

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
    return TRUE;
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	char szPath[MAX_PATH];
	GetModuleFileName(fpi.hDllInst, szPath, MAX_PATH);
	lstrcpy(PathFindFileName(szPath), "Fittle.ini");
	bTagReverse = (BOOL)GetPrivateProfileInt("Main", "TagReverse", 0, szPath);
	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	TAGINFO *pTagInfo = (TAGINFO *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);
	if(!bTagReverse)
		ChangeMSNWindow(pTagInfo->szTitle, pTagInfo->szArtist, pTagInfo->szAlbum, TRUE);
	else
		ChangeMSNWindow(pTagInfo->szArtist, pTagInfo->szTitle, pTagInfo->szAlbum, TRUE);
	return;
}

/* 再生状態が変わる時に呼ばれます */
void OnStatusChange(){
	int n = SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0);
	switch(n){
		case 0: // 停止
			ChangeMSNWindow("Stop", "Stop", "Stop", FALSE);
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

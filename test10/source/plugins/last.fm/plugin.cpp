/*
 * last.fm
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include "plugin.h"
#include "ScrobSubmitter.h"

BOOL OnInit();
void OnQuit();
void OnTrackChange();
void OnStatusChange();
void OnConfig();

#define APPLICATION_ID "fit"

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

ScrobSubmitter submitter;

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	submitter.Init(APPLICATION_ID, NULL, 0);
	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	submitter.Term();
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	// タグを取得
	TAGINFO *taginfo = (TAGINFO *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);
	int length = (int)(int)SendMessage(fpi.hParent, WM_FITTLE, GET_DURATION, 0);
	char *filename = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);

	submitter.Start(taginfo->szArtist, taginfo->szTitle, taginfo->szAlbum, "", length, filename, (ScrobSubmitter::Encoding)0);
}

/* 再生状態が変わる時に呼ばれます */
void OnStatusChange(){
	int nStatus = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0);
	switch(nStatus)
	{
	case 0://停止
		submitter.Stop();
		break;
	case 1://再生
		submitter.Resume();
		break;
	case 3://一時停止
		submitter.Pause();
		break;
	default:
		break;
	}
	return;
}

/* 設定画面を呼び出します（未実装）*/
void OnConfig(){
	return;
}

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
    return TRUE;
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
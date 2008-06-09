/*
 * ranking
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include <time.h>
#include <shlwapi.h>
#include <list>
#include <fstream>

#pragma comment(lib, "shlwapi.lib")

#define MAX_FITTLE_PATH MAX_PATH*2

using namespace std;

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

typedef struct {
	int nCount;
	char szPath[MAX_FITTLE_PATH];
	int nDay;
}ITEM;

#define DEFAULT_SURVIVAL 7

list<ITEM> Array;
char szPLPath[MAX_FITTLE_PATH] = {0};
char szDBPath[MAX_FITTLE_PATH] = {0};
UINT_PTR uTimerId = 0;
int nSurvival;

// M3Uを書き出し
void WritePlaylist(LPCSTR pszPath){
	ofstream ofs(pszPath);

	list<ITEM>::iterator i;

	for(i=Array.begin();i!=Array.end();i++){
		ofs << i->szPath << endl;
	}
	ofs.close();	// デストラクタがあるからいらないだろうけどあると落ちつく
	return;
}

// データベース読み出し
void ReadDatabase(LPCSTR pszPath){
	char szLine[MAX_FITTLE_PATH];
	int nCount;
	int nDay;

	nDay = (int)time(NULL)/60/60;

	ifstream ifs(pszPath);
	ifs.getline(szLine, MAX_FITTLE_PATH);
	nCount = atoi(szLine);
	if(nCount==0) return;
	ITEM item;
	for(int i=0;i<nCount;i++){
		ifs.getline(item.szPath, MAX_FITTLE_PATH);
		ifs.getline(szLine, MAX_FITTLE_PATH);
		item.nCount = atoi(szLine);
		ifs.getline(szLine, MAX_FITTLE_PATH);
		item.nDay = atoi(szLine);
		if(item.nCount==0) break;
		if(nSurvival<0 || nDay-item.nDay<=nSurvival*24){	// 生存時間以上再生がない場合は登録しない
			Array.push_back(item);
		}
	}
	ifs.close();
	return;
}

// データベースを書き出し
void WriteDatabase(LPCSTR pszPath){
	ofstream ofs(pszPath);

	list<ITEM>::iterator i;

	ofs << Array.size() << endl;	// アイテムの数を出力
	for(i=Array.begin();i!=Array.end();i++){
		ofs << i->szPath << endl;
		ofs << i->nCount << endl;
		ofs << i->nDay << endl;
	}
	ofs.close();
	return;
}

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE /*hModule*/, DWORD /*dwFunction*/, LPVOID /*lpNot*/){
    return TRUE;
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	char szINIPath[MAX_FITTLE_PATH] = {0};

	// パスの取得
	GetModuleFileName(fpi.hDllInst, szINIPath, MAX_FITTLE_PATH);
	PathRenameExtension(szINIPath, ".ini");
	lstrcpyn(szDBPath, szINIPath, MAX_FITTLE_PATH);
	PathRenameExtension(szDBPath, ".dat");
	GetPrivateProfileString("Main", "Path", "", szPLPath, MAX_FITTLE_PATH, szINIPath);
	if(!*szPLPath){
		lstrcpyn(szPLPath, szINIPath, MAX_FITTLE_PATH);
		PathRenameExtension(szPLPath, ".m3u");
	}
	nSurvival = GetPrivateProfileInt("Main", "Survival", DEFAULT_SURVIVAL, szINIPath);

	ReadDatabase(szDBPath);

	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	WriteDatabase(szDBPath);
	Array.clear();
	return;
}

VOID CALLBACK TimerProc(HWND /*hWnd*/, UINT /*uMsg*/, UINT /*idEvent*/, DWORD /*dwTime*/){
	char *pszPath;
	ITEM item = {0};
	list<ITEM>::iterator i;

	// パスの取得
	pszPath = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);
	lstrcpy(item.szPath, pszPath);
	item.nCount = 1;
	item.nDay = (int)time(NULL)/60/60;	// 秒->時間

	// 同一パスの検索
	for(i=Array.begin();i!=Array.end();i++){
		if(!lstrcmpi(i->szPath, pszPath)){
			item.nCount = i->nCount+1;
			Array.erase(i);
			break;
		}
	}

	// 挿入位置の検索
	for(i=Array.begin();i!=Array.end();i++){
		if(i->nCount<=item.nCount) break;
	}
	Array.insert(i, item);

	// ファイルに書き込み
	WritePlaylist(szPLPath);
	WriteDatabase(szDBPath);

	// タイマーを削除
	KillTimer(NULL, uTimerId);
	uTimerId = 0;

	// For Debugging
	//MessageBeep(MB_OK);

	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	if(uTimerId) KillTimer(NULL, uTimerId);
	int nLen = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_DURATION, 0);
	uTimerId = SetTimer(NULL, 0, nLen*1000/2, TimerProc);	// 曲の半分でイベントが呼ばれるようにする
	return;
}

/* 再生状態が変わる時に呼ばれます */
void OnStatusChange(){
	if(uTimerId!=0){
		int nStatus = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0);
		if(nStatus==0){
			KillTimer(NULL, uTimerId);
			uTimerId = 0;
		}
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

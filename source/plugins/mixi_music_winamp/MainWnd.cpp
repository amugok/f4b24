/*
 * MainWnd.cpp - mixi_music_winamp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "MainWnd.h"
#include <shlwapi.h>
//#include "resource.h"
#include "Plugin.h"
#include <tchar.h>
#include "GEN.H"
#include "wa_ipc.h"

extern HINSTANCE g_hInst;
extern HWND g_hMainWnd;
extern FITTLE_PLUGIN_INFO fpi;

#define HEADER_SIZE 10

HINSTANCE hDll = NULL;
winampGeneralPurposePlugin *wgpp = NULL;
char szPath[MAX_PATH] = {0};

/* ウィンドウ作成時 */
LRESULT OnCreate(HWND hWnd){
	int nLen = GetModuleFileName(g_hInst, szPath, MAX_PATH);
	szPath[nLen - 3] = 'm';
	szPath[nLen - 2] = 'p';
	szPath[nLen - 1] = '3';

	winampGeneralPurposePluginGetter winampGetGeneralPurposePlugin;

	hDll = LoadLibrary(_T("gen_mixi_for_winamp.dll"));
	if(!hDll) return 0;
	winampGetGeneralPurposePlugin = (winampGeneralPurposePluginGetter)GetProcAddress(hDll, _T("winampGetGeneralPurposePlugin"));

	wgpp = winampGetGeneralPurposePlugin();
	if(wgpp && wgpp->version==GPPHDR_VER){
		wgpp->hDllInstance = hDll;
		wgpp->hwndParent = hWnd;
		wgpp->init();
	}
	return 0;
}

// ホントはSafeIntなんだけどよくわからないので適当にサイズを書く
void WriteSafeInt(LPSTR pszBuff, int nSize){
	for(int i=3;i>=0;i--){
		pszBuff[i] = (char)nSize % 0xff;
		nSize /= 0xff;
	}
	return;
}

// フレームの書き込み
int WriteFrame(LPSTR pszBuff, LPCSTR pszName, LPCSTR pszData){
	int nSize;
	// フレーム名
	lstrcpy(pszBuff, pszName);
	// サイズ(4byte)
	nSize = lstrlen(pszData) + 2;
	WriteSafeInt(pszBuff + 4, nSize);
	// データ(終端文字を含む)
	lstrcpy(pszBuff + HEADER_SIZE + 1, pszData);
	return nSize + HEADER_SIZE;
}

LPSTR MakeDataToStation(LPSTR pszPath){
	TAGINFO *pTag = (TAGINFO *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);

	// ここでダミーのファイルを作成します。
	char szBuff[sizeof(TAGINFO)] = {0};
	HANDLE hFile = CreateFile(pszPath,
		GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return NULL;

	// ヘッダの書き込み
	lstrcat(szBuff, "ID3");
	szBuff[3] = (char)3;	// ID3v2.3にする。

	int nTotal = HEADER_SIZE;

	// フレームの書き込み
	nTotal += WriteFrame(szBuff + nTotal, "TPE1", pTag->szArtist);
	nTotal += WriteFrame(szBuff + nTotal, "TIT2", pTag->szTitle);
	nTotal += WriteFrame(szBuff + nTotal, "TALB", pTag->szAlbum);
	nTotal += WriteFrame(szBuff + nTotal, "TRCK", pTag->szTrack);

	// 全体のサイズを書く
	WriteSafeInt(szBuff + 6, nTotal);

	DWORD dwSize = 0;
	WriteFile(hFile, szBuff, (DWORD)nTotal, &dwSize, NULL);
	CloseHandle(hFile);

	return pszPath;
}

/* ウィンドウプロシージャ */
LRESULT CALLBACK WndProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp){

#ifdef _DEBUG
	// どんなメッセージがやりとりされているかをこれで監視します。
	TCHAR szDebugStr[100];
	wsprintf(szDebugStr, _T("msg=%#04lX wp=%d lp=%d\n"), msg, wp, lp);
	OutputDebugString(szDebugStr);
#endif

	switch (msg) {
		case WM_CREATE:
			return OnCreate(hWnd);

		case WM_WA_IPC:
			switch(lp){
				case IPC_ISPLAYING:
					// 0:停止中 1:再生中 3:一時停止中
					return SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0);

				case IPC_GETOUTPUTTIME:
					int ret;
					if(wp==0){
						// 現在の位置をミリ秒で返す
						ret = 1000*(int)SendMessage(fpi.hParent, WM_FITTLE, GET_POSITION, 0);
					}else if(wp==1){
						// 曲の長さを秒で返す
						ret = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_DURATION, 0);
					}else{
						// エラー
						ret = -1;
					}
					return ret;

				case IPC_GETLISTPOS:
					// ダミーを返す
					return 0;

				case IPC_GETPLAYLISTFILE:
					// Indexの値にかかわらずダミーのファイルを返す。
					return (LRESULT)MakeDataToStation(szPath);

			}
			return 0;

		case WM_DESTROY:
			if(wgpp) wgpp->quit();
			if(hDll) FreeLibrary(hDll);
			PostQuitMessage(0);
			return 0;

	}
	return DefWindowProc(hWnd , msg , wp , lp);
}

/* WinMainの代替関数 */
int WINAPI MyWinMain(){
	WNDCLASSEX wc;
	HWND hWnd;
	MSG msg;
	TCHAR szClassName[] =_T("Finamp v1.x");

	// ウィンドウ・クラスの登録
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc; //プロシージャ名
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInst; //インスタンス
	wc.hIcon = LoadIcon(NULL , IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL , IDI_APPLICATION);
	wc.hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,	0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;	//メニュー名
	wc.lpszClassName = (LPCSTR)szClassName;
	if(!RegisterClassEx(&wc)) return 0;

	// ウィンドウ作成
	hWnd = CreateWindowEx(0,
			szClassName,
			szClassName,			// タイトルバーにこの名前が表示されます
			WS_OVERLAPPEDWINDOW,	// ウィンドウの種類
			CW_USEDEFAULT,			// Ｘ座標
			CW_USEDEFAULT,			// Ｙ座標
			CW_USEDEFAULT,			// 幅
			CW_USEDEFAULT,			// 高さ
			NULL,					// 親ウィンドウのハンドル、親を作るときはNULL
			NULL,					// メニューハンドル、クラスメニューを使うときはNULL
			g_hInst,				// インスタンスハンドル
			NULL);
	if(!hWnd) return 0;

	g_hMainWnd = hWnd;

	UpdateWindow(hWnd);

	while (GetMessage(&msg , NULL , 0 , 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

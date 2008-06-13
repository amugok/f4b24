#include <windows.h>

/* PDKのバージョン */
#define PDK_VER 0

/* 構造体宣言 */
typedef struct{
	int nPDKVer;
	BOOL (*OnInit)();
	void (*OnQuit)();
	void (*OnTrackChange)();
	void (*OnStatusChange)();
	void (*OnConfig)();
	HWND hParent;
	HINSTANCE hDllInst;
} FITTLE_PLUGIN_INFO;

/* メッセージID */
#define WM_FITTLE WM_USER+2

#define GET_TITLE 100
/* タイトルを取得します */
// pszTitle = (const tchar *)SendMessage(hWnd, WM_FITTLE, GET_TITLE, 0);

#define GET_ARTIST 101
/* アーティストを取得します */
// pszArtist = (const char *)SendMessage(hWnd, WM_FITTLE, GET_ARTIST, 0);

#define GET_PLAYING_PATH 102
/* 現在再生中のファイルのフルパスを取得します */
// pszPath = (const char *)SendMessage(hWnd, WM_FITTLE, GET_PLAYING_PATH, 0);

#define GET_PLAYING_INDEX 103
/* 現在再生中の曲のインデックスを取得します */
// nIndex = (int)SendMessage(hWnd, WM_FITTLE, GET_PLAYING_INDEX, 0);

#define GET_STATUS 104
/* 再生状態を取得します */
/*
	0 : 停止中
	1 : 再生中
	2 : ストール中
	3 : 一時停止中
*/
// nStatus = (int)SendMessage(hWnd, WM_FITTLE, GET_STATUS, 0);

#define GET_POSITION 105
/* 現在の位置を秒単位で取得します */
// nPos = (int)SendMessage(hWnd, WM_FITTLE, GET_POSITION, 0);

#define GET_DURATION 106
/* 現在再生中のファイルの長さを秒単位で取得します */
// nLen = (int)SendMessage(hWnd, WM_FITTLE, GET_DURATION, 0);

#define GET_LISTVIEW 107
/* リストビューのハンドルを取得します */
// hList = (HWND)SendMessage(hWnd, WM_FITTLE, GET_LISTVIEW, nIndex);
/*
	nIndex = 左から何番目のタブに関連付けられたリストビューかを指定します
	負の値を指定するとカレントのリストビューのハンドルを返します
*/

#define GET_CONTROL 108
/* コントロールのハンドルを取得します */
// hCtrl = (HWND)SendMessage(hWnd, WM_FITTLE, GET_CONTROL, id);

/* id の値によって戻り値がかわります
	#define ID_COMBO	101
	#define ID_TREE		102
	#define TD_SEEKBAR	104
	#define ID_TOOLBAR	106
	#define ID_VOLUME	107
	#define ID_TAB		108
	#define ID_REBAR	109
	#define ID_STATUS	110
*/

#define GET_CURPATH 109
/* 現在のツリーのパスを取得します */
// pszPath = (const char *)SendMessage(hWnd, WM_FITTLE, GET_CURPATH, 0);

#define GET_MENU 110



/* 気にしないで下さい */
typedef FITTLE_PLUGIN_INFO *(*GetPluginInfoFunc)();

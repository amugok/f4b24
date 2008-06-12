/*
 * Fittle.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef FITTLE_H_
#define FITTLE_H_

#define MAX_FITTLE_PATH 260*2

#define WINVER		0x0400	// 98以降
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4以降

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>

#ifndef GetWindowLongPtr
#define GetWindowLongPtr GetWindowLong
#define SetWindowLongPtr SetWindowLong
#define GWLP_WNDPROC GWL_WNDPROC
#define GWLP_USERDATA GWL_USERDATA
#define GWLP_HINSTANCE GWL_HINSTANCE
#define DWORD_PTR DWORD
#endif
#ifndef MAXLONG_PTR
typedef long LONG_PTR;
#endif

#include "../resource/resource.h"

#include "../../../extra/bass24/bass.h"

// ホイール
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL	0x020A
#endif

// 定数定義
#define ID_COMBO	101
#define ID_TREE		102
#define TD_SEEKBAR	104
#define ID_LIST		105
#define ID_TOOLBAR	106
#define ID_VOLUME	107
#define ID_TAB		108
#define ID_REBAR	109
#define ID_STATUS	110
#define ID_SEEKTIMER		200	// 再生位置表示タイマー
#define ID_TIPTIMER			201	// ツールチップ消去タイマー
#define ID_SCROLLTIMERUP	202	// ドラッグ中自動スクロールタイマー
#define ID_SCROLLTIMERDOWN	203
#define ID_LBTNCLKTIMER		204
#define ID_RBTNCLKTIMER		205
#define ID_MBTNCLKTIMER		206
#define WM_TRAY		(WM_APP + 1)	// タスクトレイのメッセージ
#define SPLITTER_WIDTH	5		// スプリッタの幅
#define HOTKEY_COUNT	8		// ホットキーの数
#define SLIDER_DIVIDED	1000	// シークバー時の分割数
#define IDM_SENDPL_FIRST 50000	// プレイリストに送る動的メニュー
#define IDM_BM_FIRST 60000		// しおり動的メニュー
#define FITTLE_WIDTH 435		// ウィンドウ幅
#define FITTLE_HEIGHT 356		// ウィンドウ高さ
#define MAX_EXT_COUNT 30		// 検索拡張子の数
#define MAX_BM_SIZE 10			// しおりの数
#define BAND_COUNT 3			// リバーのバンドの数

struct FILEINFO{
	char szFilePath[MAX_FITTLE_PATH];	// ファイルパス
	char szSize[50];			// サイズ
	char szTime[50];			// 更新時間
	BOOL bPlayFlag;				// 再生済みチェックフラグ
	struct FILEINFO *pNext;		// 次のリストへのポインタ
};

typedef struct {
	HSTREAM hChan;				// ストリームハンドル
	char szFilePath[MAX_FITTLE_PATH];	// フルパス
	QWORD qStart;				// 再生開始位置
	QWORD qDuration;			// 再生終了位置
	LPBYTE pBuff;				// メモリバッファ
} CHANNELINFO;

struct CONFIG{
	int nTrayOpt;				// タスクトレイモード
	int nInfoTip;				// 曲名お知らせ機能
	int nHighTask;				// システムの優先度
	int nTreeIcon;				// ツリー、コンボのアイコン表示
	int nHideShow;				// 隠しフォルダを表示するか
	int nAllSub;				// 全てのフォルダがサブフォルダを持つ
	int nPathTip;				// ヒントでフルパスを表示
	int nGridLine;				// グリッドラインを表示
	int nSingleExpand;			// フォルダを一つしか開かない
	int nBMRoot;				// しおりをルートとして扱うか
	int nBMFullPath;			// しおりをフルパスで表示
	int nTagReverse;			// タイトル、アーティストを反転
	int nTimeInList;			// プレイリストで更新日時を取得する
	int nHotKey[HOTKEY_COUNT];	// ホットキー
	int nTreeWidth;				// ツリーの幅
	int nExistCheck;
	int nCompact;				// コンパクトモード
	int nFontHeight;			// フォントの高さ
	int nFontStyle;				// フォントのスタイル
	int nTextColor;				// 文字の色
	int nBkColor;				// 背景の色
	int nPlayTxtCol;			// 再生曲文字列
	int nPlayBkCol;				// 再生曲背景
	int nPlayView;				// 再生曲の表示方法
	int nSeekAmount;			// シーク量
	int nVolAmount;
	int nTabMulti;
	int nTabHide;				// タブが一つの時はタブを隠す
	int nShowHeader;			// ヘッダコントロールを表示する
	int nTrayClick[6];			// クリック時の動作
	int nResume;
	int nResPosFlag;
	int nResPos;
	int nTreeState;
	int nShowStatus;
	int nCloseMin;
	int nZipSearch;
	int nTabBottom;
	int nOut32bit;
	int nFadeOut;

	int nMiniPanelEnd;

	char szStartPath[MAX_FITTLE_PATH];	// スタートアップパス
	char szTypeList[MAX_EXT_COUNT][5];		// 検索拡張子
	char szBMPath[MAX_BM_SIZE][MAX_FITTLE_PATH];	// しおり
	char szFontName[32];		// フォントの名前
	char szFilerPath[MAX_FITTLE_PATH];	// ファイラのパス
	char szWAPPath[MAX_FITTLE_PATH];
	char szLastFile[MAX_FITTLE_PATH];
	char szToolPath[MAX_FITTLE_PATH];
};

extern struct CONFIG g_cfg;
extern CHANNELINFO g_cInfo[2];
extern BOOL g_bNow;

LRESULT CALLBACK NewListProc(HWND, UINT, WPARAM, LPARAM); //リストビュー用のプロシージャ
BOOL CheckFileType(char *);

#endif
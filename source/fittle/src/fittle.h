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
#define BAND_COUNT 3			// リバーのバンドの数

struct FILEINFO{
	TCHAR szFilePath[MAX_FITTLE_PATH];	// ファイルパス
	TCHAR szSize[50];			// サイズ
	TCHAR szTime[50];			// 更新時間
	BOOL bPlayFlag;				// 再生済みチェックフラグ
	struct FILEINFO *pNext;		// 次のリストへのポインタ
};

typedef struct {
	HSTREAM hChan;				// ストリームハンドル
	TCHAR szFilePath[MAX_FITTLE_PATH];	// フルパス
	QWORD qStart;				// 再生開始位置
	QWORD qDuration;			// 再生終了位置
	LPBYTE pBuff;				// メモリバッファ
} CHANNELINFO;

struct CONFIG{
	int nBkColor;				// 背景の色
	int nTextColor;				// 文字の色
	int nPlayTxtCol;			// 再生曲文字列
	int nPlayBkCol;				// 再生曲背景
	int nPlayView;				// 再生曲の表示方法
	int nHighTask;				// システムの優先度
	int nGridLine;				// グリッドラインを表示
	int nSingleExpand;			// フォルダを一つしか開かない
	int nExistCheck;			// 存在確認
	int nTimeInList;			// プレイリストで更新日時を取得する
	int nTreeIcon;				// ツリー、コンボのアイコン表示
	int nTrayOpt;				// タスクトレイモード
	int nHideShow;				// 隠しフォルダを表示するか
	int nTabBottom;				// タブを下に表示する
	int nTabMulti;				// 多段で表示する
	int nAllSub;				// 全てのフォルダがサブフォルダを持つ
	int nPathTip;				// ヒントでフルパスを表示
	int nInfoTip;				// 曲名お知らせ機能
	int nTagReverse;			// タイトル、アーティストを反転
	int nShowHeader;			// ヘッダコントロールを表示する
	int nSeekAmount;			// シーク量
	int nVolAmount;				// 音量変化量(隠し設定?)
	int nResume;				// 終了時に再生していた曲を起動時にも再生する
	int nResPosFlag;			// 終了時の再生位置も記録復元する
	int nCloseMin;				// 閉じるボタンで最小化する
	int nZipSearch;				// サブフォルダを検索で圧縮ファイルも検索する
	int nTabHide;				// タブが一つの時はタブを隠す
	int nOut32bit;				// 32bit(float)で出力する
	int nFadeOut;				// 停止時にフェードアウトする

	TCHAR szStartPath[MAX_FITTLE_PATH];	// スタートアップパス
	TCHAR szFilerPath[MAX_FITTLE_PATH];	// ファイラのパス

	int nHotKey[HOTKEY_COUNT];	// ホットキー

	int nTrayClick[6];			// クリック時の動作

	TCHAR szFontName[32];		// フォントの名前
	int nFontHeight;			// フォントの高さ
	int nFontStyle;				// フォントのスタイル

	TCHAR szToolPath[MAX_FITTLE_PATH];

	/* 状態 */

	int nTreeWidth;				// ツリーの幅
	int nCompact;				// コンパクトモード
	int nResPos;
	int nTreeState;
	int nShowStatus;

	int nMiniPanelEnd;

	TCHAR szLastFile[MAX_FITTLE_PATH];
};

extern struct CONFIG g_cfg;
extern CHANNELINFO g_cInfo[2];
extern BOOL g_bNow;

LRESULT CALLBACK NewListProc(HWND, UINT, WPARAM, LPARAM); //リストビュー用のプロシージャ
BOOL CheckFileType(LPTSTR);

#endif
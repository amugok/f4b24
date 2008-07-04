
/*
 * Fittle.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "fittle.h"
#include "func.h"
#include "tree.h"
#include "list.h"
#include "listtab.h"
#include "finddlg.h"
#include "bass_tag.h"
#include "archive.h"
#include "mt19937ar.h"
#include "plugins.h"
#include "plugin.h"
#include "f4b24.h"
#include "oplugin.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// ライブラリをリンク
#if defined(_MSC_VER)
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "../../extra/bass24/bass.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker, "/OPT:NOWIN98")
#endif

// ソフト名（バージョンアップ時に忘れずに更新）
#define FITTLE_VERSION TEXT("Fittle Ver.2.2.2 Preview 3")
#ifdef UNICODE
#define F4B24_VERSION_STRING TEXT("test28u")
#else
#define F4B24_VERSION_STRING TEXT("test28")
#endif
#define F4B24_VERSION 28
#define F4B24_IF_VERSION 28
#ifndef _DEBUG
#define FITTLE_TITLE FITTLE_VERSION TEXT(" for BASS 2.4 ") F4B24_VERSION_STRING
#else
#define FITTLE_TITLE FITTLE_VERSION TEXT(" for BASS 2.4 ") F4B24_VERSION_STRING TEXT(" <Debug>")
#endif


//--マクロ--
// 全ての選択状態を解除後、指定インデックスのアイテムを選択、表示
#define ListView_SingleSelect(hLV, nIndex) \
	ListView_SetItemState(hLV, -1, 0, (LVIS_SELECTED | LVIS_FOCUSED)); \
	ListView_SetItemState(hLV, nIndex, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED)); \
	ListView_EnsureVisible(hLV, nIndex, TRUE)

// ウィンドウをサブクラス化、プロシージャハンドルをウィンドウに関連付ける
#define SET_SUBCLASS(hWnd, Proc) \
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Proc))

// 再生中のリストタブのポインタを取得
#define GetPlayListTab(hTab) \
	GetListTab(hTab, (m_nPlayTab!=-1 ? m_nPlayTab : TabCtrl_GetCurSel(hTab)))

#define ODS(X) \
	OutputDebugString(X); OutputDebugString(TEXT("\n"));

enum {
	WM_F4B24_INTERNAL_SETUP_NEXT = 1,
	WM_F4B24_INTERNAL_PLAY_NEXT = 2
};
// --列挙型の宣言--
enum {PM_LIST=0, PM_RANDOM, PM_SINGLE};	// プレイモード
enum {GS_NOEND=0, GS_OK, GS_FAILED, GS_NEWFREQ};	// EventSyncの戻り値みたいな
enum {TREE_UNSHOWN=0, TREE_SHOW, TREE_HIDE};	// ツリーの表示状態

//--関数のプロトタイプ宣言--
// プロシージャ
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK MyHookProc(int, WPARAM, LPARAM);
static LRESULT CALLBACK NewSliderProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewTabProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewTreeProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewSplitBarProc(HWND, UINT, WPARAM, LPARAM);
static void CALLBACK EventSync(HSYNC, DWORD, DWORD, void *);
// 設定関係
static void LoadState();
static void LoadConfig();
static void SaveState(HWND);
static void ApplyConfig(HWND hWnd);
static void SendSupportList(HWND hWnd);
// コントロール関係
static void DoTrayClickAction(HWND, int);
static void PopupTrayMenu(HWND);
static void PopupPlayModeMenu(HWND, NMTOOLBAR *);
static void ToggleWindowView(HWND);
static HWND CreateToolBar(HWND);

static int ShowToolTip(HWND, HWND, LPTSTR);
static int ControlPlayMode(HMENU, int);
static BOOL ToggleRepeatMode(HMENU);
static int SetTaskTray(HWND);
static int RegHotKey(HWND);
static int UnRegHotKey(HWND);
static void OnBeginDragList(HWND, POINT);
static void OnDragList(HWND, POINT);
static void OnEndDragList(HWND);
static void DrawTabFocus(HWND, int, BOOL);
static BOOL SetUIFont(void);
static void UpdateWindowSize(HWND);
static BOOL SetUIColor(void);
static void SetStatusbarIcon(LPTSTR, BOOL);
static void ShowSettingDialog(HWND, int);
static LPTSTR MallocAndConcatPath(LISTTAB *);
static void MyShellExecute(HWND, LPTSTR, LPTSTR, BOOL);
static void InitFileTypes();
static int SaveFileDialog(LPTSTR, LPTSTR);
static LONG GetToolbarTrueWidth(HWND);
static int GetMenuPosFromString(HMENU hMenu, LPTSTR lpszText);
// 演奏制御関係
static BOOL SetChannelInfo(BOOL, struct FILEINFO *);
static BOOL _BASS_ChannelSetPosition(DWORD, int);
static void _BASS_ChannelSeekSecond(DWORD, float, int);
static BOOL PlayByUser(HWND, struct FILEINFO *);
static void OnChangeTrack();
static int FilePause();
static int StopOutputStream(HWND);
static struct FILEINFO *SelectNextFile(BOOL);
static void FreeChannelInfo(BOOL bFlag);

static void RemoveFiles(HWND hWnd);

// メンバ変数
static CRITICAL_SECTION m_cs;

// メンバ変数（状態編）
static int m_nPlayTab = -1;			// 再生中のタブインデックス
static int m_nGaplessState = GS_OK;	// ギャップレス再生用のステータス
static int m_nPlayMode = PM_LIST;	// プレイモード
static HINSTANCE m_hInst = NULL;	// インスタンス
static NOTIFYICONDATA m_ni;	// タスクトレイのアイコンのデータ
static BOOL m_nRepeatFlag = FALSE;	// リピート中かどうか
static BOOL m_bTrayFlag = FALSE;	// タスクトレイに入ってるかどうか
static TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス
static TCHAR m_szTime[100];			// 再生時間
static TCHAR m_szTag[MAX_FITTLE_PATH];	// タグ
static TCHAR m_szTreePath[MAX_FITTLE_PATH];	// ツリーのパス
static BOOL m_bFloat = FALSE;

static TAGINFO m_taginfo = {0};
#ifdef UNICODE
typedef struct{
	CHAR szTitle[256];
	CHAR szArtist[256];
	CHAR szAlbum[256];
	CHAR szTrack[10];
}TAGINFOA;
static TAGINFOA m_taginfoA;
static CHAR m_szTreePathA[MAX_FITTLE_PATH];	// ツリーのパス
static CHAR m_szFilePathA[MAX_FITTLE_PATH];	// ツリーのパス
#endif
static volatile BOOL m_bCueEnd = FALSE;
// メンバ変数（ハンドル編）
static HWND m_hCombo = NULL;		// コンボボックスハンドル
static HWND m_hTree = NULL;			// ツリービューハンドル
static HWND m_hTab = NULL;			// タブコントロールハンドル
static HWND m_hToolBar = NULL;		// ツールバーハンドル
static HWND m_hStatus = NULL;		// ステータスバーハンドル
static HWND m_hSeek = NULL;			// シークバーハンドル
static HWND m_hVolume = NULL;		// ボリュームバーハンドル
static HWND m_hTitleTip = NULL;		// タイトル用ツールチップハンドル
static HWND m_hSliderTip = NULL;	// スライダ用ツールチップハンドル
static HMENU m_hTrayMenu = NULL;	// トレイメニューハンドル
static HIMAGELIST m_hDrgImg = NULL;	// ドラッグイメージ
static HHOOK m_hHook = NULL;		// ローカルフックハンドル
static HFONT m_hBoldFont = NULL;	// 太文字フォントハンドル
static HFONT m_hFont = NULL;		// フォントハンドル
static HTREEITEM m_hHitTree = NULL;	// ツリーのヒットアイテム
static HMENU m_hMainMenu = NULL;	// メインメニューハンドル

static OUTPUT_PLUGIN_INFO *m_pOutputPlugin = NULL;

// 起動引数
static int XARGC = 0;
static LPTSTR *XARGV = 0;

// グローバル変数
struct CONFIG g_cfg = {0};			// 設定構造体
CHANNELINFO g_cInfo[2] = {0};		// チャンネル情報
BOOL g_bNow = 0;				// アクティブなチャンネル0 or 1
struct FILEINFO *g_pNextFile = NULL;	// 再生曲

typedef struct StringList {
	struct StringList *pNext;
	TCHAR szString[1];
} STRING_LIST, *LPSTRING_LIST;

static LPSTRING_LIST lpTypelist = NULL;

/* 出力プラグインリスト */
static struct OUTPUT_PLUGIN_NODE {
	struct OUTPUT_PLUGIN_NODE *pNext;
	OUTPUT_PLUGIN_INFO *pInfo;
	HMODULE hDll;
} *pOutputPluginList = NULL;

/* 出力プラグインを開放 */
static void FreeOutputPlugins(){
	struct OUTPUT_PLUGIN_NODE *pList = pOutputPluginList;
	while (pList) {
		struct OUTPUT_PLUGIN_NODE *pNext = pList->pNext;
		FreeLibrary(pList->hDll);
		HFree(pList);
		pList = pNext;
	}
	pOutputPluginList = NULL;
}

/* 出力プラグインを登録 */
static BOOL CALLBACK RegisterOutputPlugin(HMODULE hPlugin, HWND hWnd){
	FARPROC lpfnProc = GetProcAddress(hPlugin, "GetOPluginInfo");
	if (lpfnProc){
		struct OUTPUT_PLUGIN_NODE *pNewNode = (struct OUTPUT_PLUGIN_NODE *)HAlloc(sizeof(struct OUTPUT_PLUGIN_NODE));
		if (pNewNode) {
			pNewNode->pInfo = ((GetOPluginInfoFunc)lpfnProc)();
			pNewNode->pNext = NULL;
			pNewNode->hDll = hPlugin;
			if (pNewNode->pInfo){
				if (pOutputPluginList) {
					struct OUTPUT_PLUGIN_NODE *pList;
					for (pList = pOutputPluginList; pList->pNext; pList = pList->pNext);
					pList->pNext = pNewNode;
				} else {
					pOutputPluginList = pNewNode;
				}
				return TRUE;
			}
			HFree(pNewNode);
		}
	}
	return FALSE;
}

static BOOL CALLBACK OPCBIsEndCue(void){
	BOOL fRet = m_bCueEnd;
	return fRet;
}

static void CALLBACK OPCBPlayNext(HWND hWnd){
	m_bCueEnd = FALSE;
	PostMessage(hWnd, WM_F4B24_IPC, WM_F4B24_INTERNAL_PLAY_NEXT, 0);
}

static DWORD CALLBACK OPCBGetDecodeChannel(float *pGain) {
	CHANNELINFO *pCh = &g_cInfo[g_bNow];
	*pGain = pCh->sAmp;
	return pCh->hChan;
}

static int OPInit(OUTPUT_PLUGIN_INFO *pPlugin, DWORD dwId, HWND hWnd){
	m_pOutputPlugin = pPlugin;
	m_pOutputPlugin->hWnd = hWnd;
	m_pOutputPlugin->IsEndCue = OPCBIsEndCue;
	m_pOutputPlugin->PlayNext = OPCBPlayNext;
	m_pOutputPlugin->GetDecodeChannel = OPCBGetDecodeChannel;
	return m_pOutputPlugin->Init(dwId); 
}

static int InitOutputPlugin(HWND hWnd){
	EnumPlugins(NULL, TEXT(""), TEXT("*.fop"), RegisterOutputPlugin, hWnd);

	if (pOutputPluginList){
		struct OUTPUT_PLUGIN_NODE *pList = pOutputPluginList;
		DWORD dwDefaultDevice = 0xffffffff;
		OUTPUT_PLUGIN_INFO *pDefaultPlugin = 0;
		do {
			int i;
			OUTPUT_PLUGIN_INFO *pPlugin = pList->pInfo;
			DWORD dwDefaultId = pPlugin->GetDefaultDeviceID();
			if (dwDefaultDevice > dwDefaultId)
			{
				dwDefaultDevice = dwDefaultId;
				pDefaultPlugin = pPlugin;
			}
			int n = pPlugin->GetDeviceCount();
			for (i = 0; i < n; i++) {
				int nId = pPlugin->GetDeviceID(i);
				if (g_cfg.nOutputDevice != 0 && nId == g_cfg.nOutputDevice) {
					return OPInit(pPlugin, nId, hWnd); 
				}
			}
			pList = pList->pNext;
		} while (pList);
		if (pDefaultPlugin) {
			return OPInit(pDefaultPlugin, dwDefaultDevice, hWnd); 
		}
	}
	return 0;
}

static void OPTerm(){
	if (m_pOutputPlugin) m_pOutputPlugin->Term();
}

static int OPGetRate(){
/*
	if (m_pOutputPlugin) return m_pOutputPlugin->GetRate();
*/
	return 44100;
}

static BOOL OPSetup(HWND hWnd){
	if (m_pOutputPlugin) return m_pOutputPlugin->Setup(hWnd);
	return FALSE;
}

static int OPGetStatus(){
	return m_pOutputPlugin ? m_pOutputPlugin->GetStatus() : OUTPUT_PLUGIN_STATUS_STOP;
}

static float CalcBassVolume(DWORD dwVol){
	float fVol = g_cfg.nReplayGainPostAmp * g_cInfo[g_bNow].sGain * dwVol / (float)(SLIDER_DIVIDED * 100);
	switch (g_cfg.nReplayGainMixer) {
	case 0:
		/*  内蔵 */
		g_cInfo[g_bNow].sAmp = (fVol == (float)1) ? 0 : fVol;
		fVol = 1;
		break;
	case 1:
		/*  増幅のみ内蔵 */
		if (fVol > 1){
			g_cInfo[g_bNow].sAmp = fVol;
			fVol = 1;
		}else{
			g_cInfo[g_bNow].sAmp = 0;
		}
		break;
	case 2:
		/*  BASS */
		g_cInfo[g_bNow].sAmp = 0;
		if (fVol > 1) fVol = 1;
		break;
	}
	return fVol;
}

static void OPStart(BASS_CHANNELINFO *info, DWORD dwVol, BOOL fFloat){
	if (m_pOutputPlugin) m_pOutputPlugin->Start(info, CalcBassVolume(dwVol), fFloat);
}

static void OPEnd(){
	if (m_pOutputPlugin) m_pOutputPlugin->End();
}

static void OPPlay(){
	if (m_pOutputPlugin) m_pOutputPlugin->Play();
}

static void OPPause(){
	if (m_pOutputPlugin) m_pOutputPlugin->Pause();
}

static void OPStop(){
	if (m_pOutputPlugin) m_pOutputPlugin->Stop();
}
static void OPSetVolume(DWORD dwVol){
	float sVolume = CalcBassVolume(dwVol);
	if (m_pOutputPlugin) m_pOutputPlugin->SetVolume(sVolume);
}

static void OPSetFadeIn(DWORD dwVol, DWORD dwTime){
	if(g_cfg.nFadeOut){
		float sVolume = CalcBassVolume(dwVol);
		if (m_pOutputPlugin) m_pOutputPlugin->FadeIn(sVolume, dwTime);
	}
	OPSetVolume(dwVol);
}

static void OPSetFadeOut(DWORD dwTime){
	if(g_cfg.nFadeOut){
		if (m_pOutputPlugin) m_pOutputPlugin->FadeOut(dwTime);
	}
}

static BOOL OPIsSupportFloatOutput(){
	if (m_pOutputPlugin) return m_pOutputPlugin->IsSupportFloatOutput();
	return FALSE;
}

static void StringListFree(LPSTRING_LIST *pList){
	LPSTRING_LIST pCur = *pList;
	while (pCur){
		LPSTRING_LIST pNext = pCur->pNext;
		HFree(pCur);
		pCur = pNext;
	}
	*pList = NULL;
}

static  LPSTRING_LIST StringListWalk(LPSTRING_LIST *pList, int nIndex){
	LPSTRING_LIST pCur = *pList;
	int i = 0;
	while (pCur){
		if (i++ == nIndex) return pCur;
		pCur = pCur->pNext;
	}
	return NULL;
}

static  LPSTRING_LIST StringListFindI(LPSTRING_LIST *pList, LPCTSTR szValue){
	LPSTRING_LIST pCur = *pList;
	while (pCur){
		if (lstrcmpi(pCur->szString, szValue) == 0) return pCur;
		pCur = pCur->pNext;
	}
	return NULL;
}

static int StringListAdd(LPSTRING_LIST *pList, LPTSTR szValue){
	int i = 0;
	LPSTRING_LIST pCur = *pList;
	LPSTRING_LIST pNew = (LPSTRING_LIST)HAlloc(sizeof(STRING_LIST) + sizeof(TCHAR) * lstrlen(szValue));
	if (!pNew) return -1;
	pNew->pNext = NULL;
	lstrcpy(pNew->szString, szValue);
	if (pCur){
		i++;
		/* 末尾に追加 */
		while (pCur->pNext){
			pCur = pCur->pNext;
			i++;
		}
		pCur->pNext = pNew;
	} else {
		/* 先頭 */
		*pList = pNew;
	}
	return i;
}

static void ClearTypelist(){
	StringListFree(&lpTypelist);
}

static int AddTypelist(LPTSTR szExt){
	if (!szExt || !*szExt) return TRUE;
	if (StringListFindI(&lpTypelist, szExt)) return TRUE;
	return StringListAdd(&lpTypelist, szExt);
}

static LPTSTR GetTypelist(int nIndex){
	static TCHAR szNull[1] = {0};
	LPSTRING_LIST lpbm = StringListWalk(&lpTypelist, nIndex);
	return lpbm ? lpbm->szString : szNull;
}

static void lstrcpyntA(LPSTR lpDst, LPCTSTR lpSrc, int nDstMax){
#if defined(UNICODE)
	WideCharToMultiByte(CP_ACP, 0, lpSrc, -1, lpDst, nDstMax, NULL, NULL);
#else
	lstrcpyn(lpDst, lpSrc, nDstMax);
#endif
}

static void lstrcpyntW(LPWSTR lpDst, LPCTSTR lpSrc, int nDstMax){
#if defined(UNICODE)
	lstrcpyn(lpDst, lpSrc, nDstMax);
#else
	MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, lpDst, nDstMax); //S-JIS->Unicode
#endif
}

static void lstrcpynAt(LPTSTR lpDst, LPCSTR lpSrc, int nDstMax){
#if defined(UNICODE)
	MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, lpDst, nDstMax); //S-JIS->Unicode
#else
	lstrcpyn(lpDst, lpSrc, nDstMax);
#endif
}

static void lstrcpynWt(LPTSTR lpDst, LPCWSTR lpSrc, int nDstMax){
#if defined(UNICODE)
	lstrcpyn(lpDst, lpSrc, nDstMax);
#else
	WideCharToMultiByte(CP_ACP, 0, lpSrc, -1, lpDst, nDstMax, NULL, NULL);
#endif
}

/* コマンドラインパラメータの展開 */
static HMODULE ExpandArgs(int *pARGC, LPTSTR **pARGV){
#ifdef UNICODE
	*pARGC = 1;
	*pARGV = CommandLineToArgvW(GetCommandLine(), pARGC);
	return NULL;
#elif defined(_MSC_VER)
	*pARGC = __argc;
	*pARGV = __argv;
	return NULL;
#else
	/* Visual C++以外の場合MSVCRT.DLLに引数を解析させる */
	typedef struct { int newmode; } GMASTARTUPINFO;
	typedef void (__cdecl *LPFNGETMAINARGS) (int *pargc, char ***pargv, char ***penvp, int dowildcard, GMASTARTUPINFO * startinfo);
	HMODULE h = LoadLibrary("MSVCRT.DLL");
	*pARGC = 1;
	if (h){
		LPFNGETMAINARGS pfngma = (LPFNGETMAINARGS)GetProcAddress(h, "__getmainargs");
		char **xenvp;
		GMASTARTUPINFO si = {0};
		pfngma(pARGC, pARGV, &xenvp, 1, &si);
		*pARGC = *pARGC;
		*pARGV = *pARGV;
	}
	return h;
#endif
}

#ifndef _DEBUG
/*  既に実行中のFittleにパラメータを送信する */
static void SendCopyData(HWND hFittle, int iType, LPTSTR lpszString){
#ifdef UNICODE
	CHAR nameA[MAX_FITTLE_PATH];
	LPBYTE lpWork;
	COPYDATASTRUCT cds;
	DWORD la, lw, cbData;
	WideCharToMultiByte(CP_ACP, 0, lpszString, -1, nameA, MAX_FITTLE_PATH, NULL, NULL);
	la = lstrlenA(nameA) + 1;
	if (la & 1) la++; /* WORD align */
	lw = lstrlenW(lpszString) + 1;
	cbData = la * sizeof(CHAR) + lw * sizeof(WCHAR);
	lpWork = (LPBYTE)HZAlloc(cbData);
	if (!lpWork) return;
	lstrcpyA((LPSTR)(lpWork), nameA);
	lstrcpyW((LPWSTR)(lpWork + la), lpszString);
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpWork;
	cds.cbData = cbData;
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
	HFree(lpWork);
#else
	COPYDATASTRUCT cds;
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpszString;
	cds.cbData = (lstrlenA(lpszString) + 1) * sizeof(CHAR);
	// 文字列送信
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
#endif
}

// 多重起動の防止
static LRESULT CheckMultiInstance(BOOL *pbMulti){
	HANDLE hMutex;
	HWND hFittle;
	int i;

	*pbMulti = FALSE;
	hMutex = CreateMutex(NULL, TRUE, TEXT("Mutex of Fittle"));

	if(GetLastError()==ERROR_ALREADY_EXISTS){
		*pbMulti = TRUE;

		hFittle = FindWindow(TEXT("Fittle"), NULL);
		// コマンドラインがあれば本体に送信
		if(XARGC>1){
			// コマンドラインオプション
			if(XARGV[1][0] == '/'){
				if(!lstrcmpi(XARGV[1], TEXT("/play"))){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
				}else if(!lstrcmpi(XARGV[1], TEXT("/pause"))){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_PAUSE, 0), 0);
				}else if(!lstrcmpi(XARGV[1], TEXT("/stop"))){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_STOP, 0), 0);
				}else if(!lstrcmpi(XARGV[1], TEXT("/prev"))){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_PREV, 0), 0);
				}else if(!lstrcmpi(XARGV[1], TEXT("/next"))){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
				}else if(!lstrcmpi(XARGV[1], TEXT("/exit"))){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_END, 0), 0);
				}else if(!lstrcmpi(XARGV[1], TEXT("/add"))){
					for(i=2;i<XARGC;i++){
						SendCopyData(hFittle, 1, XARGV[i]);
					}
					if(XARGC>2) return 0;
				}else if(!lstrcmpi(XARGV[1], TEXT("/addplay"))){
					for(i=2;i<XARGC;i++){
						SendCopyData(hFittle, 1, XARGV[i]);
					}
					SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
					return 0;
				}
			}else{
				SendCopyData(hFittle, 0, XARGV[1]);
				return 0;
			}
		}
		// ミニパネルがあったら終わり
		if(SendMessage(hFittle, WM_USER, 0, 0))	return 0;

		if(!IsWindowVisible(hFittle) || IsIconic(hFittle)){
			SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_TRAY_WINVIEW, 0), 0);
		}	
		SetForegroundWindow(hFittle);
		return 0;
	}
	return 0;
}
#endif

static HWND Initialze(HINSTANCE hCurInst, int nCmdShow){
	HWND hWnd;
	WNDCLASSEX wc;
	WINDOWPLACEMENT wpl = {0};
	TCHAR szClassName[] = TEXT("Fittle");	// ウィンドウクラス

	DWORD dTime = GetTickCount();
	TCHAR szBuff[100];

	// BASSのバージョン確認
	if(HIWORD(BASS_GetVersion())!=BASSVERSION){
		MessageBox(NULL, TEXT("bass.dllのバージョンを確認してください。"), TEXT("Fittle"), MB_OK);
		return 0;
	}

	// ウィンドウ・クラスの登録
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_DBLCLKS; //CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc; //プロシージャ名
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInst = hCurInst; //インスタンス
	wc.hIcon = (HICON)LoadImage(hCurInst, TEXT("MYICON"), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hIconSm = (HICON)LoadImage(hCurInst, TEXT("MYICON"), IMAGE_ICON, 16, 16, LR_SHARED);
	wc.hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,	0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = TEXT("MAINMENU");	//メニュー名
	wc.lpszClassName = (LPCTSTR)szClassName;
	if(!RegisterClassEx(&wc)) return 0;

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("Fittle.ini"));

	LoadState();
	LoadConfig();

	// ウィンドウ作成
	hWnd = CreateWindow(szClassName,
			szClassName,			// タイトルバーにこの名前が表示されます
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,	// ウィンドウの種類
			CW_USEDEFAULT,			// Ｘ座標
			CW_USEDEFAULT,			// Ｙ座標
			CW_USEDEFAULT,			// 幅
			CW_USEDEFAULT,			// 高さ
			NULL,					// 親ウィンドウのハンドル、親を作るときはNULL
			NULL,					// メニューハンドル、クラスメニューを使うときはNULL
			hCurInst,				// インスタンスハンドル
			NULL);
	if(!hWnd) return 0;

	// 最大化を再現
	if(nCmdShow==SW_SHOWNORMAL && GetPrivateProfileInt(TEXT("Main"), TEXT("Maximized"), 0, m_szINIPath)){
		nCmdShow = SW_SHOWMAXIMIZED;			// 最大化状態
		//wpl.flags = WPF_RESTORETOMAXIMIZED;		// 最小化された最大化状態
	}

	// ウィンドウの位置を設定
	UpdateWindow(hWnd);
	wpl.length = sizeof(WINDOWPLACEMENT);
	wpl.showCmd = SW_HIDE;
	wpl.rcNormalPosition.left = (int)GetPrivateProfileInt(TEXT("Main"), TEXT("Left"), (GetSystemMetrics(SM_CXSCREEN)-FITTLE_WIDTH)/2, m_szINIPath);
	wpl.rcNormalPosition.top = (int)GetPrivateProfileInt(TEXT("Main"), TEXT("Top"), (GetSystemMetrics(SM_CYSCREEN)-FITTLE_HEIGHT)/2, m_szINIPath);
	wpl.rcNormalPosition.right = wpl.rcNormalPosition.left + (int)GetPrivateProfileInt(TEXT("Main"), TEXT("Width"), FITTLE_WIDTH, m_szINIPath);
	wpl.rcNormalPosition.bottom = wpl.rcNormalPosition.top + (int)GetPrivateProfileInt(TEXT("Main"), TEXT("Height"), FITTLE_HEIGHT, m_szINIPath);
	SetWindowPlacement(hWnd, &wpl);

	// ミニパネルの復元
	if(g_cfg.nMiniPanelEnd){
		if(nCmdShow!=SW_SHOWNORMAL)
			ShowWindow(hWnd, nCmdShow);	// 最大化等のウィンドウ状態を適応
		PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_MINIPANEL, 0), 0);
	}else{
		ShowWindow(hWnd, nCmdShow);	// 表示
	}

	GetWindowText(m_hStatus, szBuff, 100);
	if(lstrlen(szBuff)==0){
		wsprintf(szBuff, TEXT("Startup time: %d ms\n"), GetTickCount() - dTime);
		SetWindowText(m_hStatus, szBuff);
	}
	return hWnd;
}

int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE /*hPrevInst*/, LPSTR /*lpsCmdLine*/, int nCmdShow){
	HWND hWnd;
	MSG msg;
	HACCEL hAccel;
	HMODULE XARGD = ExpandArgs(&XARGC, &XARGV);

#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifndef _DEBUG
	BOOL bMulti = FALSE;
	LRESULT lRet = CheckMultiInstance(&bMulti);
	// 多重起動の防止
	if (bMulti) return lRet;
#endif

	// 初期化
	hWnd = Initialze(hCurInst, nCmdShow);
	if (!hWnd) return 0;

	// アクセラレーターキーを取得
	hAccel = LoadAccelerators(hCurInst, TEXT("MYACCEL"));

	// メッセージループ
	while(GetMessage(&msg, NULL, 0, 0) > 0){
		if(!TranslateAccelerator(hWnd, hAccel, &msg)){	// 通常よりアクセラレータキーの優先度を高くしてます
			HWND m_hMiniPanel = (HWND)SendMessage(hWnd, WM_FITTLE, GET_MINIPANEL, 0);
			if(!m_hMiniPanel || !IsDialogMessage(m_hMiniPanel, &msg)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

#ifdef UNICODE
	if (XARGV) GlobalFree(XARGV);
#elif defined(_MSC_VER)
#else
	if (XARGD) FreeLibrary(XARGD);
#endif
	ClearTypelist();
	FreeOutputPlugins();

	return (int)msg.wParam;
}

#if 1 
#define TIMESTART
#define TIMECHECK(m) 
#else
#define TIMESTART \
	DWORD dStartTime; \
	dStartTime = GetTickCount();

#define TIMECHECK(m) { \
	TCHAR buf[128]; \
	wsprintf(buf, TEXT(m) TEXT(": %d ms\n"), GetTickCount() - dStartTime); \
	OutputDebugString(buf); \
}
#endif

#ifdef UNICODE
/* サブクラス化による文字化け対策 */
static WNDPROC mUniWndProcOld = 0;
static LRESULT CALLBACK UniWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	/* ANSI変換を抑止する */
	if (msg == WM_SETTEXT || msg == WM_GETTEXT)
		return DefWindowProcW(hWnd, msg, wp, lp);
	return CallWindowProc(mUniWndProcOld, hWnd, msg, wp, lp);
}
static void UniFix(HWND hWnd){
	//if (!IsWindowUnicode(hWnd))
	mUniWndProcOld = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)UniWndProc);
}
#endif


// ウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	static HWND s_hRebar = NULL;			// レバーハンドル
	static HWND s_hSplitter = NULL;			// スプリッタハンドル
	static int s_nHitTab = -1;				// タブのヒットアイテム
	static UINT s_uTaskbarRestart;			// タスクトレイ再描画メッセージのＩＤ
	static int s_nClickState = 0;			// タスクトレイアイコンのクリック状態
	static BOOL s_bReviewAllow = TRUE;		// ツリーのセル変化で検索を許可するか

	int i;
	int nFileIndex;
	TCHAR szLastPath[MAX_FITTLE_PATH] = {0};
	TCHAR szCmdParPath[MAX_FITTLE_PATH] = {0};
	TCHAR szSec[10] = TEXT("");
	MENUITEMINFO mii;

	switch(msg)
	{
		case WM_USER:
			return (LRESULT)(HWND)SendMessage(hWnd, WM_FITTLE, GET_MINIPANEL, 0);

		case WM_USER+1:
			OnChangeTrack();
			break;

		case WM_CREATE:

			TIMESTART

			i = InitOutputPlugin(hWnd);

			TIMECHECK("出力プラグイン初期化")

			// BASS初期化
			if(!BASS_Init(i, OPGetRate(), 0, hWnd, NULL)){
				MessageBox(hWnd, TEXT("BASSの初期化に失敗しました。"), TEXT("Fittle"), MB_OK);
				BASS_Free();
				ExitProcess(1);
			}

			TIMECHECK("BASS初期化")

			m_bFloat = (g_cfg.nOut32bit && OPIsSupportFloatOutput()) ? TRUE : FALSE;
			g_cInfo[0].sGain = 1;
			g_cInfo[1].sGain = 1;

			TIMECHECK("出力形式確認")

			// 優先度
			if(g_cfg.nHighTask){
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
			}

			TIMECHECK("優先度設定")

			GetModuleParentDir(szLastPath);

			TIMECHECK("EXEパスの取得")

			// 書庫プラグイン初期化
			InitArchive(hWnd);
			TIMECHECK("書庫プラグイン初期化")

			// 検索拡張子の決定
			InitFileTypes();
			TIMECHECK("検索拡張子の決定")

			// タスクトレイ再描画のメッセージを保存
			s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));

			TIMECHECK("タスクトレイ再描画のメッセージを保存")

			// 現在のプロセスIDで乱数ジェネレータを初期化
			init_genrand((unsigned int)(GetTickCount()+GetCurrentProcessId()));

			TIMECHECK("RNG初期化")

			// メニューハンドルを保存
			m_hMainMenu = GetMenu(hWnd);

			TIMECHECK("メニューハンドルを保存")

			// コントロールを作成
			INITCOMMONCONTROLSEX ic;
			ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
			ic.dwICC = ICC_USEREX_CLASSES | ICC_TREEVIEW_CLASSES
					 | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES
					 | ICC_COOL_CLASSES | ICC_TAB_CLASSES;
			InitCommonControlsEx(&ic);

			TIMECHECK("InitCommonControlsEx")

			// ステータスバー作成
			m_hStatus = CreateStatusWindow(WS_CHILD | /*WS_VISIBLE |*/ SBARS_SIZEGRIP | CCS_BOTTOM | SBT_TOOLTIPS, TEXT(""), hWnd, ID_STATUS);
			if(g_cfg.nShowStatus){
				g_cfg.nShowStatus = 0;
				PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_SHOWSTATUS, 0), 0);
			}

			TIMECHECK("ステータスバー作成")

			// コンボボックス作成
			m_hCombo = CreateWindowEx(0,
				WC_COMBOBOXEX,
				NULL,
				WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,	// CBS_DROPDOWN
				0, 0, 0, 200,
				hWnd,
				(HMENU)ID_COMBO,
				m_hInst,
				NULL);

			TIMECHECK("コンボボックス作成")

			// ツリー作成
			m_hTree = CreateWindowEx(WS_EX_CLIENTEDGE,
				WC_TREEVIEW,
				NULL,
				WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | (g_cfg.nSingleExpand?TVS_SINGLEEXPAND:0),
				0, 0, 0, 0,
				hWnd,
				(HMENU)ID_TREE,
				m_hInst,
				NULL);
			TreeView_SetBkColor(m_hTree, g_cfg.nBkColor);
			TreeView_SetTextColor(m_hTree, g_cfg.nTextColor);
			DragAcceptFiles(m_hTree, TRUE);

			TIMECHECK("ツリー作成")

			// タブコントロール作成
			m_hTab = CreateWindow( 
				WC_TABCONTROL,
				NULL, 
				WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | (g_cfg.nTabMulti?TCS_MULTILINE:0) | (g_cfg.nTabBottom?TCS_BOTTOM:0),
				0, 0, 20, 20, 
				hWnd,
				(HMENU)ID_TAB,
				m_hInst,
				NULL);
			SendMessage(m_hTab, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
			DragAcceptFiles(m_hTab, TRUE);

			TIMECHECK("タブ作成")

			MakeNewTab(m_hTab, TEXT("フォルダ"), 0);

			TIMECHECK("フォルダタブ作成")

			// プレイリストリスト読み込み
			LoadPlaylists(m_hTab);

			TIMECHECK("プレイリストリスト読み込み")

			//スプリットウィンドウを作成
			s_hSplitter = CreateWindow(TEXT("static"),
				NULL,
				SS_NOTIFY | WS_CHILD | WS_VISIBLE,
				0, 0, 0, 0,
				hWnd,
				NULL,
				m_hInst,
				NULL
			);

			TIMECHECK("スプリッター作成")

			//レバーコントロールの作成
			REBARINFO rbi;
			s_hRebar = CreateWindowEx(WS_EX_TOOLWINDOW,
				REBARCLASSNAME,
				NULL,
				WS_CHILD | WS_VISIBLE | WS_BORDER | /*WS_CLIPSIBLINGS | */WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_AUTOSIZE | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE,
				0, 0, 0, 0,
				hWnd, (HMENU)ID_REBAR, m_hInst, NULL);

			ZeroMemory(&rbi, sizeof(REBARINFO));
			rbi.cbSize = sizeof(REBARINFO);
			rbi.fMask = 0;
			rbi.himl = 0;
			PostMessage(s_hRebar, RB_SETBARINFO, 0, (LPARAM)&rbi);

			TIMECHECK("レバー作成")

			//ツールチップの作成
			m_hTitleTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				NULL, NULL, 
				m_hInst, NULL);

			TIMECHECK("ツールチップ作成")

			m_hSliderTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				hWnd, NULL, 
				m_hInst, NULL);

			// ツールバーの作成
			m_hToolBar = CreateToolBar(s_hRebar);
			//RECT rc;	// ツールバーの横の長さを取得
			//SendMessage(m_hToolBar, TB_GETITEMRECT, SendMessage(m_hToolBar, TB_BUTTONCOUNT, 0, 0) - 1, (LPARAM)&rc);
			TIMECHECK("ツールバー作成")

			//ボリュームバーの作成
			m_hVolume = CreateWindowEx(0,
				TRACKBAR_CLASS,
				NULL,
				WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_BOTH | TBS_TOOLTIPS | TBS_FIXEDLENGTH | WS_CLIPSIBLINGS,
				//rc.right, 1, 120, 20,
				0, 0, 0, 0,//120, 20,
				s_hRebar,
				(HMENU)ID_VOLUME,
				m_hInst,
				NULL); 
			PostMessage(m_hVolume, TBM_SETTHUMBLENGTH, (WPARAM)16, (LPARAM)0);
			PostMessage(m_hVolume, TBM_SETRANGEMAX, (WPARAM)FALSE, (LPARAM)SLIDER_DIVIDED); //精度の設定
			TIMECHECK("ボリュームバー作成")

			//シークバーの作成
			m_hSeek = CreateWindowEx(0,
				TRACKBAR_CLASS,
				NULL,
				WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_BOTTOM | WS_DISABLED | TBS_FIXEDLENGTH | WS_CLIPSIBLINGS,
				0, 0, 0, 0,//120, 20,
				s_hRebar,
				(HMENU)TD_SEEKBAR,
				m_hInst,
				NULL); 
			PostMessage(m_hSeek, TBM_SETTHUMBLENGTH, (WPARAM)16, (LPARAM)0);
			PostMessage(m_hSeek, TBM_SETRANGEMAX, (WPARAM)FALSE, (LPARAM)SLIDER_DIVIDED); //精度の設定
			TIMECHECK("シークバー作成")

			// レバーコントロールの復元
			REBARBANDINFO rbbi;
			ZeroMemory(&rbbi, sizeof(REBARBANDINFO));
			rbbi.cbSize = sizeof(REBARBANDINFO);
			rbbi.fMask  = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_ID;
			rbbi.cyMinChild = 22;
			rbbi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
			for(i=0;i<BAND_COUNT;i++){
				wsprintf(szSec, TEXT("cx%d"), i);
				rbbi.cx = GetPrivateProfileInt(TEXT("Rebar2"), szSec, 0, m_szINIPath);
				wsprintf(szSec, TEXT("wID%d"), i);
				rbbi.wID = GetPrivateProfileInt(TEXT("Rebar2"), szSec, i, m_szINIPath);
				wsprintf(szSec, TEXT("fStyle%d"), i);
				rbbi.fStyle = GetPrivateProfileInt(TEXT("Rebar2"), szSec, (RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS), m_szINIPath); //RBBS_BREAK
				
				switch(rbbi.wID){
					case 0:	// ツールバー
						rbbi.hwndChild = m_hToolBar;
						rbbi.cxMinChild = GetToolbarTrueWidth(m_hToolBar);
						break;

					case 1:	// ボリュームバー
						rbbi.hwndChild = m_hVolume;
						rbbi.cxMinChild = 100;
						break;

					case 2:	// シークバー
						rbbi.hwndChild = m_hSeek;		
						rbbi.cxMinChild = 100;
						break;
				}
				SendMessage(s_hRebar, RB_INSERTBAND, (WPARAM)i, (LPARAM)&rbbi);

				// 非表示時の処理
				if(!(rbbi.fStyle & RBBS_HIDDEN)){
					CheckMenuItem(GetMenu(hWnd), IDM_SHOWMAIN+i, MF_BYCOMMAND | MF_CHECKED);
				}else{
					if(rbbi.wID==0)	ShowWindow(m_hToolBar, SW_HIDE);	// ツールバーが表示されてしまうバグ対策
				}
			}

			TIMECHECK("レバー状態復元")

			// メニューチェック
			if(rbbi.fStyle & RBBS_NOGRIPPER){
				CheckMenuItem(GetMenu(hWnd), IDM_FIXBAR, MF_BYCOMMAND | MF_CHECKED);
			}

			if(g_cfg.nTreeState==TREE_SHOW){
				CheckMenuItem(GetMenu(hWnd), IDM_TOGGLETREE, MF_BYCOMMAND | MF_CHECKED);
			}

			TIMECHECK("メニュー状態復元")

			//コントロールのサブクラス化
			SET_SUBCLASS(m_hSeek, NewSliderProc);
			SET_SUBCLASS(m_hVolume, NewSliderProc);
			SET_SUBCLASS(m_hTab, NewTabProc);
			SET_SUBCLASS(m_hCombo, NewComboProc);
			SET_SUBCLASS(GetWindow(m_hCombo, GW_CHILD), NewEditProc);
			SET_SUBCLASS(m_hTree, NewTreeProc);
			SET_SUBCLASS(s_hSplitter, NewSplitBarProc);

			TIMECHECK("コントロールのサブクラス化")

			// TREE_INIT
			InitTreeIconIndex(m_hCombo, m_hTree, (BOOL)g_cfg.nTreeIcon);

			TIMECHECK("ツリーアイコンの初期化")

			// システムメニューの変更
			m_hTrayMenu = GetSubMenu(LoadMenu(m_hInst, TEXT("TRAYMENU")), 0);
			ZeroMemory(&mii, sizeof(mii));
			mii.fMask = MIIM_TYPE;
			mii.cbSize = sizeof(mii);
			mii.fType = MFT_SEPARATOR;
			InsertMenuItem(GetSystemMenu(hWnd, FALSE), 7, FALSE, &mii);

			mii.fMask = MIIM_TYPE | MIIM_ID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = TEXT("メニューの表示(&V)\tCtrl+M");
			mii.wID = IDM_TOGGLEMENU;
			InsertMenuItem(GetSystemMenu(hWnd, FALSE), 8, FALSE, &mii);

			mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
			mii.fType = MFT_STRING;
			mii.dwTypeData = TEXT("&Fittle");
			mii.hSubMenu = m_hTrayMenu;
			InsertMenuItem(GetSystemMenu(hWnd, FALSE), 9, FALSE, &mii);

			DrawMenuBar(hWnd);

			TIMECHECK("メニュー状態復元2")

			// プレイモードを設定する
			ControlPlayMode(GetMenu(hWnd), (int)GetPrivateProfileInt(TEXT("Main"), TEXT("Mode"), PM_LIST, m_szINIPath));
			m_nRepeatFlag = GetPrivateProfileInt(TEXT("Main"), TEXT("Repeat"), TRUE, m_szINIPath);
			if(m_nRepeatFlag){
				m_nRepeatFlag = FALSE;
				ToggleRepeatMode(GetMenu(hWnd));
			}

			// ウィンドウタイトルの設定
			SetWindowText(hWnd, FITTLE_TITLE);
			lstrcpy(m_ni.szTip, FITTLE_TITLE);

			// タスクトレイ
			if(g_cfg.nTrayOpt==2) SetTaskTray(hWnd);

			// ボリュームの設定
			PostMessage(m_hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)GetPrivateProfileInt(TEXT("Main"), TEXT("Volumes"), SLIDER_DIVIDED, m_szINIPath));

			TIMECHECK("諸々")

			// グローバルホットキー
			RegHotKey(hWnd);

			TIMECHECK("グローバルホットキー")

			// ローカルフック
			m_hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)MyHookProc, m_hInst, GetWindowThreadProcessId(hWnd, NULL));

			TIMECHECK("ローカルフック")

			// クリティカルセクションの作成
			InitializeCriticalSection(&m_cs);

			TIMECHECK("クリティカルセクションの作成")

			// ユーザインターフェイス
			SetUIFont();

			TIMECHECK("外観の初期化")

			// メニューの非表示
			if(!GetPrivateProfileInt(TEXT("Main"), TEXT("MainMenu"), 1, m_szINIPath))
				PostMessage(hWnd, WM_COMMAND, IDM_TOGGLEMENU, 0);

			TIMECHECK("メニューの非表示")

			// コンボボックスの初期化
			SendMessage(hWnd, WM_F4B24_IPC, (WPARAM)WM_F4B24_IPC_UPDATE_DRIVELIST, (LPARAM)0);
			//SetDrivesToCombo(m_hCombo);

			TIMECHECK("コンボボックスの初期化")

			// コマンドラインの処理
			BOOL bCmd;
			bCmd = FALSE;
			for(i=1;i<XARGC;i++){
				if(bCmd) break;
				switch(GetParentDir(XARGV[i], szCmdParPath)){
					case FOLDERS: // フォルダ
					case LISTS:   // リスト
					case ARCHIVES:// アーカイブ
						if(g_cfg.nTreeState==TREE_SHOW){
							MakeTreeFromPath(m_hTree, m_hCombo, szCmdParPath);
						}else{
							lstrcpyn(m_szTreePath, szCmdParPath, MAX_FITTLE_PATH);
							SendMessage(hWnd, WM_COMMAND, IDM_FILEREVIEW, 0);
						}
						bCmd = TRUE;
						break;

					case FILES: // ファイル
						if(g_cfg.nTreeState==TREE_SHOW){
							MakeTreeFromPath(m_hTree, m_hCombo, szCmdParPath);
						}else{
							lstrcpyn(m_szTreePath, szCmdParPath, MAX_FITTLE_PATH);
							SendMessage(hWnd, WM_COMMAND, IDM_FILEREVIEW, 0);
						}
						GetLongPathName(XARGV[i], szCmdParPath, MAX_FITTLE_PATH); // 98以降
						nFileIndex = GetIndexFromPath(GetListTab(m_hTab, 0)->pRoot, szCmdParPath);
						ListView_SingleSelect(GetListTab(m_hTab, 0)->hList, nFileIndex);
						bCmd = TRUE;
						break;
				}
			}

			TIMECHECK("コマンドラインの処理")

			// プラグインを呼び出す
			InitPlugins(hWnd);

			TIMECHECK("プラグインの初期化")

#ifdef UNICODE
			/* サブクラス化による文字化け対策 */
			UniFix(hWnd);
#endif

			TIMECHECK("サブクラス化による文字化け対策")

			if(bCmd){
				PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
			}else{
				// コマンドラインなし
				// 長さ０or存在しないなら最後のパスにする
				if(lstrlen(g_cfg.szStartPath)==0 || !FILE_EXIST(g_cfg.szStartPath)){
					GetPrivateProfileString(TEXT("Main"), TEXT("LastPath"), TEXT(""), szLastPath, MAX_FITTLE_PATH, m_szINIPath);
				}else{
					lstrcpyn(szLastPath, g_cfg.szStartPath, MAX_FITTLE_PATH);
				}
				if(FILE_EXIST(szLastPath)){
					if(g_cfg.nTreeState==TREE_SHOW){
						MakeTreeFromPath(m_hTree, m_hCombo, szLastPath);
						TIMECHECK("ツリー展開")
					}else{
						lstrcpyn(m_szTreePath, szLastPath, MAX_FITTLE_PATH);
						SendMessage(hWnd, WM_COMMAND, IDM_FILEREVIEW, 0);
						TIMECHECK("フォルダ展開")
					}
				}else{
					TreeView_Select(m_hTree, MakeDriveNode(m_hCombo, m_hTree), TVGN_CARET);
				}
				// タブの復元
				TabCtrl_SetCurFocus(m_hTab, m_nPlayTab = GetPrivateProfileInt(TEXT("Main"), TEXT("TabIndex"), 0, m_szINIPath));

				TIMECHECK("タブの復元")

				// 最後に再生していたファイルを再生
				if(g_cfg.nResume){
					if(m_nPlayMode==PM_RANDOM && !g_cfg.nResPosFlag){
						PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
					}else{
						nFileIndex = GetIndexFromPath(GetCurListTab(m_hTab)->pRoot, g_cfg.szLastFile);
						ListView_SingleSelect(GetCurListTab(m_hTab)->hList, nFileIndex);
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
						// ポジションも復元
						if(g_cfg.nResPosFlag){
							BASS_ChannelStop(g_cInfo[g_bNow].hChan);
							BASS_ChannelSetPosition(g_cInfo[g_bNow].hChan, g_cfg.nResPos + g_cInfo[g_bNow].qStart, BASS_POS_BYTE);
							BASS_ChannelPlay(g_cInfo[g_bNow].hChan,  FALSE);
						}
					}
				}else if(GetKeyState(VK_SHIFT) < 0){
					// Shiftキーが押されていたら再生
					PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
				}
				TIMECHECK("レジューム")
			}
			TIMECHECK("ウインドウ作成終了")

			break;

		case WM_SETFOCUS:
			if(GetCurListTab(m_hTab)) SetFocus(GetCurListTab(m_hTab)->hList);
			break;

		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			if(OPGetStatus() != OUTPUT_PLUGIN_STATUS_STOP){
				g_cfg.nResPos = (int)(BASS_ChannelGetPosition(g_cInfo[g_bNow].hChan, BASS_POS_BYTE) - g_cInfo[g_bNow].qStart);
			}else{
				g_cfg.nResPos = 0;
			}
			StopOutputStream(hWnd);
			OPTerm();

			// クリティカルセクションを削除
			DeleteCriticalSection(&m_cs);

			SaveState(hWnd);
			SavePlaylists(m_hTab);

			if(m_bTrayFlag){
				Shell_NotifyIcon(NIM_DELETE, &m_ni);	// タスクトレイに入っていた時の処理
				m_bTrayFlag = FALSE;
			}

			QuitPlugins();	// プラグイン呼び出し

			UnRegHotKey(hWnd);
			UnhookWindowsHookEx(m_hHook);
			BASS_Free(); // Bassの開放
			
			PostQuitMessage(0);
			break;
			
		case WM_SIZE:
			RECT rcTab;
			RECT rcStBar;
			RECT rcCombo;
			int nRebarHeight;
			int sb_size[4];
			HDWP hdwp;

			//最小化イベントの処理
			if(wp==SIZE_MINIMIZED){
				switch(g_cfg.nTrayOpt)
				{
					case 1: // 最小化時
						ShowWindow(hWnd, SW_HIDE);
						SetTaskTray(hWnd);
						break;
					case 2:
						ShowWindow(hWnd, SW_HIDE);
						break;
				}
				break;
			}

			if(!IsWindowVisible(hWnd)){
				return 0;	// 助けてってば～対策
			}

			//ステータスバーの分割変更
			sb_size[3] = LOWORD(lp);
			sb_size[2] = sb_size[3] - 18;
			sb_size[1] = sb_size[2] - 120;
			sb_size[0] = sb_size[1] - 100;
			SendMessage(m_hStatus, SB_SETPARTS, (WPARAM)3, (LPARAM)(LPINT)sb_size);
			
			SendMessage(m_hStatus, WM_SIZE, wp, lp);

			//SendMessage(m_hToolBar, WM_SIZE, wp, lp);
			SendMessage(s_hRebar, WM_SIZE, wp, lp);

			nRebarHeight = (int)SendMessage(s_hRebar, RB_GETBARHEIGHT, 0, 0); 
			if(g_cfg.nShowStatus){
				GetClientRect(m_hStatus, &rcStBar);
			}else{
				rcStBar.bottom = 0;
			}
			GetClientRect(m_hCombo, &rcCombo);
			rcCombo.bottom--;	// 線が重なるように調整

			hdwp = BeginDeferWindowPos(4);

			hdwp = DeferWindowPos(hdwp, m_hCombo, HWND_TOP,
				SPLITTER_WIDTH,
				nRebarHeight + SPLITTER_WIDTH,
				(g_cfg.nTreeState==TREE_SHOW?g_cfg.nTreeWidth:0),
				HIWORD(lp)  - nRebarHeight - SPLITTER_WIDTH - rcCombo.bottom - rcStBar.bottom,
				SWP_NOZORDER);

			hdwp = DeferWindowPos(hdwp, m_hTree, m_hCombo, 
				SPLITTER_WIDTH,
				nRebarHeight + SPLITTER_WIDTH + rcCombo.bottom,
				(g_cfg.nTreeState==TREE_SHOW?g_cfg.nTreeWidth:0),
				HIWORD(lp)  - nRebarHeight - SPLITTER_WIDTH - rcCombo.bottom - rcStBar.bottom,
				SWP_NOZORDER);

			hdwp = DeferWindowPos(hdwp, s_hSplitter, HWND_TOP,
				(g_cfg.nTreeState==TREE_SHOW?SPLITTER_WIDTH + g_cfg.nTreeWidth:0),
				nRebarHeight + SPLITTER_WIDTH,
				SPLITTER_WIDTH,
				HIWORD(lp)  - nRebarHeight - SPLITTER_WIDTH - rcStBar.bottom,
				SWP_NOZORDER);

			hdwp = DeferWindowPos(hdwp, m_hTab, HWND_TOP,
				(g_cfg.nTreeState==TREE_SHOW?SPLITTER_WIDTH*2 + g_cfg.nTreeWidth:SPLITTER_WIDTH),
				nRebarHeight + SPLITTER_WIDTH,
				(g_cfg.nTreeState==TREE_SHOW?LOWORD(lp) - SPLITTER_WIDTH*3 - g_cfg.nTreeWidth:LOWORD(lp) - SPLITTER_WIDTH*2),
				HIWORD(lp)  - nRebarHeight - SPLITTER_WIDTH - rcStBar.bottom,
				SWP_NOZORDER);

			EndDeferWindowPos(hdwp);

			// リストビューリサイズ
			hdwp = BeginDeferWindowPos(1);

			GetClientRect(m_hTab, &rcTab);
			if(!(g_cfg.nTabHide && TabCtrl_GetItemCount(m_hTab)==1)){
				if(rcTab.left<rcTab.right){
					TabCtrl_AdjustRect(m_hTab, FALSE, &rcTab);
				}
			}
			hdwp = DeferWindowPos(hdwp, GetCurListTab(m_hTab)->hList, HWND_TOP,
				rcTab.left,
				rcTab.top,
				rcTab.right - rcTab.left,
				rcTab.bottom - rcTab.top,
				SWP_SHOWWINDOW | SWP_NOZORDER);

			EndDeferWindowPos(hdwp);
			break;

		case WM_COPYDATA: // 文字列受信
			{
				COPYDATASTRUCT *pcds= (COPYDATASTRUCT *)lp;
	#ifdef UNICODE
				WCHAR wszRecievedPath[MAX_FITTLE_PATH];
				LPTSTR pRecieved;
				size_t la = lstrlenA((LPSTR)pcds->lpData) + 1;
				MultiByteToWideChar(CP_ACP, 0, (LPSTR)pcds->lpData, -1, wszRecievedPath, MAX_FITTLE_PATH);
				pRecieved = wszRecievedPath;
				if (la & 1) la++; /* WORD align */
				if (la < pcds->cbData){
					LPWSTR pw = (LPWSTR)(((LPBYTE)pcds->lpData) + la);
					size_t lw = lstrlenW(pw) + 1;
					//if (la * sizeof(CHAR) + lw * sizeof(WCHAR)==pcds->cbData)
					lstrcpynW(wszRecievedPath, pw, (lw > MAX_FITTLE_PATH) ? MAX_FITTLE_PATH : lw);
				}
	#else
				LPTSTR pRecieved = (LPTSTR)pcds->lpData;
	#endif
				if(pcds->dwData==0){
					TCHAR szParPath[MAX_FITTLE_PATH];
					switch(GetParentDir(pRecieved, szParPath)){
						case FOLDERS:
						case LISTS:
						case ARCHIVES:
							MakeTreeFromPath(m_hTree, m_hCombo, szParPath);
							m_nPlayTab = 0;
							SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
							return TRUE;

						case FILES:
							MakeTreeFromPath(m_hTree, m_hCombo, (LPTSTR)szParPath);
							GetLongPathName(pRecieved, szParPath, MAX_FITTLE_PATH); // 98以降
							nFileIndex = GetIndexFromPath(GetListTab(m_hTab, 0)->pRoot, szParPath);
							ListView_SingleSelect(GetCurListTab(m_hTab)->hList, nFileIndex);
							SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
							return TRUE;
					}
				}else{
					struct FILEINFO *pSub = NULL;
					TCHAR szTest[MAX_FITTLE_PATH];
					if (IsURLPath(pRecieved))
						lstrcpyn(szTest, pRecieved, MAX_FITTLE_PATH);
					else
						GetLongPathName(pRecieved, szTest, MAX_FITTLE_PATH);
					SearchFiles(&pSub, szTest, TRUE);
					ListView_SetItemState(GetCurListTab(m_hTab)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
					InsertList(GetCurListTab(m_hTab), -1, pSub);
					ListView_EnsureVisible(GetCurListTab(m_hTab)->hList, ListView_GetItemCount(GetCurListTab(m_hTab)->hList)-1, TRUE);
					return TRUE;
				}
			}

			break;

		case WM_COMMAND:
			TCHAR szNowDir[MAX_FITTLE_PATH];
			switch(LOWORD(wp))
			{
				case IDM_REVIEW: //最新の情報に更新
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					MakeTreeFromPath(m_hTree, m_hCombo, szNowDir); 
					break;

				case IDM_FILEREVIEW:
					if(!s_bReviewAllow) break;
					GetListTab(m_hTab, 0)->nPlaying = -1;
					GetListTab(m_hTab, 0)->nStkPtr = -1;
					DeleteAllList(&(GetListTab(m_hTab, 0)->pRoot));
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					if(SearchFiles(&(GetListTab(m_hTab, 0)->pRoot), szNowDir, GetKeyState(VK_CONTROL) < 0)!=LISTS){
						MergeSort(&(GetListTab(m_hTab, 0)->pRoot), GetListTab(m_hTab, 0)->nSortState);
					}
					TraverseList(GetListTab(m_hTab, 0));
					TabCtrl_SetCurFocus(m_hTab, 0);
					break;

				case IDM_SAVE: //プレイリストに保存
					s_nHitTab = TabCtrl_GetCurSel(m_hTab);
				case IDM_TAB_SAVE:
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					if(IsPlayList(szNowDir) || IsArchive(szNowDir)){
						*PathFindFileName(szNowDir) = TEXT('\0');
					}else{
						MyPathAddBackslash(szNowDir);
					}
					int nRet;
					nRet = SaveFileDialog(szNowDir, GetListTab(m_hTab, s_nHitTab)->szTitle);
					if(nRet){
						WriteM3UFile(GetListTab(m_hTab, s_nHitTab)->pRoot, szNowDir, nRet);
					}
					s_nHitTab = NULL;
					break;

				case IDM_END: //終了
					SendMessage(hWnd, WM_CLOSE, 0, 0);
					break;

				case IDM_PLAY: //再生
					int nLBIndex;

					nLBIndex = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, -1, LVNI_SELECTED);
					if(OPGetStatus() == OUTPUT_PLUGIN_STATUS_PAUSE &&  nLBIndex==GetCurListTab(m_hTab)->nPlaying)
					{
						FilePause();	// ポーズ中なら再開
					}else{ // それ以外なら選択ファイルを再生
						if(nLBIndex!=-1){
							m_nPlayTab = TabCtrl_GetCurSel(m_hTab);
							PlayByUser(hWnd, GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, nLBIndex));
						}
					}
					break;

				case IDM_PLAYPAUSE:
 					switch(OPGetStatus()){
					case OUTPUT_PLUGIN_STATUS_PLAY:
					case OUTPUT_PLUGIN_STATUS_PAUSE:
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PAUSE, 0), 0);
						break;
					case OUTPUT_PLUGIN_STATUS_STOP:
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
						break;
					}
					break;

				case IDM_PAUSE: //一時停止
					FilePause();
					break;

				case IDM_STOP: //停止
					StopOutputStream(hWnd);
					break;

				case IDM_PREV: //前の曲
					struct FILEINFO *pPrev;
					int nPrev;

					switch(m_nPlayMode){
						case PM_LIST:
						case PM_SINGLE:
							if(GetPlayListTab(m_hTab)->nPlaying<=0){
								nPrev = ListView_GetItemCount(GetPlayListTab(m_hTab)->hList)-1;
							}else{
								nPrev = GetPlayListTab(m_hTab)->nPlaying-1;
							}
							pPrev = GetPtrFromIndex(GetPlayListTab(m_hTab)->pRoot, nPrev);
							break;

						case PM_RANDOM:
							if(GetStackPtr(GetPlayListTab(m_hTab))<=0){
								pPrev = NULL;
							}else{
								PopPrevious(GetPlayListTab(m_hTab));	// 現在再生中の曲を削除
								pPrev = PopPrevious(GetPlayListTab(m_hTab));
							}
							break;

						default:
							pPrev = NULL;
							break;
					}
					if(pPrev){
						PlayByUser(hWnd, pPrev);
					}
					break;

				case IDM_NEXT: //次の曲
					struct FILEINFO *pNext;

					pNext = SelectNextFile(FALSE);
					if(pNext){
						PlayByUser(hWnd, pNext);
					}
					break;

				case IDM_SEEKFRONT:
					_BASS_ChannelSeekSecond(g_cInfo[g_bNow].hChan, (float)g_cfg.nSeekAmount, 1);
					break;

				case IDM_SEEKBACK:
					_BASS_ChannelSeekSecond(g_cInfo[g_bNow].hChan, (float)g_cfg.nSeekAmount, -1);
					break;

				case IDM_VOLUP: //音量を上げる
					SendMessage(m_hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SendMessage(m_hVolume, TBM_GETPOS, 0 ,0) + g_cfg.nVolAmount * 10));
					break;

				case IDM_VOLDOWN: //音量を下げる
					SendMessage(m_hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SendMessage(m_hVolume, TBM_GETPOS, 0 ,0) - g_cfg.nVolAmount * 10));
					break;

				case IDM_PM_LIST: //リスト
					ControlPlayMode(m_hMainMenu, PM_LIST);
					break;

				case IDM_PM_RANDOM: //ランダム
					ControlPlayMode(m_hMainMenu, PM_RANDOM);
					break;

				case IDM_PM_SINGLE: //リピート
					ControlPlayMode(m_hMainMenu, PM_SINGLE);
					break;

				case IDM_PM_TOGGLE:
					ControlPlayMode(m_hMainMenu, m_nPlayMode+1);
					break;

				case IDM_PM_REPEAT:
					ToggleRepeatMode(m_hMainMenu);
					break;

				case IDM_SUB: //サブフォルダを検索
					GetListTab(m_hTab, 0)->nPlaying = -1;
					GetListTab(m_hTab, 0)->nStkPtr = -1;
					DeleteAllList(&(GetListTab(m_hTab, 0)->pRoot));
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					SetCursor(LoadCursor(NULL, IDC_WAIT));  // 砂時計カーソルにする
					if (PathIsDirectory(szNowDir)){
						SearchFolder(&(GetListTab(m_hTab, 0)->pRoot), szNowDir, TRUE);
						MergeSort(&(GetListTab(m_hTab, 0)->pRoot), GetListTab(m_hTab, 0)->nSortState);
					}else if(IsPlayListFast(szNowDir)){
						ReadM3UFile(&(GetListTab(m_hTab, 0)->pRoot), szNowDir);
					}else if(IsArchiveFast(szNowDir)){
						ReadArchive(&(GetListTab(m_hTab, 0)->pRoot), szNowDir);
					}else{
					}
					TraverseList(GetListTab(m_hTab, 0));
					SetCursor(LoadCursor(NULL, IDC_ARROW)); // 矢印カーソルに戻す
					TabCtrl_SetCurFocus(m_hTab, 0);
					break;

				case IDM_FIND:
					nFileIndex = (int)DialogBoxParam(m_hInst, TEXT("FIND_DIALOG"), hWnd, FindDlgProc, (LPARAM)GetCurListTab(m_hTab)->pRoot);
					if(nFileIndex!=-1){
						ListView_SingleSelect(GetCurListTab(m_hTab)->hList, nFileIndex);
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
					}
					break;

				case IDM_TOGGLEMENU:
					if(GetMenu(hWnd)){
						SetMenu(hWnd, NULL);
					}else{
						SetMenu(hWnd, m_hMainMenu);
					}
					break;

				case IDM_MINIPANEL:
					break;

				case IDM_SHOWSTATUS:
					g_cfg.nShowStatus = g_cfg.nShowStatus?0:1;	// 反転
					CheckMenuItem(m_hMainMenu, LOWORD(wp), g_cfg.nShowStatus?MF_CHECKED:MF_UNCHECKED);
					ShowWindow(m_hStatus, g_cfg.nShowStatus?SW_SHOW:SW_HIDE);
					UpdateWindowSize(hWnd);
					break;

				case IDM_SHOWMAIN:	// メインツールバー
				case IDM_SHOWVOL:	// ボリューム
				case IDM_SHOWSEEK:	// シークバー

					ZeroMemory(&rbbi, sizeof(rbbi));
					rbbi.cbSize = sizeof(rbbi);
					rbbi.fMask  = RBBIM_STYLE;
					SendMessage(s_hRebar, RB_GETBANDINFO, (WPARAM)SendMessage(s_hRebar, RB_IDTOINDEX, (WPARAM)(LOWORD(wp)-IDM_SHOWMAIN), 0), (LPARAM)&rbbi);
					if(rbbi.fStyle & RBBS_HIDDEN){
						rbbi.fStyle &= ~RBBS_HIDDEN;
						CheckMenuItem(m_hMainMenu, LOWORD(wp), MF_CHECKED);
					}else{
						rbbi.fStyle |= RBBS_HIDDEN;
						CheckMenuItem(m_hMainMenu, LOWORD(wp), MF_UNCHECKED);
					}
					SendMessage(s_hRebar, RB_SETBANDINFO, (WPARAM)SendMessage(s_hRebar, RB_IDTOINDEX, (WPARAM)(LOWORD(wp)-IDM_SHOWMAIN), 0), (LPARAM)&rbbi);
					break;

				case IDM_FIXBAR:
					//REBARBANDINFO rbbi;
					ZeroMemory(&rbbi, sizeof(rbbi));
					rbbi.cbSize = sizeof(rbbi);
					rbbi.fMask  = RBBIM_STYLE | RBBIM_SIZE;
					rbbi.fStyle = 0;
					for(i=0;i<BAND_COUNT;i++){
						SendMessage(s_hRebar, RB_GETBANDINFO, i, (LPARAM)&rbbi);
						if(rbbi.fStyle & RBBS_NOGRIPPER){
							rbbi.fStyle |= RBBS_GRIPPERALWAYS;
							rbbi.fStyle &= ~RBBS_NOGRIPPER;
							rbbi.cx += 10;
						}else{
							rbbi.fStyle |= RBBS_NOGRIPPER;
							rbbi.fStyle &= ~RBBS_GRIPPERALWAYS;
							rbbi.cx -= 10;
						}
						SendMessage(s_hRebar, RB_SETBANDINFO, i, (LPARAM)&rbbi);
					}
					CheckMenuItem(m_hMainMenu, IDM_FIXBAR, MF_BYCOMMAND | (rbbi.fStyle & RBBS_NOGRIPPER)?MF_CHECKED:MF_UNCHECKED);
					break;

				case IDM_TOGGLETREE:
					switch(g_cfg.nTreeState){
						case TREE_UNSHOWN:
							s_bReviewAllow = FALSE;
							MakeTreeFromPath(m_hTree, m_hCombo, m_szTreePath);
							s_bReviewAllow = TRUE;
							CheckMenuItem(m_hMainMenu, IDM_TOGGLETREE, MF_BYCOMMAND | MF_CHECKED);
							g_cfg.nTreeState = TREE_SHOW;
							break;

						case TREE_SHOW:
							//EnableWindow(m_hTree, FALSE);
							CheckMenuItem(m_hMainMenu, IDM_TOGGLETREE, MF_BYCOMMAND | MF_UNCHECKED);
							g_cfg.nTreeState = TREE_HIDE;
							SetFocus(GetCurListTab(m_hTab)->hList);
							break;

						case TREE_HIDE:
							//EnableWindow(m_hTree, TRUE);
							s_bReviewAllow = FALSE;
							MakeTreeFromPath(m_hTree, m_hCombo, m_szTreePath);
							s_bReviewAllow = TRUE;
							CheckMenuItem(m_hMainMenu, IDM_TOGGLETREE, MF_BYCOMMAND | MF_CHECKED);
							g_cfg.nTreeState = TREE_SHOW;
							break;
					}
					UpdateWindowSize(hWnd);
					break;

				case IDM_SETTING:
					SendMessage(hWnd, WM_F4B24_IPC, WM_F4B24_IPC_SETTING, WM_F4B24_IPC_SETTING_LP_GENERAL);
					break;


				case IDM_BM_PLAYING:
					if(FILE_EXIST(g_cInfo[g_bNow].szFilePath)){
						GetParentDir(g_cInfo[g_bNow].szFilePath, szNowDir);
						MakeTreeFromPath(m_hTree, m_hCombo, szNowDir);
					}else if(IsArchivePath(g_cInfo[g_bNow].szFilePath)){
						LPTSTR p = StrStr(g_cInfo[g_bNow].szFilePath, TEXT("/"));
						*p = TEXT('\0');
						lstrcpyn(szNowDir, g_cInfo[g_bNow].szFilePath, MAX_FITTLE_PATH);
						*p = TEXT('/');
						MakeTreeFromPath(m_hTree, m_hCombo, szNowDir);
					}
					break;

				case IDM_README:	// Readme.txtを表示
					GetModuleParentDir(szNowDir);
					lstrcat(szNowDir, TEXT("Readme.txt"));
					ShellExecute(hWnd, NULL, szNowDir, NULL, NULL, SW_SHOWNORMAL);
					break;

				case IDM_VER:	// バージョン情報
					SendMessage(hWnd, WM_F4B24_IPC, WM_F4B24_IPC_SETTING, WM_F4B24_IPC_SETTING_LP_ABOUT);
					break;

				case IDM_LIST_MOVETOP:	// 一番上に移動
					ChangeOrder(GetCurListTab(m_hTab), 0);
					break;

				case IDM_LIST_MOVEBOTTOM: //一番下に移動
					ChangeOrder(GetCurListTab(m_hTab), ListView_GetItemCount(GetCurListTab(m_hTab)->hList) - 1);
					break;

				case IDM_LIST_URL:
					break;

				case IDM_LIST_NEW:	// 新規プレイリスト
					TCHAR szLabel[MAX_FITTLE_PATH];
					lstrcpy(szLabel, TEXT("Default"));
					if(DialogBoxParam(m_hInst, TEXT("TAB_NAME_DIALOG"), hWnd, TabNameDlgProc, (LPARAM)szLabel)){
						MakeNewTab(m_hTab, szLabel, -1);
						SendToPlaylist(GetCurListTab(m_hTab), GetListTab(m_hTab, TabCtrl_GetItemCount(m_hTab)-1));
						TabCtrl_SetCurFocus(m_hTab, TabCtrl_GetItemCount(m_hTab)-1);
					}
					break;

				case IDM_LIST_ALL:	// 全て選択
					ListView_SetItemState(GetCurListTab(m_hTab)->hList, -1, LVIS_SELECTED, LVIS_SELECTED);
					break;

				case IDM_LIST_DEL: //リストから削除
					DeleteFiles(GetCurListTab(m_hTab));
					break;

				case IDM_LIST_DELFILE:
					RemoveFiles(hWnd);
					break;

				case IDM_LIST_TOOL:
					if((nLBIndex = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, -1, LVNI_SELECTED))!=-1){
						if(lstrlen(g_cfg.szToolPath)!=0){
							LPTSTR pszFiles = MallocAndConcatPath(GetCurListTab(m_hTab));
							if (pszFiles) {
								MyShellExecute(hWnd, g_cfg.szToolPath, pszFiles, TRUE);
								HFree(pszFiles);
							}
						}else{
							SendMessage(hWnd, WM_F4B24_IPC, WM_F4B24_IPC_SETTING, WM_F4B24_IPC_SETTING_LP_PATH);
						}
					}
					break;

				case IDM_LIST_PROP:	// プロパティ
					SHELLEXECUTEINFO sei;
					
					if((nLBIndex = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, -1, LVNI_SELECTED))!=-1){
						ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
						sei.cbSize = sizeof(SHELLEXECUTEINFO);
						sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
						sei.lpFile = GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, nLBIndex)->szFilePath;
						sei.lpVerb = TEXT("properties");
						sei.hwnd = NULL;
						ShellExecuteEx(&sei);
					}
					break;

				case IDM_TREE_UP:
					HTREEITEM hParent;

					if(g_cfg.nTreeState==TREE_UNSHOWN){
						if(!PathIsRoot(m_szTreePath)){
							*PathFindFileName(m_szTreePath) = TEXT('\0');
							MakeTreeFromPath(m_hTree, m_hCombo, m_szTreePath);
						}
					}else{
						if(!m_hHitTree) m_hHitTree = TreeView_GetNextItem(m_hTree, TVI_ROOT, TVGN_CARET);
						hParent = TreeView_GetParent(m_hTree, m_hHitTree);
						if(hParent!=NULL){
							TreeView_Select(m_hTree, hParent, TVGN_CARET);
							MyScroll(m_hTree);
						}else{
							GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
							if(szNowDir[3]){
								PathRemoveFileSpec(szNowDir);
								MakeTreeFromPath(m_hTree, m_hCombo, szNowDir);
							}
						}
						m_hHitTree = NULL;
					}
					break;

				case IDM_TREE_SUB:
					TreeView_Select(m_hTree, m_hHitTree, TVGN_CARET);
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_SUB, 0);
					m_hHitTree = NULL;
					break;

				case IDM_TREE_ADD:
				//case IDM_TREE_SUBADD:
					struct FILEINFO *pSub;
					pSub = NULL;

					GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					SearchFiles(&pSub, szNowDir, TRUE);
					ListView_SetItemState(GetCurListTab(m_hTab)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
					InsertList(GetCurListTab(m_hTab), -1, pSub);
					ListView_EnsureVisible(GetCurListTab(m_hTab)->hList, ListView_GetItemCount(GetCurListTab(m_hTab)->hList)-1, TRUE);
					m_hHitTree = NULL;
					break;

				case IDM_TREE_NEW:
					struct LISTTAB *pNew;

					GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					lstrcpyn(szLabel, PathFindFileName(szNowDir), MAX_FITTLE_PATH);
					if(IsPlayList(szNowDir) || IsArchive(szNowDir)){
						// ".m3u"を削除
						PathRemoveExtension(szLabel);
					}
					if(DialogBoxParam(m_hInst, TEXT("TAB_NAME_DIALOG"), hWnd, TabNameDlgProc, (LPARAM)szLabel)){
						pNew = MakeNewTab(m_hTab, szLabel, -1);
						SearchFiles(&(pNew->pRoot), szNowDir, TRUE);
						TraverseList(pNew);
						TabCtrl_SetCurFocus(m_hTab, TabCtrl_GetItemCount(m_hTab)-1);
						InvalidateRect(m_hTab, NULL, FALSE);
					}
					m_hHitTree = NULL;
					break;

				case IDM_EXPLORE:
				case IDM_TREE_EXPLORE:
					// パスの取得
					if(LOWORD(wp)==IDM_EXPLORE){
						lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					}else{
						GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					}
					// 前処理
					if(IsPlayList(szNowDir) || IsArchive(szNowDir)){
						*PathFindFileName(szNowDir) = TEXT('\0');
					}else{
						MyPathAddBackslash(szNowDir);
					}
					// エクスプローラに投げる処理
					MyShellExecute(hWnd, (g_cfg.szFilerPath[0]?g_cfg.szFilerPath:TEXT("explorer.exe")), szNowDir, FALSE);
					m_hHitTree = NULL;
					break;

				case IDM_TAB_NEW:
					lstrcpy(szLabel, TEXT("Default"));
					if(DialogBoxParam(m_hInst, TEXT("TAB_NAME_DIALOG"), hWnd, TabNameDlgProc, (LPARAM)szLabel)){
						MakeNewTab(m_hTab, szLabel, -1);
						TabCtrl_SetCurFocus(m_hTab, TabCtrl_GetItemCount(m_hTab)-1);
					}
					break;

				case IDM_TAB_RENAME:
					lstrcpyn(szLabel, GetListTab(m_hTab, s_nHitTab)->szTitle, MAX_FITTLE_PATH);
					if(DialogBoxParam(m_hInst, TEXT("TAB_NAME_DIALOG"), hWnd, TabNameDlgProc, (LPARAM)szLabel)){
						RenameListTab(m_hTab, s_nHitTab, szLabel);
						InvalidateRect(m_hTab, NULL, FALSE);
					}
					break;

				case IDM_TAB_DEL:
					// カレントタブを消すなら左をカレントにする
					if(TabCtrl_GetCurSel(m_hTab)==s_nHitTab){
						TabCtrl_SetCurFocus(m_hTab, s_nHitTab-1);
					}
					// 再生タブを消すなら再生タブをクリア
					if(s_nHitTab==m_nPlayTab){
						m_nPlayTab = -1;
					}
					RemoveListTab(m_hTab, s_nHitTab);
					break;

				case IDM_TAB_LEFT:	// 左へ移動
					MoveTab(m_hTab, s_nHitTab, -1);
					break;

				case IDM_TAB_RIGHT:	// 右へ移動
					MoveTab(m_hTab, s_nHitTab, 1);
					break;

				case IDM_TAB_FOR_RIGHT:
					TabCtrl_SetCurFocus(m_hTab, (TabCtrl_GetCurSel(m_hTab)+1==TabCtrl_GetItemCount(m_hTab)?0:TabCtrl_GetCurSel(m_hTab)+1));
					break;

				case IDM_TAB_FOR_LEFT:
					TabCtrl_SetCurFocus(m_hTab, (TabCtrl_GetCurSel(m_hTab)?TabCtrl_GetCurSel(m_hTab)-1:TabCtrl_GetItemCount(m_hTab)-1));
					break;

				case IDM_TAB_SORT:
					SortListTab(GetCurListTab(m_hTab), 0);
					break;

				case IDM_TAB_NOEXIST:
					ListView_SetItemCountEx(GetCurListTab(m_hTab)->hList, LinkCheck(&(GetCurListTab(m_hTab)->pRoot)), 0);
					break;

				case IDM_TRAY_WINVIEW:
					ToggleWindowView(hWnd);
					break;

				case IDM_TOGGLEFOCUS:
					if(GetFocus()!=GetCurListTab(m_hTab)->hList){
						SetFocus(GetCurListTab(m_hTab)->hList);
					}else{
						SetFocus(m_hTree);
					}
					break;

				case ID_COMBO:
					if(HIWORD(wp)==CBN_SELCHANGE){
						TreeView_Select(m_hTree, MakeDriveNode(m_hCombo, m_hTree), TVGN_CARET);
					}
					break;

				default:
					if(IDM_SENDPL_FIRST<=LOWORD(wp) && LOWORD(wp)<IDM_BM_FIRST){
						//プレイリストに送る
						SendToPlaylist(GetCurListTab(m_hTab), GetListTab(m_hTab, LOWORD(wp) - IDM_SENDPL_FIRST));
					}else{
						return DefWindowProc(hWnd, msg, wp, lp);
					}
					break;
			}
			break;

		case WM_SYSCOMMAND:
			switch (wp) {
				case IDM_PLAY: //再生
				case IDM_PAUSE: //一時停止
				case IDM_PLAYPAUSE:
				case IDM_STOP: //停止
				case IDM_PREV: //前の曲
				case IDM_NEXT: //次の曲
				case IDM_PM_LIST: // リスト
				case IDM_PM_RANDOM: // ランダム
				case IDM_PM_SINGLE: // シングル
				case IDM_PM_REPEAT: // リピート
				case IDM_TOGGLEMENU:
				case IDM_END: // 終了
				case IDM_TRAY_WINVIEW:
					SendMessage(hWnd, WM_COMMAND, wp, 0);
					break;

				case SC_CLOSE:
					if(g_cfg.nCloseMin){
						ShowWindow(hWnd, SW_MINIMIZE);
						return 0;
					}

				default:
					return DefWindowProc(hWnd, msg, wp, lp);
			}
			break;

		case WM_CONTEXTMENU:
			HMENU hPopMenu;
			RECT rcItem;

			if((HWND)wp==hWnd){
				return DefWindowProc(hWnd, msg, wp, lp);		
			}else{
				switch(GetDlgCtrlID((HWND)wp))
				{
					case ID_TREE:
						TVHITTESTINFO tvhti;

						if(lp==-1){	// キーボード
							m_hHitTree = TreeView_GetNextItem(m_hTree, TVI_ROOT, TVGN_CARET);
							if(!m_hHitTree) break; 
							TreeView_GetItemRect(m_hTree, m_hHitTree, &rcItem, TRUE);
							MapWindowPoints(m_hTree, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
							lp = MAKELPARAM(rcItem.left, rcItem.bottom);
						}else{	// マウス
							tvhti.pt.x = (short)LOWORD(lp);
							tvhti.pt.y = (short)HIWORD(lp);
							ScreenToClient(m_hTree, &tvhti.pt);
							TreeView_HitTest(m_hTree, &tvhti);
							m_hHitTree = tvhti.hItem;
						}
						TreeView_SelectDropTarget(m_hTree, m_hHitTree);
						hPopMenu = GetSubMenu(LoadMenu(m_hInst, TEXT("TREEMENU")), 0);
						TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, hWnd, NULL);
						DestroyMenu(hPopMenu);
						TreeView_SelectDropTarget(m_hTree, NULL);
						break;

					case ID_TAB:
						TCHITTESTINFO tchti;

						if(lp==-1){	// キーボード
							s_nHitTab = TabCtrl_GetCurSel(m_hTab);
							TabCtrl_GetItemRect(m_hTab, s_nHitTab, &rcItem);
							MapWindowPoints(m_hTab, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
							lp = MAKELPARAM(rcItem.left, rcItem.bottom);
						}else{	// マウス
							tchti.pt.x = (short)LOWORD(lp);
							tchti.pt.y = (short)HIWORD(lp);
							tchti.flags = TCHT_NOWHERE;
							ScreenToClient(m_hTab, &tchti.pt);
							s_nHitTab = TabCtrl_HitTest(m_hTab, &tchti);
						}
						hPopMenu = GetSubMenu(LoadMenu(m_hInst, TEXT("TABMENU")), 0);
						// グレー表示の処理
						if(s_nHitTab<=0){
							EnableMenuItem(hPopMenu, IDM_TAB_DEL, MF_GRAYED | MF_DISABLED);
							EnableMenuItem(hPopMenu, IDM_TAB_RENAME, MF_GRAYED | MF_DISABLED);
						}
						if(s_nHitTab<=1) EnableMenuItem(hPopMenu, IDM_TAB_LEFT, MF_GRAYED | MF_DISABLED);
						if(s_nHitTab>=TabCtrl_GetItemCount(m_hTab)-1 || s_nHitTab<=0)
							EnableMenuItem(hPopMenu, IDM_TAB_RIGHT, MF_GRAYED | MF_DISABLED);
						TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, hWnd, NULL);
						DestroyMenu(hPopMenu);
						break;

					case ID_REBAR:
						POINT pt;
						HMENU hOptionMenu = GetSubMenu(m_hMainMenu, GetMenuPosFromString(m_hMainMenu, TEXT("オプション(&O)")));
						GetCursorPos(&pt);
						TrackPopupMenu(GetSubMenu(hOptionMenu, GetMenuPosFromString(hOptionMenu, TEXT("ツールバー(&B)"))), TPM_LEFTALIGN | TPM_TOPALIGN, /*(short)LOWORD(lp)*/pt.x, /*(short)HIWORD(lp)*/pt.y, 0, hWnd, NULL);
						break;
				}
			}
			break;


		case WM_TRAY:
			switch(lp){
				case WM_LBUTTONDOWN:
					if(s_nClickState==0){
						s_nClickState = 1;
						SetTimer(hWnd, ID_LBTNCLKTIMER, (g_cfg.nTrayClick[1]?GetDoubleClickTime():0), NULL);
					}
					break;

				case WM_RBUTTONDOWN:
					if(s_nClickState==0){
						s_nClickState = 1;
						SetTimer(hWnd, ID_RBTNCLKTIMER, (g_cfg.nTrayClick[3]?GetDoubleClickTime():0), NULL);
					}
					break;

				case WM_MBUTTONDOWN:
					if(s_nClickState==0){
						s_nClickState = 1;
						SetTimer(hWnd, ID_MBTNCLKTIMER, (g_cfg.nTrayClick[5]?GetDoubleClickTime():0), NULL);
					}
					break;

				case WM_LBUTTONUP:
					if(s_nClickState==1){
						s_nClickState = 2;
					}else if(s_nClickState==3){
						DoTrayClickAction(hWnd, 0);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_RBUTTONUP:
					if(s_nClickState==1){
						s_nClickState = 2;
					}else if(s_nClickState==3){
						DoTrayClickAction(hWnd, 2);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_MBUTTONUP:
					if(s_nClickState==1){
						s_nClickState = 2;
					}else if(s_nClickState==3){
						DoTrayClickAction(hWnd, 4);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_LBUTTONDBLCLK:
					KillTimer(hWnd, ID_LBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(hWnd, 1);
					break;

				case WM_RBUTTONDBLCLK:
					KillTimer(hWnd, ID_RBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(hWnd, 3);
					break;

				case WM_MBUTTONDBLCLK:
					KillTimer(hWnd, ID_MBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(hWnd, 5);
					break;

				default:
					return DefWindowProc(hWnd, msg, wp, lp);
			}
			break;

		case WM_TIMER:
			QWORD qPos;
			QWORD qLen;
			int nPos;
			int nLen;

			switch (wp){
				case ID_SEEKTIMER:
					//再生時間をステータスバーに表示
					qPos = BASS_ChannelGetPosition(g_cInfo[g_bNow].hChan, BASS_POS_BYTE) - g_cInfo[g_bNow].qStart;
					qLen = g_cInfo[g_bNow].qDuration;
					nPos = (int)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, qPos);
					nLen = (int)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, qLen);
					wsprintf(m_szTime, TEXT("\t%02d:%02d / %02d:%02d"), nPos/60, nPos%60, nLen/60, nLen%60);
					PostMessage(m_hStatus, SB_SETTEXT, (WPARAM)2|0, (LPARAM)m_szTime);
					//シーク中でなければ現在の再生位置をスライダバーに表示する
					if(!(GetCapture()==m_hSeek || qLen==0))
						PostMessage(m_hSeek, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SLIDER_DIVIDED * qPos / qLen));
					break;

				case ID_TIPTIMER:
					//ツールチップを消す
					SendMessage(m_hTitleTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, NULL);
					KillTimer(hWnd, ID_TIPTIMER);
					break;

				case ID_LBTNCLKTIMER:
					KillTimer(hWnd, ID_LBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(hWnd, 0);
						s_nClickState = 0;
					}else if(s_nClickState==1){
						s_nClickState = 3;
					}
					break;

				case ID_RBTNCLKTIMER:
					KillTimer(hWnd, ID_RBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(hWnd, 2);
						s_nClickState = 0;
					}else if(s_nClickState==1){
						s_nClickState = 3;
					}
					break;

				case ID_MBTNCLKTIMER:
					KillTimer(hWnd, ID_MBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(hWnd, 4);
						s_nClickState = 0;
					}else if(s_nClickState==1){
						s_nClickState = 3;
					}
					break;

				default:
					//他のタイマーはDefaultにまかせる
					return DefWindowProc(hWnd, msg, wp, lp);
			}
			break;

		case WM_NOTIFY:
			NMHDR *pnmhdr;

			pnmhdr = (LPNMHDR)lp;
			switch(pnmhdr->idFrom){
				case ID_TREE:
					NMTREEVIEW *pnmtv;
					switch(pnmhdr->code){
						case TVN_ITEMEXPANDED: // 表示位置調整
							MyScroll(m_hTree);
							break;

						case TVN_SELCHANGED: // フォルダがかわったら検索
							pnmtv = (LPNMTREEVIEW)lp;
							GetPathFromNode(m_hTree, pnmtv->itemNew.hItem, m_szTreePath);
							SendMessage(hWnd, WM_COMMAND, IDM_FILEREVIEW, 0);
							break;

						case TVN_ITEMEXPANDING: // ツリーが開いたらサブノードを追加
							pnmtv = (LPNMTREEVIEW)lp; 
							if(pnmtv->action==TVE_EXPAND){
								MakeTwoTree(m_hTree, (pnmtv->itemNew).hItem);
							}
							break;

						case TVN_BEGINDRAG:	// ドラッグ開始
							pnmtv = (LPNMTREEVIEW)lp;
							m_hHitTree = pnmtv->itemNew.hItem;
							OnBeginDragTree(m_hTree);
							break;
					}
					break;

				case ID_TAB:
					switch(pnmhdr->code)
					{
						case TCN_SELCHANGING:
							LockWindowUpdate(hWnd);
							ShowWindow(GetCurListTab(m_hTab)->hList, SW_HIDE);
							break;
						
						case TCN_SELCHANGE:
							GetClientRect(m_hTab, &rcTab);
							if(!(g_cfg.nTabHide && TabCtrl_GetItemCount(m_hTab)==1)){
								TabCtrl_AdjustRect(m_hTab, FALSE, &rcTab);
							}
							SendMessage(GetCurListTab(m_hTab)->hList, WM_SETFONT, (WPARAM)m_hFont, 0);
							SetWindowPos(GetCurListTab(m_hTab)->hList,
								HWND_TOP,
								rcTab.left,
								rcTab.top,
								rcTab.right - rcTab.left,
								rcTab.bottom - rcTab.top,
								SWP_SHOWWINDOW);
							SetFocus(GetCurListTab(m_hTab)->hList);
							LockWindowUpdate(NULL);
							break;
					}
					break;

				case ID_REBAR:
					// リバーの行数が変わったらWM_SIZEを投げる
					if(pnmhdr->code==RBN_HEIGHTCHANGE){
						UpdateWindow(s_hRebar);
						UpdateWindowSize(hWnd);
					}
					break;

				case ID_STATUS:
					if(pnmhdr->code==NM_DBLCLK){
						SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_MINIPANEL, 0);
					}
					break;

				default:
					// ツールバーのツールチップの表示
					if(pnmhdr->code==TTN_NEEDTEXT){ 
						TOOLTIPTEXT *lptip;
						lptip = (LPTOOLTIPTEXT)lp; 
						switch (lptip->hdr.idFrom){ 
							case IDM_PLAY: 
								lptip->lpszText = TEXT("再生 (Z)"); 
								break; 
							case IDM_PAUSE: 
								lptip->lpszText = TEXT("一時停止 (X)"); 
								break; 
							case IDM_STOP: 
								lptip->lpszText = TEXT("停止 (C)"); 
								break; 
							case IDM_PREV: 
								lptip->lpszText = TEXT("前の曲 (V)"); 
								break; 
							case IDM_NEXT: 
								lptip->lpszText = TEXT("次の曲 (B)"); 
								break; 
							case IDM_PM_TOGGLE:
								switch (m_nPlayMode)
								{
									case PM_LIST:
										lptip->lpszText = TEXT("リスト"); 
										break;
									case PM_RANDOM:
										lptip->lpszText = TEXT("ランダム"); 
										break;
									case PM_SINGLE:
										lptip->lpszText = TEXT("シングル"); 
										break;
								}
								break; 
							case IDM_PM_REPEAT: 
								lptip->lpszText = TEXT("リピート (Ctrl+R)"); 
								break;
						}
					}else if(pnmhdr->code==TBN_DROPDOWN){
						// ドロップダウンメニューの表示
						PopupPlayModeMenu(pnmhdr->hwndFrom, (LPNMTOOLBAR)lp);
					}
					break;
			}
			break;

		case WM_HOTKEY: // ホットキー
			switch((int)wp){
				case 0:	// 再生
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
					break;
				case 1:	// 再生/一時停止
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAYPAUSE, 0), 0);
					break;
				case 2:	// 停止
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_STOP, 0), 0);
					break;
				case 3:	// 前の曲
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PREV, 0), 0);
					break;
				case 4:	// 次の曲
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
					break;
				case 5:	// 音量を上げる
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_VOLUP, 0);
					break;
				case 6:	// 音量を下げる
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_VOLDOWN, 0);
					break;
				case 7: // タスクトレイから復帰
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_TRAY_WINVIEW, 0), 0);
					break;
			}
			break;

		case WM_FITTLE:	// プラグインインターフェイス
			switch(wp){
				case GET_TITLE:
#ifdef UNICODE
					return (LRESULT)&m_taginfoA;
#else
					return (LRESULT)&m_taginfo;
#endif

				case GET_ARTIST:
#ifdef UNICODE
					return (LRESULT)m_taginfoA.szArtist;
#else
					return (LRESULT)m_taginfo.szArtist;
#endif

				case GET_PLAYING_PATH:
#ifdef UNICODE
					lstrcpyntA(m_szFilePathA, g_cInfo[g_bNow].szFilePath, MAX_FITTLE_PATH);
					return (LRESULT)m_szFilePathA;
#else
					return (LRESULT)g_cInfo[g_bNow].szFilePath;
#endif

				case GET_PLAYING_INDEX:
					return (LRESULT)GetCurListTab(m_hTab)->nPlaying;

				case GET_STATUS:
					return (LRESULT)OPGetStatus();

				case GET_POSITION:
					return (LRESULT)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, BASS_ChannelGetPosition(g_cInfo[g_bNow].hChan, BASS_POS_BYTE) - g_cInfo[g_bNow].qStart);

				case GET_DURATION:
					return (LRESULT)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, g_cInfo[g_bNow].qDuration);

				case GET_LISTVIEW:
					int nCount;

					if(lp<0){
						return (LRESULT)GetCurListTab(m_hTab)->hList;
					}else{
						nCount = TabCtrl_GetItemCount(m_hTab);
						for(i=0;i<nCount;i++){
							if(lp==i){
								return (LRESULT)GetListTab(m_hTab, (int)lp)->hList;
							}
						}
					}
					return NULL;

				case GET_CONTROL:
					switch(lp){
						case ID_COMBO:
						case ID_TREE:
						case ID_TAB:
						case ID_REBAR:
						case ID_STATUS:
							return (LRESULT)GetDlgItem(hWnd, lp);
						case TD_SEEKBAR:
						case ID_TOOLBAR:
						case ID_VOLUME:
							return (LRESULT)GetDlgItem(s_hRebar, lp);
					}
					break;

				case GET_CURPATH:
#ifdef UNICODE
					lstrcpyntA(m_szTreePathA, m_szTreePath, MAX_FITTLE_PATH);
					return (LRESULT)m_szTreePathA;
#else
					return (LRESULT)m_szTreePath;
#endif

				case GET_MENU:
					return (HRESULT)m_hMainMenu;

				case GET_MINIPANEL:
					return 0;

				default:
					break;
			}
			break;

		case WM_F4B24_IPC:
			switch(wp){
			case WM_F4B24_INTERNAL_PLAY_NEXT:
				EnterCriticalSection(&m_cs);

				// ファイル切り替え
				g_bNow = !g_bNow;
				// 開放
				FreeChannelInfo(!g_bNow);

				LeaveCriticalSection(&m_cs);

				PostMessage(GetParent(m_hStatus), WM_USER+1, 0, 0); 
				break;

			case WM_F4B24_INTERNAL_SETUP_NEXT:
				g_pNextFile = SelectNextFile(TRUE);
				m_nGaplessState = GS_OK;

				if(g_pNextFile){
					// オープン
					if(!SetChannelInfo(!g_bNow, g_pNextFile)){
						m_nGaplessState = GS_FAILED;
					}else{
						BASS_CHANNELINFO info1,info2;
						// 二つのファイルの周波数、チャンネル数、ビット数をチェック
						BASS_ChannelGetInfo(g_cInfo[g_bNow].hChan, &info1);
						BASS_ChannelGetInfo(g_cInfo[!g_bNow].hChan, &info2);
						if(info1.freq!=info2.freq || info1.chans!=info2.chans || info1.flags!=info2.flags){
							m_nGaplessState = GS_NEWFREQ;
						}else{
							m_nGaplessState = GS_OK;
						}
					}
				}
				break;
			case WM_F4B24_IPC_GET_VERSION:
				return F4B24_VERSION;
			case WM_F4B24_IPC_GET_IF_VERSION:
				return F4B24_IF_VERSION;
			case WM_F4B24_IPC_APPLY_CONFIG:
				ApplyConfig(hWnd);
				break;
			case WM_F4B24_IPC_UPDATE_DRIVELIST:
				SetDrivesToCombo(m_hCombo);
				break;
			case WM_F4B24_IPC_GET_REPLAYGAIN_MODE:
				return g_cfg.nReplayGainMode;
			case WM_F4B24_IPC_GET_PREAMP:
				return g_cfg.nReplayGainPreAmp;
			case WM_F4B24_IPC_INVOKE_OUTPUT_PLUGIN_SETUP:
				return OPSetup(hWnd);

			case WM_F4B24_IPC_GET_VERSION_STRING:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)FITTLE_VERSION);
				break;
			case WM_F4B24_IPC_GET_VERSION_STRING2:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)TEXT("f4b24 - ") F4B24_VERSION_STRING);
				break;
			case WM_F4B24_IPC_GET_SUPPORT_LIST:
				SendSupportList((HWND)lp);
				break;
			case WM_F4B24_IPC_GET_PLAYING_PATH:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)g_cInfo[g_bNow].szFilePath);
				break;
			case WM_F4B24_IPC_GET_PLAYING_TITLE:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_taginfo.szTitle);
				break;
			case WM_F4B24_IPC_GET_PLAYING_ARTIST:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_taginfo.szArtist);
				break;
			case WM_F4B24_IPC_GET_PLAYING_ALBUM:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_taginfo.szAlbum);
				break;
			case WM_F4B24_IPC_GET_PLAYING_TRACK:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_taginfo.szTrack);
				break;

			case WM_F4B24_IPC_SETTING:
				ShowSettingDialog(hWnd, lp);
				break;

			case WM_F4B24_IPC_GET_CAPABLE:
				if (lp == WM_F4B24_IPC_GET_CAPABLE_LP_FLOATOUTPUT){
					return OPIsSupportFloatOutput() ? WM_F4B24_IPC_GET_CAPABLE_RET_SUPPORTED : WM_F4B24_IPC_GET_CAPABLE_RET_NOT_SUPPORTED;
				}
				break;
			
			case WM_F4B24_IPC_GET_CURPATH:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_szTreePath);
				break;
			case WM_F4B24_IPC_SET_CURPATH:
				if(g_cfg.nTreeState == TREE_SHOW){
					TCHAR szWorkPath[MAX_FITTLE_PATH];
					SendMessage((HWND)lp, WM_GETTEXT, (WPARAM)MAX_FITTLE_PATH, (LPARAM)szWorkPath);
					MakeTreeFromPath(m_hTree, m_hCombo, szWorkPath);
				}else{
					SendMessage((HWND)lp, WM_GETTEXT, (WPARAM)MAX_FITTLE_PATH, (LPARAM)m_szTreePath);
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FILEREVIEW, 0), 0);
				}
				break;

			case WM_F4B24_HOOK_REPLAY_GAIN:
				return 0;
			}
			break;

		case WM_DEVICECHANGE:	// ドライブの数の変更に対応
			switch (wp)
			{
				case 0x8000://DBT_DEVICEARRIVAL:
				case 0x8004://DBT_DEVICEREMOVECOMPLETE:
					SendMessage(hWnd, WM_F4B24_IPC, (WPARAM)WM_F4B24_IPC_UPDATE_DRIVELIST, (LPARAM)0);
					//SetDrivesToCombo(m_hCombo);
					break;
			}
			break;

		default:
			// タスクトレイが再描画されたらアイコンも再描画
			if((msg == s_uTaskbarRestart) && m_bTrayFlag){
				SetTaskTray(hWnd);
			}
			return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0L;
}

static void DoTrayClickAction(HWND hWnd, int nKind){
	switch(g_cfg.nTrayClick[nKind]){
		case 1:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_PLAY, 0);
			break;
		case 2:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_PAUSE, 0);
			break;
		case 3:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_STOP, 0);
			break;
		case 4:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_PREV, 0);
			break;
		case 5:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_NEXT, 0);
			break;
		case 6:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_TRAY_WINVIEW, 0);
			break;
		case 7:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_END, 0);
			break;
		case 8:
			PopupTrayMenu(hWnd);
			break;
		case 9:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_PLAYPAUSE, 0);
			break;

	}
	return;
}

static void PopupTrayMenu(HWND hWnd){
	POINT pt;

	GetCursorPos(&pt);
	SetForegroundWindow(hWnd);
	TrackPopupMenu(m_hTrayMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
	PostMessage(hWnd, WM_NULL, 0, 0);
	return;
}

static void PopupPlayModeMenu(HWND hWnd, NMTOOLBAR *lpnmtb){
	RECT rc;
	MENUITEMINFO mii;
	HMENU hPopMenu;

	SendMessage(hWnd, TB_GETRECT, (WPARAM)lpnmtb->iItem, (LPARAM)&rc);
	MapWindowPoints(hWnd, HWND_DESKTOP, (LPPOINT)&rc, 2);
	hPopMenu = CreatePopupMenu();
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.cch = 100;
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	mii.fType = MFT_STRING;
	mii.dwTypeData = TEXT("リスト(&L)");
	mii.wID = IDM_PM_LIST;
	if(m_nPlayMode==PM_LIST) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, 0, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = TEXT("ランダム(&R)");
	mii.wID = IDM_PM_RANDOM;
	if(m_nPlayMode==PM_RANDOM) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, IDM_PM_LIST, TRUE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = TEXT("シングル(&S)");
	mii.wID = IDM_PM_SINGLE;
	if(m_nPlayMode==PM_SINGLE) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, IDM_PM_RANDOM, TRUE, &mii);
	mii.fState = MFS_UNCHECKED;
	TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, rc.left, rc.bottom, 0, hWnd, NULL);
	DestroyMenu(hPopMenu);
	return;
}

// グローバルな設定を読み込む
static void LoadConfig(){
	int i;
	TCHAR szSec[10];

	// コントロールカラー
	g_cfg.nBkColor = (int)GetPrivateProfileInt(TEXT("Color"), TEXT("BkColor"), (int)GetSysColor(COLOR_WINDOW), m_szINIPath);
	g_cfg.nTextColor = (int)GetPrivateProfileInt(TEXT("Color"), TEXT("TextColor"), (int)GetSysColor(COLOR_WINDOWTEXT), m_szINIPath);
	g_cfg.nPlayTxtCol = (int)GetPrivateProfileInt(TEXT("Color"), TEXT("PlayTxtCol"), (int)RGB(0xFF, 0, 0), m_szINIPath);
	g_cfg.nPlayBkCol = (int)GetPrivateProfileInt(TEXT("Color"), TEXT("PlayBkCol"), (int)RGB(230, 234, 238), m_szINIPath);
	// 表示方法
	g_cfg.nPlayView = GetPrivateProfileInt(TEXT("Color"), TEXT("PlayView"), 1, m_szINIPath);

	// システムの優先度の設定
	g_cfg.nHighTask = GetPrivateProfileInt(TEXT("Main"), TEXT("Priority"), 0, m_szINIPath);
	// グリッドライン
	g_cfg.nGridLine = GetPrivateProfileInt(TEXT("Main"), TEXT("GridLine"), 1, m_szINIPath);
	g_cfg.nSingleExpand = GetPrivateProfileInt(TEXT("Main"), TEXT("SingleExp"), 0, m_szINIPath);
	// 存在確認
	g_cfg.nExistCheck = GetPrivateProfileInt(TEXT("Main"), TEXT("ExistCheck"), 0, m_szINIPath);
	// プレイリストで更新日時を取得する
	g_cfg.nTimeInList = GetPrivateProfileInt(TEXT("Main"), TEXT("TimeInList"), 0, m_szINIPath);
	// システムイメージリストを結合
	g_cfg.nTreeIcon = GetPrivateProfileInt(TEXT("Main"), TEXT("TreeIcon"), TRUE, m_szINIPath);
	// タスクトレイに収納のチェック
	g_cfg.nTrayOpt = GetPrivateProfileInt(TEXT("Main"), TEXT("Tray"), 1, m_szINIPath);
	// ツリー表示設定
	g_cfg.nHideShow = GetPrivateProfileInt(TEXT("Main"), TEXT("HideShow"), 0, m_szINIPath);
	// タブを下に表示する
	g_cfg.nTabBottom = GetPrivateProfileInt(TEXT("Main"), TEXT("TabBottom"), 0, m_szINIPath);
	// 多段で表示する
	g_cfg.nTabMulti = GetPrivateProfileInt(TEXT("Main"), TEXT("TabMulti"), 1, m_szINIPath);
	// すべてのフォルダが～
	g_cfg.nAllSub = GetPrivateProfileInt(TEXT("Main"), TEXT("AllSub"), 0, m_szINIPath);
	// ツールチップでフルパスを表示
	g_cfg.nPathTip = GetPrivateProfileInt(TEXT("Main"), TEXT("PathTip"), 1, m_szINIPath);
	// 曲名お知らせ機能
	g_cfg.nInfoTip = GetPrivateProfileInt(TEXT("Main"), TEXT("Info"), 1, m_szINIPath);
	// タグを反転
	g_cfg.nTagReverse = GetPrivateProfileInt(TEXT("Main"), TEXT("TagReverse"), 0, m_szINIPath);
	// ヘッダコントロールを表示する
	g_cfg.nShowHeader = GetPrivateProfileInt(TEXT("Main"), TEXT("ShowHeader"), 1, m_szINIPath);
	// シーク量
	g_cfg.nSeekAmount = GetPrivateProfileInt(TEXT("Main"), TEXT("SeekAmount"), 5, m_szINIPath);
	// 音量変化量(隠し設定)
	g_cfg.nVolAmount = GetPrivateProfileInt(TEXT("Main"), TEXT("VolAmount"), 5, m_szINIPath);
	// 終了時に再生していた曲を起動時にも再生する
	g_cfg.nResume = GetPrivateProfileInt(TEXT("Main"), TEXT("Resume"), 0, m_szINIPath);
	// 終了時の再生位置も記録復元する
	g_cfg.nResPosFlag = GetPrivateProfileInt(TEXT("Main"), TEXT("ResPosFlag"), 0, m_szINIPath);
	// 閉じるボタンで最小化する
	g_cfg.nCloseMin = GetPrivateProfileInt(TEXT("Main"), TEXT("CloseMin"), 0, m_szINIPath);
	// サブフォルダを検索で圧縮ファイルも検索する
	g_cfg.nZipSearch = GetPrivateProfileInt(TEXT("Main"), TEXT("ZipSearch"), 0, m_szINIPath);
	// タブが一つの時はタブを隠す
	g_cfg.nTabHide = GetPrivateProfileInt(TEXT("Main"), TEXT("TabHide"), 0, m_szINIPath);
	// 音声出力デバイス
	g_cfg.nOutputDevice = GetPrivateProfileInt(TEXT("Main"), TEXT("OutputDevice"), 0, m_szINIPath);
	// 32bit(float)で出力する
	g_cfg.nOut32bit = GetPrivateProfileInt(TEXT("Main"), TEXT("Out32bit"), 0, m_szINIPath);
	// 停止時にフェードアウトする
	g_cfg.nFadeOut = GetPrivateProfileInt(TEXT("Main"), TEXT("FadeOut"), 1, m_szINIPath);
	// ReplayGainの適用方法
	g_cfg.nReplayGainMode = GetPrivateProfileInt(TEXT("ReplayGain"), TEXT("Mode"), 1, m_szINIPath);
	// 音量増幅方法
	g_cfg.nReplayGainMixer = GetPrivateProfileInt(TEXT("ReplayGain"), TEXT("Mixer"), 1, m_szINIPath);
	// PreAmp(%)
	g_cfg.nReplayGainPreAmp = GetPrivateProfileInt(TEXT("ReplayGain"), TEXT("PreAmp"), 100, m_szINIPath);
	// PostAmp(%)
	g_cfg.nReplayGainPostAmp = GetPrivateProfileInt(TEXT("ReplayGain"), TEXT("PostAmp"), 100, m_szINIPath);
	// スタートアップフォルダ読み込み
	GetPrivateProfileString(TEXT("Main"), TEXT("StartPath"), TEXT(""), g_cfg.szStartPath, MAX_FITTLE_PATH, m_szINIPath);
	// ファイラのパス
	GetPrivateProfileString(TEXT("Main"), TEXT("FilerPath"), TEXT(""), g_cfg.szFilerPath, MAX_FITTLE_PATH, m_szINIPath);

	// ホットキーの設定
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintf(szSec, TEXT("HotKey%d"), i);
		g_cfg.nHotKey[i] = GetPrivateProfileInt(TEXT("HotKey"), szSec, 0, m_szINIPath);
	}

	// クリック時の動作
	g_cfg.nTrayClick[0] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click0"), 6, m_szINIPath);
	g_cfg.nTrayClick[1] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click1"), 0, m_szINIPath);
	g_cfg.nTrayClick[2] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click2"), 8, m_szINIPath);
	g_cfg.nTrayClick[3] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click3"), 0, m_szINIPath);
	g_cfg.nTrayClick[4] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click4"), 5, m_szINIPath);
	g_cfg.nTrayClick[5] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click5"), 0, m_szINIPath);

	// フォント設定読み込み
	GetPrivateProfileString(TEXT("Font"), TEXT("FontName"), TEXT(""), g_cfg.szFontName, 32, m_szINIPath);	// フォント名""がデフォルトの印
	g_cfg.nFontHeight = GetPrivateProfileInt(TEXT("Font"), TEXT("Height"), 10, m_szINIPath);
	g_cfg.nFontStyle = GetPrivateProfileInt(TEXT("Font"), TEXT("Style"), 0, m_szINIPath);

	// 外部ツール
	GetPrivateProfileString(TEXT("Tool"), TEXT("Path0"), TEXT(""), g_cfg.szToolPath, MAX_FITTLE_PATH, m_szINIPath);
	return;
}

/* 終了時の状態を読み込む */
static void LoadState(){
	// ミニパネル表示状態
	g_cfg.nMiniPanelEnd = (int)GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("End"), 0, m_szINIPath);
	// ツリーの幅を設定
	g_cfg.nTreeWidth = GetPrivateProfileInt(TEXT("Main"), TEXT("TreeWidth"), 200, m_szINIPath);
	//g_cfg.nTWidthSub = GetPrivateProfileInt(TEXT("Main"), TEXT("TWidthSub"), 200, m_szINIPath);
	g_cfg.nTreeState = GetPrivateProfileInt(TEXT("Main"), TEXT("TreeState"), TREE_SHOW, m_szINIPath);
	// ステータスバー表示非表示
	g_cfg.nShowStatus =  GetPrivateProfileInt(TEXT("Main"), TEXT("ShowStatus"), 1, m_szINIPath);
	// 終了時の再生位置も記録復元する
	g_cfg.nResPos = GetPrivateProfileInt(TEXT("Main"), TEXT("ResPos"), 0, m_szINIPath);
	// 最後に再生していたファイル
	GetPrivateProfileString(TEXT("Main"), TEXT("LastFile"), TEXT(""), g_cfg.szLastFile, MAX_FITTLE_PATH, m_szINIPath);

	m_hFont = NULL;
}

/* 終了時の状態を保存する */
static void SaveState(HWND hWnd){
	int i;
	TCHAR szLastPath[MAX_FITTLE_PATH];
	TCHAR szSec[10];
	WINDOWPLACEMENT wpl;
	REBARBANDINFO rbbi;

	wpl.length = sizeof(WINDOWPLACEMENT);

	lstrcpyn(szLastPath, m_szTreePath, MAX_FITTLE_PATH);
	// コンパクトモードを考慮しながらウィンドウサイズを保存
	GetWindowPlacement(hWnd, &wpl);
	WritePrivateProfileInt(TEXT("Main"), TEXT("Maximized"), (wpl.showCmd==SW_SHOWMAXIMIZED || wpl.flags & WPF_RESTORETOMAXIMIZED), m_szINIPath);	// 最大化
	WritePrivateProfileInt(TEXT("Main"), TEXT("Top"), wpl.rcNormalPosition.top, m_szINIPath); //ウィンドウ位置Top
	WritePrivateProfileInt(TEXT("Main"), TEXT("Left"), wpl.rcNormalPosition.left, m_szINIPath); //ウィンドウ位置Left
	WritePrivateProfileInt(TEXT("Main"), TEXT("Height"), wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top, m_szINIPath); //ウィンドウ位置Height
	WritePrivateProfileInt(TEXT("Main"), TEXT("Width"), wpl.rcNormalPosition.right - wpl.rcNormalPosition.left, m_szINIPath); //ウィンドウ位置Width
	ShowWindow(hWnd, SW_HIDE);	// 終了を高速化して見せるために非表示

	WritePrivateProfileInt(TEXT("Main"), TEXT("TreeWidth"), g_cfg.nTreeWidth, m_szINIPath); //ツリーの幅
	WritePrivateProfileInt(TEXT("Main"), TEXT("TreeState"), (g_cfg.nTreeState==TREE_SHOW), m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("Mode"), m_nPlayMode, m_szINIPath); //プレイモード
	WritePrivateProfileInt(TEXT("Main"), TEXT("Repeat"), m_nRepeatFlag, m_szINIPath); //リピートモード
	WritePrivateProfileInt(TEXT("Main"), TEXT("Volumes"), (int)SendMessage(m_hVolume, TBM_GETPOS, 0, 0), m_szINIPath); //ボリューム
	WritePrivateProfileInt(TEXT("Main"), TEXT("ShowStatus"), g_cfg.nShowStatus, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("ResPos"), g_cfg.nResPos, m_szINIPath);

	WritePrivateProfileInt(TEXT("Main"), TEXT("TabIndex"), (m_nPlayTab==-1?TabCtrl_GetCurSel(m_hTab):m_nPlayTab), m_szINIPath);	//TabのIndex
	WritePrivateProfileInt(TEXT("Main"), TEXT("MainMenu"), (GetMenu(hWnd)?1:0), m_szINIPath);

	WritePrivateProfileString(TEXT("Main"), TEXT("LastPath"), szLastPath, m_szINIPath); //ラストパス
	WritePrivateProfileString(TEXT("Main"), TEXT("LastFile"), g_cfg.szLastFile, m_szINIPath);

	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("End"), g_cfg.nMiniPanelEnd, m_szINIPath);

	WritePrivateProfileInt(TEXT("Column"), TEXT("Width0"), ListView_GetColumnWidth(GetCurListTab(m_hTab)->hList, 0), m_szINIPath);
	WritePrivateProfileInt(TEXT("Column"), TEXT("Width1"), ListView_GetColumnWidth(GetCurListTab(m_hTab)->hList, 1), m_szINIPath);
	WritePrivateProfileInt(TEXT("Column"), TEXT("Width2"), ListView_GetColumnWidth(GetCurListTab(m_hTab)->hList, 2), m_szINIPath);
	WritePrivateProfileInt(TEXT("Column"), TEXT("Width3"), ListView_GetColumnWidth(GetCurListTab(m_hTab)->hList, 3), m_szINIPath);
	WritePrivateProfileInt(TEXT("Column"), TEXT("Sort"), GetCurListTab(m_hTab)->nSortState, m_szINIPath);

	//　レバーの状態を保存
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE | RBBIM_ID;
	for(i=0;i<BAND_COUNT;i++){
		SendMessage(GetDlgItem(hWnd, ID_REBAR), RB_GETBANDINFO, i, (WPARAM)&rbbi);
		wsprintf(szSec, TEXT("fStyle%d"), i);
		WritePrivateProfileInt(TEXT("Rebar2"), szSec, rbbi.fStyle, m_szINIPath);
		wsprintf(szSec, TEXT("cx%d"), i);
		WritePrivateProfileInt(TEXT("Rebar2"), szSec, rbbi.cx, m_szINIPath);
		wsprintf(szSec, TEXT("wID%d"), i);
		WritePrivateProfileInt(TEXT("Rebar2"), szSec, rbbi.wID, m_szINIPath);
	}

	WritePrivateProfileString(NULL, NULL, NULL, m_szINIPath);

}


// ウィンドウの表示/非表示をトグル
static void ToggleWindowView(HWND hWnd){
	if(IsIconic(hWnd) || !IsWindowVisible(hWnd)){
		if(m_bTrayFlag && g_cfg.nTrayOpt==1){
			Shell_NotifyIcon(NIM_DELETE, &m_ni);
			m_bTrayFlag = FALSE;
		}
		SendMessage(m_hTitleTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, NULL);
		KillTimer(hWnd, ID_TIPTIMER);
		ShowWindow(hWnd, SW_SHOW);
		ShowWindow(hWnd, SW_RESTORE);
		//SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
		//SetForegroundWindow(hWnd);
	}else{
		ShowWindow(hWnd, SW_MINIMIZE);
	}
	return;
}

static void AddType(LPTSTR lpszType){
	LPTSTR e = StrStr(lpszType, TEXT("."));
	AddTypelist(e ? e + 1 : lpszType);
}
static void AddTypes(LPCSTR lpszTypes) {
	TCHAR szTemp[MAX_FITTLE_PATH] = {0};
	LPTSTR q = szTemp;
	lstrcpynAt(q, lpszTypes, MAX_FITTLE_PATH);
	while(*q){
		LPTSTR p = StrStr(q, TEXT(";"));
		if(p){
			*p = TEXT('\0');
			AddType(q);
			q = p + 1;
		}else{
			AddType(q);
			break;
		}
	}
}

static BOOL CALLBACK FileProc(LPCTSTR lpszPath, HWND hWnd){
#ifdef UNICODE
	HPLUGIN hPlugin = BASS_PluginLoad((LPSTR)lpszPath, BASS_UNICODE);
#else
	HPLUGIN hPlugin = BASS_PluginLoad(lpszPath, 0);
#endif
	if(hPlugin){
		const BASS_PLUGININFO *info = BASS_PluginGetInfo(hPlugin);
		for(DWORD d=0;d<info->formatc;d++){
			AddTypes(info->formats[d].exts);
		}
	}
	return TRUE;
}

// DLL郡初期化、対応形式決定
static void InitFileTypes(){
	ClearTypelist();
	AddTypes("mp3;mp2;mp1;wav;ogg;aif;aiff");
	AddTypes("mod;mo3;xm;it;s3m;mtm;umx");

	EnumFiles(NULL, TEXT(""), TEXT("bass*.dll"), FileProc, 0);
}

static int XATOI(LPCTSTR p){
	int r = 0;
	while (*p == TEXT(' ') || *p == TEXT('\t')) p++;
	while (*p >= TEXT('0') && *p <= TEXT('9')){
		r = r * 10 + (*p - TEXT('0'));
		p++;
	}
	return r;
}

/* xx:xx:xxという表記とハンドルから時間をQWORDで取得 */
static QWORD GetByteFromSecStr(HCHANNEL hChan, LPTSTR pszSecStr){
	int min, sec, frame;

	if(*pszSecStr == TEXT('\0')){
		return BASS_ChannelGetLength(hChan, BASS_POS_BYTE);
	}

	min = XATOI(pszSecStr);
	sec = 0;
	frame = 0;

	while (*pszSecStr && *pszSecStr != TEXT(':')) pszSecStr++;

	if (*pszSecStr){
		sec = XATOI(++pszSecStr);
		while (*pszSecStr && *pszSecStr != TEXT(':')) pszSecStr++;
		if (*pszSecStr){
			frame = XATOI(++pszSecStr);
		}
	}

	return BASS_ChannelSeconds2Bytes(hChan, (float)((min * 60 + sec) * 75 + frame) / 75.0);
}

// ファイルをオープン、構造体を設定
static BOOL SetChannelInfo(BOOL bFlag, struct FILEINFO *pInfo){
	TCHAR szFilePath[MAX_FITTLE_PATH];
	QWORD qStart, qEnd;
	TCHAR szStart[100], szEnd[100];
	szStart[0] = 0;
	szEnd[0] = 0;

	lstrcpyn(g_cInfo[bFlag].szFilePath, pInfo->szFilePath, MAX_FITTLE_PATH);
	g_cInfo[bFlag].pBuff = 0;
	g_cInfo[bFlag].qStart = 0;

	if(IsArchivePath(pInfo->szFilePath)){
		AnalyzeArchivePath(&g_cInfo[bFlag], szFilePath, szStart, szEnd);
	}else{
		lstrcpyn(szFilePath, pInfo->szFilePath, MAX_FITTLE_PATH);
		g_cInfo[bFlag].qDuration = 0;
	}

	g_cInfo[bFlag].sGain = 1;
	g_cInfo[bFlag].sAmp = 0;
	g_cInfo[bFlag].hChan = BASS_StreamCreateFile((BOOL)g_cInfo[bFlag].pBuff,
												(g_cInfo[bFlag].pBuff?(void *)g_cInfo[bFlag].pBuff:(void *)szFilePath),
												0, (DWORD)g_cInfo[bFlag].qDuration,
#ifdef UNICODE
												(g_cInfo[bFlag].pBuff?0:BASS_UNICODE) |
#endif
												BASS_STREAM_DECODE | m_bFloat*BASS_SAMPLE_FLOAT);
	if(!g_cInfo[bFlag].hChan){
	g_cInfo[bFlag].hChan = BASS_MusicLoad((BOOL)g_cInfo[bFlag].pBuff,
												(g_cInfo[bFlag].pBuff?(void *)g_cInfo[bFlag].pBuff:(void *)szFilePath),
												0, (DWORD)g_cInfo[bFlag].qDuration,
#ifdef UNICODE
												(g_cInfo[bFlag].pBuff?0:BASS_UNICODE) |
#endif
												BASS_MUSIC_DECODE | m_bFloat*BASS_SAMPLE_FLOAT | BASS_MUSIC_PRESCAN, 0);
	}
	if(g_cInfo[bFlag].hChan){
		if(!IsArchivePath(pInfo->szFilePath) || !GetArchiveGain(pInfo->szFilePath, &g_cInfo[bFlag].sGain, g_cInfo[bFlag].hChan)){
			DWORD dwGain = SendMessage(GetParent(m_hStatus), WM_F4B24_IPC, WM_F4B24_HOOK_REPLAY_GAIN, (LPARAM)g_cInfo[bFlag].hChan);
			g_cInfo[bFlag].sGain = dwGain ? *(float *)&dwGain : (float)1;
		}
		SendMessage(GetParent(m_hStatus), WM_F4B24_IPC, WM_F4B24_HOOK_CREATE_DECODE_STREAM, (LPARAM)g_cInfo[bFlag].hChan);
		if(szStart[0]){
			qStart = GetByteFromSecStr(g_cInfo[bFlag].hChan, szStart);
			qEnd = GetByteFromSecStr(g_cInfo[bFlag].hChan, szEnd);
			BASS_ChannelSetPosition(g_cInfo[bFlag].hChan, qStart, BASS_POS_BYTE);
			g_cInfo[bFlag].qStart = qStart;
			g_cInfo[bFlag].qDuration = qEnd - qStart;
			BASS_ChannelSetSync(g_cInfo[bFlag].hChan, BASS_SYNC_POS, qStart + (g_cInfo[bFlag].qDuration)*99/100, &EventSync, 0);
			BASS_ChannelSetSync(g_cInfo[bFlag].hChan, BASS_SYNC_POS, qEnd, &EventSync, (void *)1);
		}else{
			g_cInfo[bFlag].qDuration = BASS_ChannelGetLength(g_cInfo[bFlag].hChan, BASS_POS_BYTE);
			if(g_cInfo[bFlag].qDuration==-1){
				g_cInfo[bFlag].qDuration = 0;
			}
			// 曲を99%再生後、イベントが起こるように。
			if(lstrcmpi(PathFindExtension(szFilePath), TEXT(".CDA"))){
				BASS_ChannelSetSync(g_cInfo[bFlag].hChan, BASS_SYNC_POS, (g_cInfo[bFlag].qDuration)*99/100, &EventSync, 0);
			}
		}
		return TRUE;
	}else{
#ifdef UNICODE
		CHAR szAnsi[MAX_FITTLE_PATH];
		WideCharToMultiByte(CP_ACP, 0, szFilePath, -1, szAnsi, MAX_FITTLE_PATH, NULL, NULL);
		BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 60000); // 5secは短いので60secにする
		g_cInfo[bFlag].hChan = BASS_StreamCreateURL(szAnsi, 0, BASS_STREAM_BLOCK | BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT * m_bFloat, NULL, 0);
#else
		BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 60000); // 5secは短いので60secにする
		g_cInfo[bFlag].hChan = BASS_StreamCreateURL(szFilePath, 0, BASS_STREAM_BLOCK | BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT * m_bFloat, NULL, 0);
#endif
		if(g_cInfo[bFlag].hChan){
			DWORD dwGain = SendMessage(GetParent(m_hStatus), WM_F4B24_IPC, WM_F4B24_HOOK_REPLAY_GAIN, (LPARAM)g_cInfo[bFlag].hChan);
			g_cInfo[bFlag].sGain = dwGain ? *(float *)&dwGain : (float)1;
			SendMessage(GetParent(m_hStatus), WM_F4B24_IPC, WM_F4B24_HOOK_CREATE_DECODE_STREAM, (LPARAM)g_cInfo[bFlag].hChan);
			g_cInfo[bFlag].qDuration = BASS_ChannelGetLength(g_cInfo[bFlag].hChan, BASS_POS_BYTE);
			if(g_cInfo[bFlag].qDuration==-1){
				g_cInfo[bFlag].qDuration = 0;
			}
			BASS_ChannelSetSync(g_cInfo[bFlag].hChan, BASS_SYNC_END, 0, &EventSync, 0);
			return TRUE;
		}
		return FALSE;
	}
}

static void FreeChannelInfo(BOOL bFlag){
	if (g_cInfo[bFlag].hChan){
		BASS_ChannelStop(g_cInfo[bFlag].hChan);
		SendMessage(GetParent(m_hStatus), WM_F4B24_IPC, WM_F4B24_HOOK_FREE_DECODE_STREAM, (LPARAM)g_cInfo[bFlag].hChan);
		if (!BASS_StreamFree(g_cInfo[bFlag].hChan)){
			BASS_MusicFree(g_cInfo[bFlag].hChan);
		}
		g_cInfo[bFlag].hChan = 0;
	}

	if(g_cInfo[bFlag].pBuff){
		HeapFree(GetProcessHeap(), 0, (LPVOID)g_cInfo[bFlag].pBuff);
		g_cInfo[bFlag].pBuff = NULL;
	}
	g_cInfo[bFlag].sGain = 1;
	return;
}


// 指定ファイルを再生
static BOOL PlayByUser(HWND hWnd, struct FILEINFO *pPlayFile){
	BASS_CHANNELINFO info;
//	DWORD d;

	// 再生曲を停止、開放
	StopOutputStream(hWnd);
	if(SetChannelInfo(g_bNow, pPlayFile)){
		// ストリーム作成
		BASS_ChannelGetInfo(g_cInfo[g_bNow].hChan, &info);
		OPStart(&info, SendMessage(m_hVolume, TBM_GETPOS, 0, 0), m_bFloat);

		OnStatusChangePlugins();

		// シークバー用タイマー作成
		SetTimer(hWnd, ID_SEEKTIMER, 500, NULL);

		// 表示切替
		g_pNextFile = pPlayFile;
		m_nGaplessState = GS_OK;
		OnChangeTrack();

		return TRUE;
	}else{
		SetWindowText(hWnd, TEXT("ファイルのオープンに失敗しました！"));
		return FALSE;
	}
}

// 次のファイルをオープンする関数(99％地点で発動)
static void CALLBACK EventSync(HSYNC /*handle*/, DWORD /*channel*/, DWORD /*data*/, void *user){
	if(user==0){
		PostMessage(GetParent(m_hStatus), WM_F4B24_IPC, WM_F4B24_INTERNAL_SETUP_NEXT, 0);
	}else{
		m_bCueEnd = TRUE;
	}
	return;
}

// 表示切替
static void OnChangeTrack(){
	int nPlayIndex;
	TCHAR szShortTag[64] = {0};
	TCHAR szTitleCap[524] = {0};
	BASS_CHANNELINFO info;

	m_bCueEnd = FALSE;

	// 99％までいかなかった場合
	if(m_nGaplessState==GS_NOEND || !g_pNextFile){
		g_pNextFile = SelectNextFile(TRUE);
		if(g_pNextFile){
			SetChannelInfo(g_bNow, g_pNextFile);
		}else{
			StopOutputStream(GetParent(m_hStatus));
			return;
		}
	}

	// リピート終了
	if(!g_pNextFile){
		StopOutputStream(GetParent(m_hStatus));
		return;
	}

	//周波数が変わるときは開きなおす
	BASS_ChannelGetInfo(g_cInfo[g_bNow].hChan, &info);
	if(m_nGaplessState==GS_NEWFREQ){
		OPStop();
		OPStart(&info, SendMessage(m_hVolume, TBM_GETPOS, 0, 0), m_bFloat);
	}else{
		OPSetVolume(SendMessage(m_hVolume, TBM_GETPOS, 0, 0));
	}

	// 切り替わったファイルのインデックスを取得
	nPlayIndex = GetIndexFromPtr(GetPlayListTab(m_hTab)->pRoot, g_pNextFile);

	// 表示を切り替え
	ListView_SingleSelect(GetPlayListTab(m_hTab)->hList, nPlayIndex);
	InvalidateRect(GetCurListTab(m_hTab)->hList, NULL, TRUE); // CUSTOMDRAWイベント発生

	// ファイルのオープンに失敗した
	if(m_nGaplessState==GS_FAILED){
		StopOutputStream(GetParent(m_hStatus));
		SetWindowText(GetParent(m_hStatus), TEXT("ファイルのオープンに失敗しました！"));
		return;
	}

	// 再生済みにする
	g_pNextFile->bPlayFlag = TRUE;
	lstrcpyn(g_cfg.szLastFile, g_pNextFile->szFilePath, MAX_FITTLE_PATH);

	// ステータスクリア
	m_nGaplessState = GS_NOEND;

	// 現在再生曲と違う曲なら再生履歴に追加する
	if(GetPlaying(GetPlayListTab(m_hTab))!=g_pNextFile)
		PushPlaying(GetPlayListTab(m_hTab), g_pNextFile);
	
	// カレントファイルを変更
	GetPlayListTab(m_hTab)->nPlaying = nPlayIndex;


	// タグを
	if(GetArchiveTagInfo(g_cInfo[g_bNow].szFilePath, &m_taginfo)
	|| BASS_TAG_Read(g_cInfo[g_bNow].hChan, &m_taginfo)){
		if(!g_cfg.nTagReverse){
			wsprintf(m_szTag, TEXT("%s / %s"), m_taginfo.szTitle, m_taginfo.szArtist);
		}else{
			wsprintf(m_szTag, TEXT("%s / %s"), m_taginfo.szArtist, m_taginfo.szTitle);
		}
	}else{
		lstrcpyn(m_szTag, GetFileName(g_cInfo[g_bNow].szFilePath), MAX_FITTLE_PATH);
		lstrcpyn(m_taginfo.szTitle, m_szTag, 256);
	}
#ifdef UNICODE
	lstrcpyntA(m_taginfoA.szTitle, m_taginfo.szTitle, 256);
	lstrcpyntA(m_taginfoA.szArtist, m_taginfo.szArtist, 256);
	lstrcpyntA(m_taginfoA.szAlbum, m_taginfo.szAlbum, 256);
	lstrcpyntA(m_taginfoA.szTrack, m_taginfo.szTrack, 10);
#endif

	//タイトルバーの処理
	wsprintf(szTitleCap, TEXT("%s - %s"), m_szTag, FITTLE_TITLE);
	SetWindowText(GetParent(m_hStatus), szTitleCap);

	float time = (float)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, BASS_ChannelGetLength(g_cInfo[g_bNow].hChan, BASS_POS_BYTE)); // playback duration
	DWORD dLen = (DWORD)BASS_StreamGetFilePosition(g_cInfo[g_bNow].hChan, BASS_FILEPOS_END); // file length
	DWORD bitrate;
	if(dLen>0 && time>0){
		bitrate = (DWORD)(dLen/(125*time)+0.5); // bitrate (Kbps)
		wsprintf(szTitleCap, TEXT("%d Kbps  %d Hz  %s"), bitrate, info.freq, (info.chans!=1?TEXT("Stereo"):TEXT("Mono")));
	}else{
		wsprintf(szTitleCap, TEXT("? Kbps  %d Hz  %s"), info.freq, (info.chans!=1?TEXT("Stereo"):TEXT("Mono")));
	}

	//ステータスバーの処理
	if(g_cfg.nTreeIcon){
		SetStatusbarIcon(g_cInfo[g_bNow].szFilePath, TRUE);
	}
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)0|0, (LPARAM)szTitleCap);
	SendMessage(m_hStatus, SB_SETTIPTEXT, (WPARAM)0, (LPARAM)szTitleCap);

	wsprintf(szTitleCap, TEXT("\t%d / %d"), GetPlayListTab(m_hTab)->nPlaying + 1, ListView_GetItemCount(GetPlayListTab(m_hTab)->hList));
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)1|0, (LPARAM)szTitleCap);

	//シークバー
	if(g_cInfo[g_bNow].qDuration>0){
		EnableWindow(m_hSeek, TRUE);
	}else{
		EnableWindow(m_hSeek, FALSE);
	}

	//タスクトレイの処理
	lstrcpyn(szShortTag, m_szTag, 54);
	szShortTag[52] = TEXT('.');
	szShortTag[53] = TEXT('.');
	szShortTag[54] = TEXT('.');
	szShortTag[55] = TEXT('\0');
	wsprintf(m_ni.szTip, TEXT("%s - Fittle"), szShortTag);

	if(m_bTrayFlag)
		Shell_NotifyIcon(NIM_MODIFY, &m_ni); //ToolTipの変更
	if(g_cfg.nInfoTip == 2 || (g_cfg.nInfoTip == 1 && m_bTrayFlag))
		ShowToolTip(GetParent(m_hStatus), m_hTitleTip, m_ni.szTip);

	OnTrackChagePlugins();

	g_pNextFile = NULL;	// 消去
	return;
}

// 次に再生すべきファイルのポインタ取得
static struct FILEINFO *SelectNextFile(BOOL bTimer){
	int nLBCount;
	int nPlayIndex;
	int nTmpIndex;

	nLBCount = ListView_GetItemCount(GetPlayListTab(m_hTab)->hList);
	if(nLBCount<=0){
		return NULL;
	}
	nPlayIndex = GetPlayListTab(m_hTab)->nPlaying;
	switch (m_nPlayMode)
	{
		case PM_LIST:
			nPlayIndex++;
			if(nPlayIndex==nLBCount){
				if(m_nRepeatFlag || !bTimer){
					nPlayIndex = 0;
				}else{
					return NULL;	// リピートがオフ
				}
			}					
			break;

		case PM_RANDOM:
			int nUnPlayCount;
			int nBefore;

			if(nLBCount==1){
				// リストにファイルが一個しかない
				nPlayIndex = 0;
				if(!m_nRepeatFlag && bTimer && GetUnPlayedFile(GetPlayListTab(m_hTab)->pRoot)==0){
					SetUnPlayed(GetPlayListTab(m_hTab)->pRoot);
					return NULL;
				}
			}else{
				nUnPlayCount = GetUnPlayedFile(GetPlayListTab(m_hTab)->pRoot);
				if(nUnPlayCount==0){
					if(m_nRepeatFlag || !bTimer){
						// すべて再生していた場合
						nBefore = GetPlayListTab(m_hTab)->nPlaying;
						nUnPlayCount = SetUnPlayed(GetPlayListTab(m_hTab)->pRoot);
						do{
							nTmpIndex = genrand_int32() % nUnPlayCount;
						}while(nTmpIndex==nBefore);
						nPlayIndex = GetUnPlayedIndex(GetPlayListTab(m_hTab)->pRoot, nTmpIndex);
					}else{
						// リピートがオフ
						SetUnPlayed(GetPlayListTab(m_hTab)->pRoot);
						return NULL;
					}
				}else{
					// 再生途中
					nTmpIndex = genrand_int32() % nUnPlayCount;
					nPlayIndex = GetUnPlayedIndex(GetPlayListTab(m_hTab)->pRoot, nTmpIndex);
				}
			}
			break;

		case PM_SINGLE:
			// lp=1のときはタイマーから。それ以外はユーザー入力
			if(bTimer){
				if(!m_nRepeatFlag){
					return NULL;
				}
				if(GetPlayListTab(m_hTab)->nPlaying==-1)
					nPlayIndex = 0;
			}else{
				nPlayIndex++;
				if(GetPlayListTab(m_hTab)->nPlaying==--nLBCount)
					nPlayIndex = 0;
			}
			break;

	}
	return GetPtrFromIndex(GetPlayListTab(m_hTab)->pRoot, nPlayIndex);
}


#define TB_BTN_NUM 8
#define TB_BMP_NUM 9
// ツールバーの作成
static HWND CreateToolBar(HWND hRebar){
	HIMAGELIST hToolImage;
	HBITMAP hBmp;
	TBBUTTON tbb[] = {
		{0, IDM_PLAY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{1, IDM_PAUSE, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
		{2, IDM_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{3, IDM_PREV, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{4, IDM_NEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
		{5, IDM_PM_TOGGLE, TBSTATE_ENABLED, TBSTYLE_DROPDOWN, 0L, 0},
		{6, IDM_PM_REPEAT, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
		//{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	};
	HWND hToolBar = NULL;

	hToolBar = CreateWindowEx(WS_EX_TOOLWINDOW,
                    TOOLBARCLASSNAME,
                    NULL,
                    WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_TOOLTIPS,// | WS_CLIPSIBLINGS,// | WS_CLIPCHILDREN, 
                    0, 0, 100, 100,
                    hRebar,
                    (HMENU)ID_TOOLBAR, m_hInst, NULL);
	if(!hToolBar) return NULL;
	SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	hBmp = (HBITMAP)LoadImage(m_hInst, TEXT("toolbarex.bmp"), IMAGE_BITMAP, 16*TB_BMP_NUM, 16, LR_LOADFROMFILE | LR_SHARED);
	if(hBmp==NULL)	hBmp = LoadBitmap(m_hInst, (LPTSTR)IDR_TOOLBAR1);
	
	//SendMessage(hToolBar, TB_AUTOSIZE, 0, 0) ;
	
	hToolImage = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, TB_BMP_NUM, 0);
	ImageList_AddMasked(hToolImage, hBmp, RGB(0,255,0)); //緑を背景色に

	DeleteObject(hBmp);
	SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)hToolImage);
	
	SendMessage(hToolBar, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
	SendMessage(hToolBar, TB_ADDBUTTONS, (WPARAM)TB_BTN_NUM, (LPARAM)&tbb);
	return hToolBar;
}

// 千分率でファイルをシーク
static BOOL _BASS_ChannelSetPosition(DWORD handle, int nPos){
	QWORD qPos;
	BOOL bRet;

	int nStatus = OPGetStatus();

	qPos = g_cInfo[g_bNow].qDuration;
	qPos = qPos*nPos/1000 + g_cInfo[g_bNow].qStart;

	OPSetFadeOut(150);

	OPStop();
	bRet = BASS_ChannelSetPosition(handle, qPos, BASS_POS_BYTE);

	OPSetVolume(SendMessage(m_hVolume, TBM_GETPOS, 0, 0));

	OPPlay();

	if(nStatus != OPGetStatus()){
		OnStatusChangePlugins();
	}

	SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	return bRet;
}

static void _BASS_ChannelSeekSecond(DWORD handle, float fSecond, int nSign){
	QWORD qPos;
	QWORD qSeek;
	QWORD qSet;

	qPos = BASS_ChannelGetPosition(handle, BASS_POS_BYTE) - g_cInfo[g_bNow].qStart;
	qSeek = BASS_ChannelSeconds2Bytes(handle, fSecond);
	if(nSign<0 && qPos<qSeek){
		qSet = 0;
	}else if(nSign>0 && qPos+qSeek>g_cInfo[g_bNow].qDuration){
		qSet = g_cInfo[g_bNow].qDuration-1;
	}else{
		qSet = qPos + nSign*qSeek;
	}
	OPStop();
	BASS_ChannelSetPosition(handle, qSet + g_cInfo[g_bNow].qStart, BASS_POS_BYTE);
	OPPlay();

	return;
}

// 再生中なら一時停止、一時停止中なら復帰
static int FilePause(){
	DWORD dMode;

	dMode = OPGetStatus();
	if(dMode==OUTPUT_PLUGIN_STATUS_PLAY){
		OPSetFadeOut(250);
		OPPause();
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(TRUE, 0));
	}else if(dMode==OUTPUT_PLUGIN_STATUS_PAUSE){
		OPPlay();
		OPSetFadeIn(SendMessage(m_hVolume, TBM_GETPOS, 0, 0), 250);
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	}else{
		OPSetVolume(SendMessage(m_hVolume, TBM_GETPOS, 0, 0));
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	}

	// プラグインを呼ぶ
	OnStatusChangePlugins();

	return 0;
}

// アウトプットストリームを停止、表示を初期化
static int StopOutputStream(HWND hWnd){
	OPSetFadeOut(250);

	// ストリームを削除
	KillTimer(hWnd, ID_SEEKTIMER);
	OPStop();
	OPEnd();

	FreeChannelInfo(!g_bNow);
	FreeChannelInfo(g_bNow);

	//スライダバー
	EnableWindow(m_hSeek, FALSE);
	SendMessage(m_hSeek, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);
	//ツールバー
	SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	
	//文字列表示関係
	SetWindowText(hWnd, FITTLE_TITLE);
	lstrcpy(m_ni.szTip, FITTLE_TITLE);
	if(m_bTrayFlag)
		Shell_NotifyIcon(NIM_MODIFY, &m_ni); //ToolTipの変更

	SendMessage(hWnd, WM_TIMER, MAKEWPARAM(ID_TIPTIMER, 0), 0);

	lstrcpy(m_szTag, TEXT(""));
	lstrcpy(m_szTime, TEXT("\t"));

	// ステータスバーの初期化
	SetStatusbarIcon(NULL, FALSE);
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)0|0, (LPARAM)TEXT(""));
	SendMessage(m_hStatus, SB_SETTIPTEXT, (WPARAM)0, (LPARAM)TEXT(""));
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)1|0, (LPARAM)TEXT(""));
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)2|0, (LPARAM)TEXT(""));
	// リストビューを再描画
	InvalidateRect(GetCurListTab(m_hTab)->hList, NULL, TRUE); 

	// プラグインを呼ぶ
	OnStatusChangePlugins();

	return 0;
}

// プレイモードを制御する関数
static int ControlPlayMode(HMENU hMenu, int nMode){
	MENUITEMINFO mii;
	int i = 0;

	if(nMode>PM_SINGLE) nMode = PM_LIST;
	m_nPlayMode = nMode;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	if(i++==nMode){
		mii.fState = MFS_CHECKED;
		SendMessage(m_hToolBar,  TB_CHANGEBITMAP, (WPARAM)IDM_PM_TOGGLE, (LPARAM)MAKELONG(5, 0));
	}
	SetMenuItemInfo(hMenu, IDM_PM_LIST, FALSE, &mii);
	SetMenuItemInfo(m_hTrayMenu, IDM_PM_LIST, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	if(i++==nMode){
		mii.fState = MFS_CHECKED;
		SendMessage(m_hToolBar,  TB_CHANGEBITMAP, (WPARAM)IDM_PM_TOGGLE, (LPARAM)MAKELONG(7, 0));
	}
	SetMenuItemInfo(hMenu, IDM_PM_RANDOM, FALSE, &mii);
	SetMenuItemInfo(m_hTrayMenu, IDM_PM_RANDOM, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	if(i++==nMode){
		mii.fState = MFS_CHECKED;
		SendMessage(m_hToolBar,  TB_CHANGEBITMAP, (WPARAM)IDM_PM_TOGGLE, (LPARAM)MAKELONG(8, 0));
	}
	SetMenuItemInfo(hMenu, IDM_PM_SINGLE, FALSE, &mii);
	SetMenuItemInfo(m_hTrayMenu, IDM_PM_SINGLE, FALSE, &mii);
	InvalidateRect(m_hToolBar, NULL, TRUE);
	return 0;
}

// リピートモードを切り替え
static BOOL ToggleRepeatMode(HMENU hMenu){
	MENUITEMINFO mii;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	if(m_nRepeatFlag){
		m_nRepeatFlag = FALSE;
		mii.fState = MFS_UNCHECKED;
		SendMessage(m_hToolBar, TB_CHECKBUTTON, (WPARAM)IDM_PM_REPEAT, (LPARAM)MAKELONG(FALSE, 0));
	}else{
		m_nRepeatFlag = TRUE;
		mii.fState = MFS_CHECKED;
		SendMessage(m_hToolBar, TB_CHECKBUTTON, (WPARAM)IDM_PM_REPEAT, (LPARAM)MAKELONG(TRUE, 0));
	}
	SetMenuItemInfo(hMenu, IDM_PM_REPEAT, FALSE, &mii);
	SetMenuItemInfo(m_hTrayMenu, IDM_PM_REPEAT, FALSE, &mii);
	return TRUE;
}

// システムトレイに入れる関数
static int SetTaskTray(HWND hWnd){
	static HICON hIcon = NULL;

	if(!hIcon){
		hIcon = (HICON)LoadImage(m_hInst, TEXT("MYICON"), IMAGE_ICON, 16, 16, LR_SHARED);
	}

	m_bTrayFlag = TRUE;
	m_ni.cbSize = sizeof(m_ni);
	m_ni.hWnd = hWnd;
	m_ni.uID = 1;
	m_ni.uCallbackMessage = WM_TRAY;
	m_ni.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_ni.hIcon = hIcon;
	Shell_NotifyIcon(NIM_ADD, &m_ni);
	return 0;
}

// ツールチップを表示する関数
static int ShowToolTip(HWND hWnd, HWND hTitleTip, LPTSTR pszToolText){
	TOOLINFO tin;
	RECT rcTask, rcTip;
	HWND hTaskBar;
	POINT ptDraw;

	//タスクバーの位置、高さを取得
	hTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
	if(hTaskBar==NULL) return 0;
	GetWindowRect(hTaskBar, &rcTask);
	//ツールチップを作り、表示
	ZeroMemory(&tin, sizeof(tin));
	tin.cbSize = sizeof(tin);
	tin.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	tin.hwnd = NULL;//hWnd;
	tin.lpszText = pszToolText;
	SendMessage(hTitleTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&tin);
	SendMessage(hTitleTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&tin);
	SendMessage(hTitleTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&tin);
	//デスクトップ作業領域の隅に移動
	GetWindowRect(hTitleTip, &rcTip);
	ptDraw.x = rcTask.right - (rcTip.right - rcTip.left) - 1;
	// タスクバーの位置
	if(rcTask.top==0){
		if((rcTask.right-rcTask.left)>=(rcTask.bottom-rcTask.top)){
			ptDraw.y = rcTask.bottom - 2; // 上にある
		}else{
			ptDraw.y = rcTask.bottom - (rcTip.bottom - rcTip.top);
			if(rcTask.left==0){	// 左にある
				ptDraw.x = 0;
			}
		}
	}else{
		ptDraw.y = rcTask.top - (rcTip.bottom - rcTip.top) + 2; // 下にある
	}
	SendMessage(hTitleTip, TTM_TRACKPOSITION, 0, (LPARAM)(DWORD)MAKELONG(ptDraw.x, ptDraw.y));
	//手前に表示
	SetWindowPos(hTitleTip, HWND_TOPMOST, 0, 0, 0, 0,
		/*SWP_SHOWWINDOW | */SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	//6秒後消します
	SetTimer(hWnd, ID_TIPTIMER, 6000, NULL);
	return 0;
}

static int RegHotKey(HWND hWnd){
	UINT uMod;
	int i;

	for(i=0;i<HOTKEY_COUNT;i++){
		uMod = 0;
		if(HIBYTE(g_cfg.nHotKey[i]) & HOTKEYF_ALT)
			uMod = MOD_ALT;
		if(HIBYTE(g_cfg.nHotKey[i]) & HOTKEYF_SHIFT)
			uMod = uMod | MOD_SHIFT;
		if(HIBYTE(g_cfg.nHotKey[i]) & HOTKEYF_CONTROL)
			uMod = uMod | MOD_CONTROL;
		RegisterHotKey(hWnd, i, uMod, LOBYTE(g_cfg.nHotKey[i]));
	}
	return 0;
}

static int UnRegHotKey(HWND hWnd){
	int i;
	for(i=0;i<HOTKEY_COUNT;i++) UnregisterHotKey(hWnd, i);
	return 0;
}

// 対応ファイルかどうか拡張子でチェック
BOOL CheckFileType(LPTSTR szFilePath){
	int i=0;
	LPTSTR szCheckType;
	LPTSTR lpExt;

	szCheckType = PathFindExtension(szFilePath);
	if(!szCheckType || !*szCheckType) return FALSE;
	szCheckType++;
	do {
		lpExt = GetTypelist(i++);
		if (!lpExt[0]) return FALSE;
	} while(lstrcmpi(lpExt, szCheckType) != 0);
	return TRUE;
}

// コマンドラインオプションを考慮したExecute
static void MyShellExecute(HWND hWnd, LPTSTR pszExePath, LPTSTR pszFilePathes, BOOL bMulti){
	LPTSTR pszArgs = PathGetArgs(pszExePath);

	int nSize = lstrlen(pszArgs) + lstrlen(pszFilePathes) + 5;

	LPTSTR pszBuff = (LPTSTR)HZAlloc(sizeof(TCHAR) * nSize);

	if(!bMulti){
		PathQuoteSpaces(pszFilePathes);
	}
	if(*pszArgs){	// コマンドラインオプションを考慮
		*(pszArgs-1) = TEXT('\0');
		wsprintf(pszBuff, TEXT("%s %s"), pszArgs, pszFilePathes);
	}else{
		wsprintf(pszBuff, TEXT("%s"), pszFilePathes);
	}
	if(!bMulti){
		PathUnquoteSpaces(pszFilePathes);
	}
	ShellExecute(hWnd, TEXT("open"), pszExePath, pszBuff, NULL, SW_SHOWNORMAL);
	if(*pszArgs){	// 戻そう
		*(pszArgs-1) = TEXT(' ');
	}

	HFree(pszBuff);
	return;
}

// リストで選択されたアイテムのパスを連結して返す。どこかでFreeしてください。
static LPTSTR MallocAndConcatPath(LISTTAB *pListTab){
	DWORD s = sizeof(TCHAR) * 2;
	LPTSTR pszRet = (LPTSTR)HZAlloc(s);
	LPTSTR pszNew;

	int i = -1;
	while( (i = ListView_GetNextItem(pListTab->hList, i, LVNI_SELECTED)) != -1 ){
		LPTSTR pszTarget = GetPtrFromIndex(pListTab->pRoot, i)->szFilePath;
		if(!IsURLPath(pszTarget) && !IsArchivePath(pszTarget)){	// 削除できないファイルでなければ
			s += (lstrlen(pszTarget) + 3) * sizeof(TCHAR);
			pszNew = (LPTSTR)HRealloc(pszRet, s);
			if (!pszNew) {
				HFree(pszRet);
				return NULL;
			}
			pszRet = pszNew;
			lstrcat(pszRet, TEXT("\""));
			lstrcat(pszRet, pszTarget);
			lstrcat(pszRet, TEXT("\" "));
		}
	}
	return pszRet;
}

// ファイルを保存ダイアログを出す
static int SaveFileDialog(LPTSTR szDir, LPTSTR szDefTitle){
	TCHAR szFile[MAX_FITTLE_PATH];
	TCHAR szFileTitle[MAX_FITTLE_PATH];
	OPENFILENAME ofn;

	lstrcpyn(szFile, szDefTitle, MAX_FITTLE_PATH);
	lstrcpyn(szFileTitle, szDefTitle, MAX_FITTLE_PATH);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = 
		TEXT("プレイリスト(絶対パス) (*.m3u)\0*.m3u\0")
		TEXT("プレイリスト(相対パス) (*.m3u)\0*.m3u\0")
		TEXT("UTF8プレイリスト(絶対パス) (*.m3u8)\0*.m3u8\0")
		TEXT("UTF8プレイリスト(相対パス) (*.m3u8)\0*.m3u8\0")
		TEXT("すべてのファイル(*.*)\0*.*\0\0");
	ofn.lpstrFile = szFile;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.lpstrInitialDir = szDir;
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = MAX_FITTLE_PATH;
	ofn.nMaxFileTitle = MAX_FITTLE_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = TEXT("m3u");
	ofn.lpstrTitle = TEXT("名前を付けて保存する");
	if(GetSaveFileName(&ofn)==0) return 0;
	lstrcpyn(szDir, szFile, MAX_FITTLE_PATH);
	return ofn.nFilterIndex;
}

// ドラッグイメージ作成、表示
static void OnBeginDragList(HWND hLV, POINT pt){
	m_hDrgImg = ListView_CreateDragImage(hLV, ListView_GetNextItem(hLV, -1, LVNI_SELECTED), &pt);
	ImageList_BeginDrag(m_hDrgImg, 0, 0, 0);
	ImageList_DragEnter(hLV, pt.x, pt.y);
	SetCapture(hLV);
	return;
}

// ドラッグイメージを動かしながらLVIS_DROPHILITEDを表示
static void OnDragList(HWND hLV, POINT pt){
	LVHITTESTINFO pinfo;
	int nHitItem;
	RECT rcLV;

	pinfo.pt.x = pt.x;
	pinfo.pt.y = pt.y;
	ImageList_DragShowNolock(FALSE);
	ListView_SetItemState(hLV, -1, 0, LVIS_DROPHILITED);
	nHitItem = ListView_HitTest(hLV, &pinfo);
	if(nHitItem!=-1)	ListView_SetItemState(hLV, nHitItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
	UpdateWindow(hLV);
	ImageList_DragShowNolock(TRUE);
	ImageList_DragMove(pt.x, pt.y);

	GetClientRect(hLV, &rcLV);
	if(pt.y >=0 && pt.y <= GetSystemMetrics(SM_CYHSCROLL)){
		//上スクロール
		SetTimer(GetParent(hLV), ID_SCROLLTIMERUP, 100, NULL);
	}else if(pt.y >= rcLV.bottom - GetSystemMetrics(SM_CYHSCROLL)){
		//下スクロール
		SetTimer(GetParent(hLV), ID_SCROLLTIMERDOWN, 100, NULL);
	}else{
		KillTimer(GetParent(hLV), ID_SCROLLTIMERUP);
		KillTimer(GetParent(hLV), ID_SCROLLTIMERDOWN);
	}
}

// ドラッグイメージ削除
static void OnEndDragList(HWND hLV){
	KillTimer(GetParent(hLV), ID_SCROLLTIMERUP);
	KillTimer(GetParent(hLV), ID_SCROLLTIMERDOWN);
	ListView_SetItemState(hLV, -1, 0, LVIS_DROPHILITED);
	ImageList_DragLeave(hLV);
	ImageList_EndDrag();
	ImageList_Destroy(m_hDrgImg);
	m_hDrgImg = NULL;
	ReleaseCapture();
}

static void DrawTabFocus(HWND m_hTab, int nIdx, BOOL bFlag){
	RECT rcItem;
	HDC hDC;

	if((bFlag && !GetListTab(m_hTab, nIdx)->bFocusRect)
	|| (!bFlag && GetListTab(m_hTab, nIdx)->bFocusRect)){
		TabCtrl_GetItemRect(m_hTab, nIdx, &rcItem);
		rcItem.left += 1;
		rcItem.top += 1;
		rcItem.right -= 1;
		rcItem.bottom -= 1;
		hDC = GetDC(m_hTab);
		DrawFocusRect(hDC, &rcItem);
		GetListTab(m_hTab, nIdx)->bFocusRect = !GetListTab(m_hTab, nIdx)->bFocusRect;
		ReleaseDC(m_hTab, hDC);
	}
}

static BOOL SetUIFont(void){
	int nPitch;
	HDC hDC;

	// 使っていたフォントを削除
	if(m_hFont) DeleteObject(m_hFont);
	if(m_hBoldFont){
		DeleteObject(m_hBoldFont);
		m_hBoldFont = NULL;
	}
	// フォント作成
	if(g_cfg.szFontName[0] != TEXT('\0')){
		hDC = GetDC(m_hTree);
		nPitch = -MulDiv(g_cfg.nFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		ReleaseDC(m_hTree, hDC);
		m_hFont = CreateFont(nPitch, 0, 0, 0, (g_cfg.nFontStyle&BOLD_FONTTYPE?FW_BOLD:0),
			(DWORD)(g_cfg.nFontStyle&ITALIC_FONTTYPE?TRUE:FALSE), FALSE, FALSE, (DWORD)DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
			g_cfg.szFontName);
		if(!m_hFont) g_cfg.szFontName[0] = TEXT('\0');	// フォントが読み込めない
	}else{
		m_hFont = NULL;
	}
	// フォントを各コントロールに投げる
	SendMessage(m_hCombo, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	if(m_hFont){	// タブは特別
		SendMessage(m_hTab, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	}else{
		SendMessage(m_hTab, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	}
	SendMessage(m_hTree, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	for(int i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
		SendMessage(GetListTab(m_hTab, i)->hList, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	}
	// コンボボックスのサイズ調整
	UpdateWindowSize(GetParent(m_hCombo));
	return TRUE;
}

static void UpdateWindowSize(HWND hWnd){
	RECT rcDisp;

	GetClientRect(hWnd, &rcDisp);
	SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(rcDisp.right, rcDisp.bottom));
	return;
}

static void SetStatusbarIcon(LPTSTR pszPath, BOOL bShow){
	static HICON s_hIcon = NULL;
	SHFILEINFO shfinfo = {0};
	TCHAR szIconPath[MAX_FITTLE_PATH] = {0};

	if(s_hIcon){
		DestroyIcon(s_hIcon);
		s_hIcon = NULL;
	}
	if(bShow){
		s_hIcon = IsArchivePath(pszPath) ? GetArchiveItemIcon(pszPath) : NULL;
		if (!s_hIcon){
			lstrcpyn(szIconPath, pszPath, MAX_FITTLE_PATH);
			SHGetFileInfo(szIconPath, FILE_ATTRIBUTE_NORMAL, &shfinfo, sizeof(shfinfo), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON); 
			s_hIcon = shfinfo.hIcon;
		}
		SendMessage(m_hStatus, SB_SETICON, (WPARAM)0, (LPARAM)s_hIcon);
	}else{
		SendMessage(m_hStatus, SB_SETICON, (WPARAM)0, (LPARAM)NULL);
	}
	return;
}

static BOOL SetUIColor(void){
	HWND hList;
	for(int i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
		hList = GetListTab(m_hTab, i)->hList;
		ListView_SetBkColor(hList, (COLORREF)g_cfg.nBkColor);
		ListView_SetTextBkColor(hList, (COLORREF)g_cfg.nBkColor);
		ListView_SetTextColor(hList, (COLORREF)g_cfg.nTextColor);
	}
	TreeView_SetBkColor(m_hTree, g_cfg.nBkColor);
	TreeView_SetTextColor(m_hTree, g_cfg.nTextColor);
	InitTreeIconIndex(m_hCombo, m_hTree, (BOOL)g_cfg.nTreeIcon);
	InvalidateRect(GetCurListTab(m_hTab)->hList, NULL, TRUE);
	return TRUE;
}

// スライダバーの新しいプロシージャ
static LRESULT CALLBACK NewSliderProc(HWND hSB, UINT msg, WPARAM wp, LPARAM lp){
	int nXPos;
	TOOLINFO tin;

	switch(msg)
	{
		case WM_LBUTTONUP: // 左アップだったらシーク適用
			if(GetCapture()==m_hSeek){
				_BASS_ChannelSetPosition(g_cInfo[g_bNow].hChan, SendMessage(hSB, TBM_GETPOS, 0, 0));
			}
		case WM_RBUTTONUP: // 右アップだったらドラッグキャンセル
			if(GetCapture()==m_hSeek || GetCapture()==m_hVolume){
				ReleaseCapture();
				SendMessage(m_hSliderTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&tin);
			}else if(msg!=WM_LBUTTONUP){
				SendMessage(GetParent(m_hStatus), WM_CONTEXTMENU, (WPARAM)GetParent(hSB), lp);
			}
			return 0;

		case WM_LBUTTONDOWN: //ドラッグモードに移行
			SetCapture(hSB);
			SendMessage(hSB, WM_MOUSEMOVE, wp, lp);
			return 0;

		case WM_MOUSEMOVE:
			int nPos;
			float fLen;
			RECT rcSB, rcTool, rcThumb;
			POINT pt;
			TCHAR szDrawBuff[10];

			//シーク中かつ領域内だったら
			if(GetCapture()==m_hSeek || GetCapture()==m_hVolume){
				// チャンネルのRECT(スライダ相対)を取得
				SendMessage(hSB, TBM_GETCHANNELRECT, 0, (LPARAM)(LPRECT)&rcSB);
				SendMessage(hSB, TBM_GETTHUMBRECT, 0, (LPARAM)(LPRECT)&rcThumb);
				// マウスの位置(スライダ相対)を取得
				nXPos = (short)LOWORD(lp);
				if((short)LOWORD(lp)<rcSB.left) nXPos = rcSB.left;
				if((short)LOWORD(lp)>rcSB.right ) nXPos = rcSB.right ;
				//マウスの下にバーを移動
				nXPos -= rcSB.left + (rcThumb.right - rcThumb.left)/2;
				SendMessage(hSB, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SLIDER_DIVIDED * nXPos / (rcSB.right - rcSB.left - (rcThumb.right - rcThumb.left))));

				nPos = (int)SendMessage(hSB, TBM_GETPOS, 0 ,0);
				if(hSB==m_hSeek){
					//バーの位置から時間を取得
					fLen = (float)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, g_cInfo[g_bNow].qDuration);
					wsprintf(szDrawBuff, TEXT("%02d:%02d"), (nPos * (int)fLen / SLIDER_DIVIDED)/60, (nPos * (int)fLen / SLIDER_DIVIDED)%60);
				}else if(hSB==m_hVolume){
					wsprintf(szDrawBuff, TEXT("%02d%%"), nPos/10);
				}

				//ツールチップを作り、表示
				ZeroMemory(&tin, sizeof(tin));
				tin.cbSize = sizeof(tin);
				tin.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
				tin.hwnd = GetParent(hSB);
				tin.lpszText = szDrawBuff;
				SendMessage(m_hSliderTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&tin);
				SendMessage(m_hSliderTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&tin);
				SendMessage(m_hSliderTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&tin);
				GetWindowRect(m_hSliderTip, &rcTool);
				pt.x = nXPos + rcSB.left - (int)((rcThumb.right - rcThumb.left)*1.5);
				pt.y = rcSB.bottom;
				ClientToScreen(hSB, &pt);
				SendMessage(m_hSliderTip, TTM_TRACKPOSITION, 0,
					(LPARAM)(DWORD)MAKELONG(pt.x,  pt.y + 20));	// スライダの20Pixel下に表示

				if(hSB==m_hVolume){
					OPSetVolume(nPos);
				}

			}
			break;

		case WM_MOUSEWHEEL:
			if(hSB==m_hSeek){
				if((short)HIWORD(wp)<0){
					SendMessage(GetParent(m_hStatus), WM_COMMAND, MAKEWPARAM(IDM_SEEKFRONT, 0), 0);
				}else{
					SendMessage(GetParent(m_hStatus), WM_COMMAND, MAKEWPARAM(IDM_SEEKBACK, 0), 0);
				}
			}else if(hSB==m_hVolume){
				if((short)HIWORD(wp)<0){
					SendMessage(hSB, TBM_SETPOS, TRUE, (WPARAM)SendMessage(hSB, TBM_GETPOS, 0, 0) - SLIDER_DIVIDED/50);
				}else{
					SendMessage(hSB, TBM_SETPOS, TRUE, (WPARAM)SendMessage(hSB, TBM_GETPOS, 0, 0) + SLIDER_DIVIDED/50);
				}
			}
			return 0;

		case WM_HSCROLL:
			return 0; // チルトホイールマウスが欲しい

		case WM_MBUTTONDOWN:	// 中ボタンクリック
			if(hSB==m_hSeek){
				FilePause();	// ポーズにしてみる
			}else if(hSB==m_hVolume){
				SendMessage(hSB, TBM_SETPOS, TRUE, (WPARAM)0);	// ミュートにしてみる
			}
			return 0; // フォーカスを得るのを防ぐ

		case TBM_SETPOS:
			if(hSB==m_hVolume){
				OPSetVolume(lp);
			}
			break;

		default:
			break;
	}
	return CallWindowProc((WNDPROC)(LONG64)GetWindowLongPtr(hSB, GWLP_USERDATA), hSB, msg, wp, lp);
}

static LRESULT CALLBACK NewSplitBarProc(HWND hSB, UINT msg, WPARAM wp, LPARAM lp){
	static int s_nPreX;
	static int s_nMouseState = 0;

	RECT rcDisp;
	POINT pt;

	switch(msg){
		case WM_MOUSEMOVE:
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			if(s_nMouseState==1){
				GetCursorPos(&pt);
				g_cfg.nTreeWidth += pt.x - s_nPreX;
				//g_cfg.nTWidthSub = g_cfg.nTreeWidth;
				UpdateWindowSize(GetParent(hSB));
				s_nPreX = pt.x;
			}
			break;

		case WM_MOUSELEAVE:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			break;

		case WM_LBUTTONDOWN:
			if(g_cfg.nTreeState==TREE_SHOW){
				// マウスの移動範囲を制限
				GetWindowRect(GetParent(hSB), &rcDisp);
				rcDisp.right -= SPLITTER_WIDTH*2;
				rcDisp.left += SPLITTER_WIDTH;
				ClipCursor(&rcDisp);
				// スプリッタドラッグ開始
				s_nMouseState = 1;
				GetCursorPos(&pt);
				s_nPreX = pt.x;
				SetCapture(hSB);
			}
			break;

		case WM_LBUTTONUP:
			if(g_cfg.nTreeState==TREE_SHOW){
				// スプリッタドラッグ終了
				ClipCursor(NULL);
				s_nMouseState = 0;
				ReleaseCapture();
			}
			break;

		case WM_LBUTTONDBLCLK:	// ツリービューをトグル
			/*if(g_cfg.nTreeWidth<=0){	// 単純に展開
				if(g_cfg.nTWidthSub<=0) g_cfg.nTWidthSub = 200;	// 手動で左端までいったあとの展開
				g_cfg.nTreeWidth = g_cfg.nTWidthSub;
			}else{						// 非表示
				g_cfg.nTWidthSub = g_cfg.nTreeWidth;
				g_cfg.nTreeWidth = -SPLITTER_WIDTH;
			}
			UpdateWindowSize(GetParent(hSB));
			MyScroll(m_hTree);
			*/
			SendMessage(GetParent(hSB), WM_COMMAND, (WPARAM)IDM_TOGGLETREE, 0);
			break;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hSB, GWLP_USERDATA), hSB, msg, wp, lp);
}

static int GetListIndex(HWND hwndList){
	int i;
	for(i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
		if (hwndList == GetListTab(m_hTab, i)->hList)
			return i;
	}
	return -1;
}

static LPCTSTR GetColumnText(HWND hwndList, int nRow, int nColumn, LPTSTR pWork, int nWorkMax){
	int i = GetListIndex(hwndList);
	if (i >= 0){
		struct FILEINFO *pTmp = GetListTab(m_hTab, i)->pRoot;
		switch(nColumn){
			case 0:
				return GetFileName(GetPtrFromIndex(pTmp, nRow)->szFilePath);
				break;
			case 1:
				return GetPtrFromIndex(pTmp, nRow)->szSize;
				break;
			case 2:
				LPTSTR pszPath;
				pszPath = GetPtrFromIndex(pTmp, nRow)->szFilePath;
				if(IsURLPath(pszPath)){
					return TEXT("URL");
				}else if(IsArchivePath(pszPath) && GetArchiveItemType(pszPath, pWork, nWorkMax)){
					return pWork;
				}else{
					LPTSTR p = PathFindExtension(pszPath);
					if (p && *p) return p + 1;
				}
				break;
			case 3:
				return GetPtrFromIndex(pTmp, nRow)->szTime;
				break;
		}
	}
	return NULL;
}

static LRESULT CALLBACK NewTabProc(HWND hTC, UINT msg, WPARAM wp, LPARAM lp){
	static int s_nDragTab = -1;
	TCHITTESTINFO tchti;
	RECT rcItem = {0};

	switch(msg)
	{
		case WM_LBUTTONDOWN:
			tchti.flags = TCHT_NOWHERE;
			tchti.pt.x = (short)LOWORD(lp);
			tchti.pt.y = (short)HIWORD(lp);
			s_nDragTab = TabCtrl_HitTest(hTC, &tchti);
			if(s_nDragTab>0){
				SetCapture(hTC);
			}
			break;

		case WM_LBUTTONUP:
			if(GetCapture()==hTC){
				ReleaseCapture();
				s_nDragTab = -1;
			}
			break;

		case WM_MOUSEMOVE:
			int nHitTab;
			if(GetCapture()==hTC){
				tchti.flags = TCHT_NOWHERE;
				tchti.pt.x = (short)LOWORD(lp);
				tchti.pt.y = (short)HIWORD(lp);
				nHitTab = TabCtrl_HitTest(hTC, &tchti);
				if(nHitTab>0 && nHitTab!=s_nDragTab){
					MoveTab(hTC, s_nDragTab, nHitTab-s_nDragTab);
					s_nDragTab = nHitTab;
				}
			}
			break;

		case WM_CONTEXTMENU:	// リスト右クリックメニュー
			HMENU hPopMenu;
			int nItem;
			int i;
			TCHAR szMenu[MAX_FITTLE_PATH+4];

			switch(GetDlgCtrlID((HWND)wp)){
				case ID_LIST:
					if(lp==-1){	//キーボード
						nItem = ListView_GetNextItem((HWND)wp, -1, LVNI_SELECTED);
						if(nItem<0) return 0; 
						ListView_GetItemRect((HWND)wp, nItem, &rcItem, LVIR_SELECTBOUNDS);
						MapWindowPoints((HWND)wp, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
						lp = MAKELPARAM(rcItem.left, rcItem.bottom);
					}
					hPopMenu = GetSubMenu(LoadMenu(m_hInst, TEXT("LISTMENU")), 0);

					for(i=0;i<TabCtrl_GetItemCount(hTC);i++){
						wsprintf(szMenu, TEXT("&%d: %s"), i, GetListTab(hTC, i)->szTitle);
						AppendMenu(GetSubMenu(hPopMenu, GetMenuPosFromString(hPopMenu, TEXT("プレイリストに送る(&S)"))), MF_STRING, IDM_SENDPL_FIRST+i, szMenu); 
					}

					/*if(ListView_GetSelectedCount((HWND)wp)!=1){
						EnableMenuItem(hPopMenu, IDM_LIST_DELFILE, MF_GRAYED | MF_DISABLED);
					}*/
					TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, GetParent(hTC), NULL);
					DestroyMenu(hPopMenu);
					return 0;	// タブコントロールの分のWM_CONTEXTMENUが送られるのを防ぐ
			}
			break;

		case WM_NOTIFY:
			NMHDR *pnmhdr;

			pnmhdr = (LPNMHDR)lp;
			switch(pnmhdr->idFrom)
			{
				case ID_LIST:
					LPNMLISTVIEW lplv;
					lplv = (LPNMLISTVIEW)lp;
					switch(pnmhdr->code)
					{

						// リストビューからの要求
#if 1
						/* Windowsの挙動が異常 */
						case LVN_GETDISPINFOA:
						case LVN_GETDISPINFOW:
							{
								LVITEM *item = &((NMLVDISPINFO*)lp)->item;
								// テキストをセット
								if(item->mask & LVIF_TEXT){
									TCHAR szBuf[32];
									LPCTSTR lpszColText = GetColumnText(pnmhdr->hwndFrom, item->iItem, item->iSubItem, szBuf, 32);
									if (lpszColText)
										lstrcpyn(item->pszText, lpszColText, item->cchTextMax);
								}
							}
							break;
#else
						case LVN_GETDISPINFOA:
							{
								LVITEMA *item = &((NMLVDISPINFOA*)lp)->item;
								// テキストをセット
								if(item->mask & LVIF_TEXT){
									TCHAR szBuf[32];
									LPCTSTR lpszColText = GetColumnText(pnmhdr->hwndFrom, item->iItem, item->iSubItem, szBuf, 32);
									if (lpszColText)
										lstrcpyntA(item->pszText, lpszColText, item->cchTextMax);
								}
							}
							break;
						case LVN_GETDISPINFOW:
							{
								LVITEMW *item = &((NMLVDISPINFOW*)lp)->item;
								// テキストをセット
								if(item->mask & LVIF_TEXT){
									TCHAR szBuf[32];
									LPCTSTR lpszColText = GetColumnText(pnmhdr->hwndFrom, item->iItem, item->iSubItem, szBuf, 32);
									if (lpszColText)
										lstrcpyntW(item->pszText, lpszColText, item->cchTextMax);
								}
							}
							break;
#endif
						case LVN_GETINFOTIPA:
							if(g_cfg.nPathTip){
								NMLVGETINFOTIPA *lvgit = (NMLVGETINFOTIPA *)lp;
								LPTSTR pszFilePath = GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, lvgit->iItem)->szFilePath;
								lvgit->dwFlags = 0;
								lstrcpyntA(lvgit->pszText, pszFilePath, lvgit->cchTextMax);
							}
							break;
						case LVN_GETINFOTIPW:
							if(g_cfg.nPathTip){
								NMLVGETINFOTIPW *lvgit = (NMLVGETINFOTIPW *)lp;
								LPTSTR pszFilePath = GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, lvgit->iItem)->szFilePath;
								lvgit->dwFlags = 0;
								lstrcpyntW(lvgit->pszText, pszFilePath, lvgit->cchTextMax);
							}
							break;
						case NM_CUSTOMDRAW:
							NMLVCUSTOMDRAW *lplvcd;
							LOGFONT lf;

							lplvcd = (LPNMLVCUSTOMDRAW)lp;
							if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
								return CDRF_NOTIFYITEMDRAW;
							if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
								if(m_nPlayTab==TabCtrl_GetCurSel(hTC)
									&& GetCurListTab(hTC)->nPlaying==(signed int)lplvcd->nmcd.dwItemSpec
									&& OPGetStatus() != OUTPUT_PLUGIN_STATUS_STOP){
										switch(g_cfg.nPlayView){
											case 1:
												if(!m_hBoldFont){
													GetObject(GetCurrentObject(lplvcd->nmcd.hdc, OBJ_FONT), sizeof(LOGFONT), &lf);
													lf.lfWeight = FW_DEMIBOLD;
													m_hBoldFont = CreateFontIndirect(&lf);
												}
												SelectObject(lplvcd->nmcd.hdc, m_hBoldFont);
												break;
											case 2:
												lplvcd->clrText = g_cfg.nPlayTxtCol;
												break;
											case 3:
												lplvcd->clrTextBk = g_cfg.nPlayBkCol;
												break;
										}
										return CDRF_NEWFONT;
								}
							}
							break;

						case LVN_BEGINDRAG:	// アイテムドラッグ開始
							OnBeginDragList(pnmhdr->hwndFrom, lplv->ptAction);
							break;

						case LVN_COLUMNCLICK:	// カラムクリック
							int nSort;

							if(lplv->iSubItem+1==abs(GetCurListTab(hTC)->nSortState)){
								nSort = -1*GetCurListTab(hTC)->nSortState;
							}else{
								nSort = lplv->iSubItem+1;
							}
							SortListTab(GetCurListTab(hTC), nSort);
							break;

						case NM_DBLCLK:
							SendMessage(GetParent(hTC), WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_PLAY, 0), 0);
							break;

					}
					break;
			}
			break;

		case WM_COMMAND:
			SendMessage(GetParent(hTC), msg, wp, lp); //親ウィンドウにそのまま送信
			break;

		/* case WM_UNICHAR: */
		case WM_CHAR:
			if((int)wp == '\t'){
				if(GetKeyState(VK_SHIFT) < 0){
					SetFocus(m_hTree);
				}else{
					SetFocus(GetCurListTab(hTC)->hList);
				}
				return 0;
			}
			break;

		case WM_TIMER:
			switch (wp){
				case ID_SCROLLTIMERUP:		// 上スクロール
					ListView_Scroll(GetCurListTab(hTC)->hList, 0, -10);
					break;

				case ID_SCROLLTIMERDOWN:	// 下スクロール
					ListView_Scroll(GetCurListTab(hTC)->hList, 0, 10);
					break;
			}
			break;

		case WM_MOUSEWHEEL:
			if((short)HIWORD(wp)<0){
				TabCtrl_SetCurFocus(hTC, TabCtrl_GetItemCount(hTC)==TabCtrl_GetCurSel(hTC)+1?0:TabCtrl_GetCurSel(hTC)+1);
			}else{
				TabCtrl_SetCurFocus(hTC, (TabCtrl_GetCurSel(hTC)==0?TabCtrl_GetItemCount(hTC)-1:TabCtrl_GetCurSel(hTC)-1));
			}
			break;

		case WM_LBUTTONDBLCLK:
			m_nPlayTab = TabCtrl_GetCurSel(hTC);
			SendMessage(GetParent(hTC), WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_NEXT, 0), 0);
			break;

		case WM_DROPFILES:
			HDROP hDrop;
			struct FILEINFO *pSub;
			int nFileCount;
			POINT pt;
			TCHAR szPath[MAX_FITTLE_PATH];
			TCHAR szLabel[MAX_FITTLE_PATH];

			// ドラッグされたファイルを追加
			pSub = NULL;
			hDrop = (HDROP)wp;
			nFileCount = (int)DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			for(i=0;i<nFileCount;i++){
				DragQueryFile(hDrop, i, szPath, MAX_FITTLE_PATH);
				SearchFiles(&pSub, szPath, TRUE);
			}
			DragQueryPoint(hDrop, &pt);
			DragFinish(hDrop);

			// 既存のタブに追加
			nItem = TabCtrl_GetItemCount(hTC);
			for(i=0;i<nItem;i++){
				TabCtrl_GetItemRect(hTC, i, &rcItem);
				if(PtInRect(&rcItem, pt)){
					ListView_SetItemState(GetListTab(hTC, i)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);	// 選択状態クリア
					InsertList(GetListTab(hTC, i), -1, pSub);
					ListView_EnsureVisible(GetListTab(hTC, i)->hList, ListView_GetItemCount(GetListTab(hTC, i)->hList)-1, TRUE);	// 一番下のアイテムを表示
					TabCtrl_SetCurFocus(hTC, i);
					break;
				}
			}

			// タブを新規に追加
			if(nFileCount==1){
				lstrcpyn(szLabel, PathFindFileName(szPath), MAX_FITTLE_PATH);
				if(PathIsDirectory(szPath)){
					PathRemoveBackslash(szLabel);
				}else{
					PathRemoveExtension(szLabel);
				}
			}else{
				lstrcpy(szLabel, TEXT("Default"));
			}
			if(pt.x>rcItem.right && pt.y>=rcItem.top && pt.y<=rcItem.bottom){
				MakeNewTab(hTC, szLabel, -1);
				InsertList(GetListTab(hTC, nItem), -1, pSub);
				ListView_SingleSelect(GetListTab(hTC, nItem)->hList, 0);
				TabCtrl_SetCurFocus(hTC, nItem);
			}
			break;

		default:
			break;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hTC, GWLP_USERDATA), hTC, msg, wp, lp);
}

// リストビューの新しいプロシージャ
LRESULT CALLBACK NewListProc(HWND hLV, UINT msg, WPARAM wp, LPARAM lp){
	LVHITTESTINFO pinfo;
	RECT rcItem;
	POINT pt;
	struct FILEINFO *pSub = NULL;
	// struct FILEINFO *pPlaying = NULL;
	int i;
	HWND hWnd;

	switch (msg) {
		case WM_MOUSEMOVE:
			if(GetCapture()==hLV){
				// カーソルを変更
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);
				if(hWnd==m_hTab){
					SetOLECursor(3);
				}else if(hWnd==GetCurListTab(m_hTab)->hList){
					SetOLECursor(2);
				}else{
					SetOLECursor(1);
				}
				// タブフォーカスの描画
				ScreenToClient(GetParent(hLV), &pt);
				for(i=0;i<TabCtrl_GetItemCount(GetParent(hLV));i++){
					TabCtrl_GetItemRect(GetParent(hLV), i, &rcItem);
					if(PtInRect(&rcItem, pt)){
						DrawTabFocus(GetParent(hLV), i, TRUE);
					}else{
						DrawTabFocus(GetParent(hLV), i, FALSE);
					}
				}
				// ドラッグイメージを描画
				pt.x = (short)LOWORD(lp);
				pt.y = (short)HIWORD(lp);
				OnDragList(hLV, pt);
			}
			break;

		case WM_RBUTTONDOWN:
			if(GetCapture()==hLV){
				OnEndDragList(hLV);	// ドラッグキャンセル
			}
			break;

		case WM_LBUTTONUP:
			int nBefore, nAfter, nCount;

			if(GetCapture()==hLV){
				OnEndDragList(hLV); //ドラッグ終了
				//移動前アイテムと移動後アイテムのインデックスを取得
				pinfo.pt.x = (short)LOWORD(lp);
				pinfo.pt.y = (short)HIWORD(lp);
				nBefore = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
				nAfter = ListView_HitTest(hLV, &pinfo);
				if(nAfter!=-1 && nBefore!=nAfter){
					ChangeOrder(GetCurListTab(m_hTab), nAfter);
				}else{
					//タブへドラッグ
					GetCursorPos(&pt);
					ScreenToClient(m_hTab, &pt);
					for(i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
						TabCtrl_GetItemRect(m_hTab, i, &rcItem);
						if(PtInRect(&rcItem, pt)){
							DrawTabFocus(m_hTab, i, FALSE);
							SendToPlaylist(GetCurListTab(m_hTab), GetListTab(m_hTab, i));
							nCount = ListView_GetItemCount(GetListTab(m_hTab ,i)->hList) - 1;
							ListView_SingleSelect(GetListTab(m_hTab ,i)->hList, nCount);
							break;
						}
					}
				}
			}
			break;

		case WM_KEYDOWN:
			switch (wp)
			{
				case VK_RETURN:
					SendMessage(GetParent(GetParent(hLV)), WM_COMMAND, (WPARAM)IDM_PLAY, 0);
					break;

				case VK_DELETE:
					DeleteFiles(GetCurListTab(GetParent(hLV)));
					break;

				case VK_TAB:
					if(g_cfg.nTreeState!=TREE_SHOW) break;
					if(GetKeyState(VK_SHIFT) < 0){
						SetFocus(GetParent(hLV));
					}else{
						SetFocus(m_hCombo);
					}
					break;

				case VK_DOWN:
					int nToIndex;
					if(GetKeyState(VK_CONTROL) < 0){
						nToIndex = ListView_GetNextItem(hLV, -1, LVIS_SELECTED);
						do{
							nToIndex++;
						}while(ListView_GetItemState(hLV, nToIndex, LVIS_SELECTED));
						ChangeOrder(GetCurListTab(GetParent(hLV)), nToIndex);
						return 0;
					}
					break;

				case VK_UP:
					if(GetKeyState(VK_CONTROL) < 0){
						nToIndex = ListView_GetNextItem(hLV, -1, LVIS_SELECTED);
						if(nToIndex>0){
							ChangeOrder(GetCurListTab(GetParent(hLV)), nToIndex-1);
						}
						return 0;
					}
					break;

				case VK_LEFT:
					SCROLLINFO sinfo;

					ZeroMemory(&sinfo, sizeof(SCROLLINFO));
					sinfo.cbSize = sizeof(SCROLLINFO);
					sinfo.fMask = SIF_POS;
					GetScrollInfo(hLV, SB_HORZ, &sinfo);
					if(g_cfg.nTreeState==TREE_SHOW && sinfo.nPos==0){
						SetFocus(m_hTree);
						TreeView_EnsureVisible(m_hTree, TreeView_GetSelection(m_hTree));
						MyScroll(m_hTree);
					}
					break;

				default:
					break;
			}
			break;

		case WM_DROPFILES:
			HDROP hDrop;
			TCHAR szPath[MAX_FITTLE_PATH];

			/*	カーソルの下に追加
			int nHitItem;
			GetCursorPos(&pt);
			ScreenToClient(hLV, &pt);
			pinfo.pt.x = pt.x;
			pinfo.pt.y = pt.y;
			nHitItem = ListView_HitTest(hLV, &pinfo);
			*/
			hDrop = (HDROP)wp;
			//pPlaying = GetPtrFromIndex(GetCurListTab(GetParent(hLV))->pRoot, GetCurListTab(GetParent(hLV))->nPlaying);
			for(i=0;i<(int)DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);i++){
				DragQueryFile(hDrop, i, szPath, MAX_FITTLE_PATH);
				SearchFiles(&pSub, szPath, TRUE);
			}
			DragFinish(hDrop);

			ListView_SetItemState(hLV, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);	// 表示をクリア
			InsertList(GetCurListTab(GetParent(hLV)), -1, pSub);
			ListView_EnsureVisible(hLV, ListView_GetItemCount(hLV)-1, TRUE);
			//GetCurListTab(GetParent(hLV))->nPlaying = GetIndexFromPtr(GetCurListTab(GetParent(hLV))->pRoot, pPlaying);
			SetForegroundWindow(GetParent(GetParent(hLV)));
			break;

		default:
			break;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hLV, GWLP_USERDATA), hLV, msg, wp, lp);
}

// ツリービューのプロシージャ
static LRESULT CALLBACK NewTreeProc(HWND hTV, UINT msg, WPARAM wp, LPARAM lp){
	int i;
	HTREEITEM hti;
	static int s_nPrevCur;

	switch(msg){
		case WM_LBUTTONDBLCLK:	// 子を持たないノードダブルクリックで再生
			if(!TreeView_GetChild(hTV, TreeView_GetSelection(hTV))){
				m_nPlayTab = 0;
				SendMessage(GetParent(hTV), WM_COMMAND, (WPARAM)IDM_NEXT, 0);
			}
			break;

		case WM_CHAR:
			switch(wp){
				case TEXT('\t'):
					if(GetKeyState(VK_SHIFT) < 0){
						SetFocus(GetNextWindow(hTV, GW_HWNDPREV));
					}else{
						SetFocus(GetNextWindow(hTV, GW_HWNDNEXT));
					}
					return 0;

				case VK_RETURN:
					if(ListView_GetItemCount(GetListTab(m_hTab, 0)->hList)>0){
						m_nPlayTab = 0;
						SendMessage(GetParent(hTV), WM_COMMAND, (WPARAM)IDM_NEXT, 0);
					}else{
						TreeView_Expand(hTV, TreeView_GetSelection(hTV), TVE_TOGGLE);
					}
					return 0;

				default:
					break;
			}
			break;

		case WM_KEYDOWN:
			if(wp==VK_RIGHT){	// 子ノードがない or 展開状態ならばListにフォーカスを移す
				hti = TreeView_GetSelection(hTV);

				TVITEM tvi;
				tvi.mask = TVIF_STATE;
				tvi.hItem = hti;
				tvi.stateMask = TVIS_EXPANDED;
				TreeView_GetItem(hTV, &tvi);

				if(!TreeView_GetChild(hTV, hti) || (tvi.state & TVIS_EXPANDED)){
					SetFocus(GetCurListTab(m_hTab)->hList);
					return 0;
				}
				break;
			}
			break;

		case WM_LBUTTONUP:
			POINT pt;
			HWND hWnd;
			struct FILEINFO *pSub;
			pSub = NULL;
			TCHAR szNowDir[MAX_FITTLE_PATH];
			int nHitTab;
			TCHITTESTINFO info;
			RECT rcTab;
			RECT rcList;

			if(GetCapture()==hTV){
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);
				if(hWnd==m_hTab){
					GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					SearchFiles(&pSub, szNowDir, TRUE);
					ScreenToClient(m_hTab, &pt);
					info.pt.x = pt.x;
					info.pt.y = pt.y;
					info.flags = TCHT_ONITEM;
					nHitTab = TabCtrl_HitTest(m_hTab, &info);
					if(nHitTab!=-1){
						DrawTabFocus(m_hTab, nHitTab, FALSE);
						ListView_SetItemState(GetListTab(m_hTab, nHitTab)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
						InsertList(GetListTab(m_hTab, nHitTab), -1, pSub);
						ListView_EnsureVisible(GetListTab(m_hTab, nHitTab)->hList, ListView_GetItemCount(GetListTab(m_hTab, nHitTab)->hList)-1, TRUE);
					}
				}else if(hWnd==GetCurListTab(m_hTab)->hList){
					SendMessage(GetParent(hTV), WM_COMMAND, MAKEWPARAM(IDM_TREE_ADD, 0), 0);
				}else if(hWnd==GetParent(hTV)){
					// 新規タブ
					GetWindowRect(GetCurListTab(m_hTab)->hList, &rcList);
					GetWindowRect(m_hTab, &rcTab);

					if(g_cfg.nTabBottom==0 && pt.y>=rcTab.top && pt.y<=rcList.top && pt.x>=rcTab.left
					|| g_cfg.nTabBottom==1 && pt.y<=rcTab.bottom && pt.y>=rcList.bottom && pt.x>=rcTab.left){
						SendMessage(GetParent(hTV), WM_COMMAND, MAKEWPARAM(IDM_TREE_NEW, 0), 0);
					}
				}
				ReleaseCapture();
			}
			break;

		case WM_RBUTTONUP:
			if(GetCapture()==hTV){
				ReleaseCapture();
				for(i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
					DrawTabFocus(m_hTab, i, FALSE);
				}
			}
			break;

		case WM_MOUSEMOVE:
			RECT rcItem;

			if(GetCapture()==m_hTree){
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);

				if(hWnd==m_hTab || hWnd==GetCurListTab(m_hTab)->hList){
					SetOLECursor(3);
				}else if(hWnd==GetParent(m_hTab)){
					// 新規タブ
					GetWindowRect(GetCurListTab(m_hTab)->hList, &rcList);
					GetWindowRect(m_hTab, &rcTab);

					if(g_cfg.nTabBottom==0 && pt.y>=rcTab.top && pt.y<=rcList.top && pt.x>=rcTab.left
					|| g_cfg.nTabBottom==1 && pt.y<=rcTab.bottom && pt.y>=rcList.bottom && pt.x>=rcTab.left){
						SetOLECursor(3);
					}else{
						SetOLECursor(1);
					}
				}else{
					SetOLECursor(1);
				}
				
				// タブのフォーカスを描画
				ScreenToClient(m_hTab, &pt);
				for(i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
					TabCtrl_GetItemRect(m_hTab, i, &rcItem);
					if(PtInRect(&rcItem, pt)){
						DrawTabFocus(m_hTab, i, TRUE);
					}else{
						DrawTabFocus(m_hTab, i, FALSE);
					}
				}
			}
			break;

		case WM_DROPFILES:
			HDROP hDrop;
			TCHAR szPath[MAX_FITTLE_PATH];
			TCHAR szParDir[MAX_PATH];

			hDrop = (HDROP)wp;
			DragQueryFile(hDrop, 0, szPath, MAX_FITTLE_PATH);
			DragFinish(hDrop);

			GetParentDir(szPath, szParDir);
			MakeTreeFromPath(hTV, m_hCombo, szParDir);

			SetForegroundWindow(GetParent(GetParent(hTV)));
			break;

	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hTV, GWLP_USERDATA), hTV, msg, wp, lp);
}

// ツールバーの幅を取得（ドロップダウンがあってもうまく行きます）
static LONG GetToolbarTrueWidth(HWND hToolbar){
	RECT rct;

	SendMessage(hToolbar, TB_GETITEMRECT, SendMessage(hToolbar, TB_BUTTONCOUNT, 0, 0)-1, (LPARAM)&rct);
	return rct.right;
}

// 設定ファイルを読み直し適用する(一部設定を除く)
static void ApplyConfig(HWND hWnd){
	TCHAR szNowDir[MAX_FITTLE_PATH];
	BOOL fIsIconic = IsIconic(hWnd);
	BOOL fOldTrayVisible = m_bTrayFlag;
	BOOL fNewTrayVisible = FALSE;

	UnRegHotKey(hWnd);

	LoadConfig();

	if(g_cfg.nHighTask){
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);		
	}else{
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);	
	}

	if(g_cfg.nTreeIcon) RefreshComboIcon(m_hCombo);
	InitTreeIconIndex(m_hCombo, m_hTree, (BOOL)g_cfg.nTreeIcon);

	SetUIColor();
	SetUIFont();

	if (g_cfg.nTrayOpt == 0){
		fNewTrayVisible = FALSE;
		if (!IsWindowVisible(hWnd))
			ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
	} else {
		if (g_cfg.nTrayOpt == 1){
			fNewTrayVisible = !IsWindowVisible(hWnd) || fIsIconic;
		}else{
			fNewTrayVisible = TRUE;
		}
		if (fIsIconic)
			ShowWindow(hWnd, SW_HIDE);
	}

	if (fOldTrayVisible && !fNewTrayVisible){
		Shell_NotifyIcon(NIM_DELETE, &m_ni);
		m_bTrayFlag = FALSE;
	}
	if (!fOldTrayVisible && fNewTrayVisible){
			SetTaskTray(GetParent(m_hStatus));
	}

	lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
	MakeTreeFromPath(m_hTree, m_hCombo, szNowDir); 

	RegHotKey(hWnd);
	InvalidateRect(hWnd, NULL, TRUE);
}

// 外部設定プログラムを起動する
static void ExecuteSettingDialog(HWND hWnd, LPCTSTR lpszConfigPath){
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	TCHAR szCmd[MAX_FITTLE_PATH];
	TCHAR szPath[MAX_FITTLE_PATH];

	(void)hWnd;

	GetModuleParentDir(szPath);
	lstrcat(szPath, TEXT("fconfig.exe"));
	
	wsprintf(szCmd, TEXT("\"%s\" %s"), szPath, lpszConfigPath);
	

	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);

	if (CreateProcess(NULL,(LPTSTR)szCmd,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS, NULL,NULL,&si,&pi)){
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

// 設定画面を開く
static void ShowSettingDialog(HWND hWnd, int nPage){
	static const LPCTSTR table[] = {
		TEXT("fittle/general"),
		TEXT("fittle/path"),
		TEXT("fittle/control"),
		TEXT("fittle/tasktray"),
		TEXT("fittle/hotkey"),
		TEXT("fittle/about")
	};
	if (nPage < 0 || nPage > WM_F4B24_IPC_SETTING_LP_MAX) nPage = 0;
	ExecuteSettingDialog(hWnd, table[nPage]);
}

// 対応拡張子リストを取得する
static void SendSupportList(HWND hWnd){
	LPTSTR lpExt;
	TCHAR szList[MAX_FITTLE_PATH];
	int i;
	int p=0;
	for(i = 0; lpExt = GetTypelist(i), (lpExt[0] != TEXT('\0')); i++){
		int l = lstrlen(lpExt);
		if (l > 0 && p + (p != 0) + l + 1 < MAX_FITTLE_PATH)
		{
			if (p != 0) szList[p++] = TEXT(' ');
			lstrcpy(szList + p, lpExt);
			p += l;
		}
	}
	if (p < MAX_FITTLE_PATH) szList[p] = 0;
	SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)szList);
}

// 指定した文字列を持つメニュー項目を探す
static int GetMenuPosFromString(HMENU hMenu, LPTSTR lpszText){
	int i;
	TCHAR szBuf[64];
	int c = GetMenuItemCount(hMenu);
	for (i = 0; i < c; i++){
		GetMenuString(hMenu, i, szBuf, 64, MF_BYPOSITION);
		if (lstrcmp(lpszText, szBuf) == 0)
			return i;
	}
	return 0;
}

static LRESULT CALLBACK MyHookProc(int nCode, WPARAM wp, LPARAM lp){
	MSG *msg;
	msg = (MSG *)lp;

	if(nCode<0)
		return CallNextHookEx(m_hHook, nCode, wp, lp);
	
	if(msg->message==WM_MOUSEWHEEL){
		SendMessage(WindowFromPoint(msg->pt), WM_MOUSEWHEEL, msg->wParam, msg->lParam);
		msg->message = WM_NULL;
	}

	return CallNextHookEx(m_hHook, nCode, wp, lp);
}

// 選択したファイルを実際に削除する
static void RemoveFiles(HWND hWnd){
	// 初期化
	SHFILEOPSTRUCT fops;
	TCHAR szMsg[MAX_FITTLE_PATH];
	int i, j, q, s;
	LPTSTR pszTarget;
	BOOL bPlaying;
	LPTSTR p, np;

	bPlaying = FALSE;
	j = 0;
	q = 0;
	s = 2 * sizeof(TCHAR);
	p = (LPTSTR)HZAlloc(s);
	if (!p) return;

	// パスを連結など
	i = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, -1, LVNI_SELECTED);
	while(i!=-1){
		pszTarget = GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, i)->szFilePath;
		if(i==GetCurListTab(m_hTab)->nPlaying) bPlaying = TRUE;	// 削除ファイルが演奏中
		if(!IsURLPath(pszTarget) && !IsArchivePath(pszTarget)){	// 削除できないファイルでなければ
			int l = lstrlen(pszTarget);
			j++;
			s += (l + 1) * sizeof(TCHAR);
			np = (LPTSTR)HRealloc(p, s);
			if (!np) {
				HFree(p);
				return;
			}
			p = np;
			lstrcpy(p+q, pszTarget);
			q += l + 1;
			*(p+q) = TEXT('\0');
		}
		i = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, i, LVNI_SELECTED);
	}
	if(j==1){
		// ファイル一個選択
		wsprintf(szMsg, TEXT("%'%s%' をごみ箱に移しますか？"), p);
	}else if(j>1){
		// 複数ファイル選択
		wsprintf(szMsg, TEXT("これらの %d 個の項目をごみ箱に移しますか？"), j);
	}
	if(j>0 && MessageBox(hWnd, szMsg, TEXT("ファイルの削除の確認"), MB_YESNO | MB_ICONQUESTION)==IDYES){
		// 実際に削除
		fops.hwnd = hWnd;
		fops.wFunc = FO_DELETE;
		fops.pFrom = p;
		fops.pTo = NULL;
		fops.fFlags = FOF_FILESONLY | FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_MULTIDESTFILES;
		if(bPlaying) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_STOP, 0), 0);
		if(SHFileOperation(&fops)==0){
			if(bPlaying) PopPrevious(GetCurListTab(m_hTab));	// 履歴からとりあえず最後だけ削除
			DeleteFiles(GetCurListTab(m_hTab));
		}
		if(bPlaying) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
	}
	HFree(p);
}


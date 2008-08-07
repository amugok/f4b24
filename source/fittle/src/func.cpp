/*
 * Func.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	便利な関数(あんまり便利じゃないかも)
//

#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#endif

#include "func.h"
#include "list.h"
#include "bass_tag.h"
#include "archive.h"
#include "f4b24.h"

/* ファイル名のポインタを取得 */
LPTSTR GetFileName(LPTSTR szIn){
	LPTSTR p, q;

	p = q = szIn;
	if(IsURLPath(szIn)) return q;
	if(IsArchivePath(szIn)){
		LPTSTR r = GetArchiveItemFileName(szIn);
		if (r) return r;
	}
	while(*p){
#ifdef UNICODE
#else
		if(IsDBCSLeadByte(*p)){
			p += 2;
			continue;
		}
#endif
		if(*p==TEXT('\\') || *p==TEXT('/') /* || *p==TEXT('?')*/ ) q = p + 1;
		p++;
	}
	return q;
}

//親フォルダを取得
int GetParentDir(LPCTSTR pszFilePath, LPTSTR pszParPath){
	TCHAR szLongPath[MAX_FITTLE_PATH] = {0};

	GetLongPathName(pszFilePath, szLongPath, MAX_FITTLE_PATH); //98以降のみ
	if(!FILE_EXIST(pszFilePath)){
		//ファイル、ディレクトリ以外
		return OTHERS;
	}else if(PathIsDirectory(pszFilePath)){
		//ディレクトリだった場合
		lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
		return FOLDERS;
	}else{
		//ファイルだった場合

		if(IsPlayListFast(szLongPath)){
			// リスト
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			return LISTS;
		}else if(IsArchiveFast(szLongPath)){
			// アーカイブ
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			return ARCHIVES;
		}else{
			// 音楽ファイル
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			*PathFindFileName(pszParPath) = TEXT('\0');
			return FILES;
		}
	}
}

/*プレイリストかどうか判断*/
BOOL IsPlayListFast(LPTSTR szFilePath){
	LPTSTR p;
	if((p = PathFindExtension(szFilePath)) == NULL || !*p) return FALSE;
	p++;
	if(lstrcmpi(p, TEXT("M3U"))==0 || lstrcmpi(p, TEXT("M3U8"))==0 || lstrcmpi(p, TEXT("PLS"))==0)
		return TRUE;
	else
		return FALSE;
}

/*プレイリストかどうか判断*/
BOOL IsPlayList(LPTSTR szFilePath){
	if(PathIsDirectory(szFilePath)) return FALSE;
	return IsPlayListFast(szFilePath);
}

void FormatDateTime(LPTSTR pszBuf, LPFILETIME pft){
	FILETIME lt;
	FileTimeToLocalFileTime(pft, &lt);
	FormatLocalDateTime(pszBuf, &lt);
}

void FormatLocalDateTime(LPTSTR pszBuf, LPFILETIME pft){
	SYSTEMTIME st;
	FileTimeToSystemTime(pft, &st);
	wsprintf(pszBuf, TEXT("%02d/%02d/%02d %02d:%02d:%02d"),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

BOOL GetTimeAndSize(LPCTSTR pszFilePath, LPTSTR pszFileSize, LPTSTR pszFileTime){
	WASTR szPath;
	BY_HANDLE_FILE_INFORMATION bhfi;
	HANDLE hFile;

	WAstrcpyT(&szPath, pszFilePath);
	hFile = WAOpenFile(&szPath);
	if(hFile==INVALID_HANDLE_VALUE || !GetFileInformationByHandle(hFile, &bhfi)){
		lstrcpy(pszFileSize, TEXT("-"));
		lstrcpy(pszFileTime, TEXT("-"));
		return FALSE;
	}

	FormatDateTime(pszFileTime, &bhfi.ftLastWriteTime);
	wsprintf(pszFileSize, TEXT("%d KB"), bhfi.nFileSizeLow / 1024);

	CloseHandle(hFile);

	return TRUE;
}

void SetOLECursor(int nIndex){
	static HMODULE hMod = NULL;
	HCURSOR hCur;

	if(!hMod){
		hMod = GetModuleHandle(TEXT("ole32"));
	}
	hCur = LoadCursor(hMod, MAKEINTRESOURCE(nIndex));
	SetCursor(hCur);
	return;
}

LPTSTR MyPathAddBackslash(LPTSTR pszPath){
	if(PathIsDirectory(pszPath)){
		return PathAddBackslash(pszPath);
	}else if(IsPlayListFast(pszPath) || IsArchiveFast(pszPath)){
		return pszPath;
	}else{
		return PathAddBackslash(pszPath);
	}
}

void GetFolderPart(LPTSTR pszPath){
	if(IsPlayList(pszPath) || IsArchive(pszPath)){
		*PathFindFileName(pszPath) = TEXT('\0');
	}else{
		MyPathAddBackslash(pszPath);
	}
}


// ウィンドウをサブクラス化、USERDATAに元のWNDPROCを保存
void SubClassControl(HWND hWnd, WNDPROC Proc){
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Proc));
}

LRESULT SubClassCallNext(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA), hWnd, msg, wp, lp);
}

// 指定した文字列を持つメニュー項目を探す
int GetMenuPosFromString(HMENU hMenu, LPTSTR lpszText){
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

// ツールバーの幅を取得（ドロップダウンがあってもうまく行きます）
LONG GetToolbarTrueWidth(HWND hToolbar){
	RECT rct;

	SendMessage(hToolbar, TB_GETITEMRECT, SendMessage(hToolbar, TB_BUTTONCOUNT, 0, 0)-1, (LPARAM)&rct);
	return rct.right;
}

// 子ウインドウなどのサイズを調整させる
void UpdateWindowSize(HWND hWnd){
	RECT rcDisp;

	GetClientRect(hWnd, &rcDisp);
	SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(rcDisp.right, rcDisp.bottom));
	return;
}

// ドロップされたファイルのリストを取得
int GetDropFiles(HDROP hDrop, struct FILEINFO **ppSub, LPPOINT ppt, LPTSTR szPath){
	int i;
	int nFileCount;
	*ppSub = NULL;
	nFileCount = (int)DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	for(i = 0; i< nFileCount; i++){
		DragQueryFile(hDrop, i, szPath, MAX_FITTLE_PATH);
		SearchFiles(ppSub, szPath, TRUE);
	}
	if (ppt) DragQueryPoint(hDrop, ppt);
	DragFinish(hDrop);
	return nFileCount;
}

// 外部設定プログラムを起動する
static void ExecuteSettingDialog(HWND hWnd, LPCSTR lpszConfigPath){

	WASTR szCmd;
	WASTR szPara;

	WAGetModuleParentDir(NULL, &szCmd);
	WAstrcatA(&szCmd, "fconfig.exe");
	WAstrcpyA(&szPara, lpszConfigPath);

	WAShellExecute(hWnd, &szCmd, &szPara);
}

// 設定画面を開く
void ShowSettingDialog(HWND hWnd, int nPage){
	static const LPCSTR table[] = {
		"fittle/general",
		"fittle/path",
		"fittle/control",
		"fittle/tasktray",
		"fittle/hotkey",
		"fittle/about"
	};
	if (nPage < 0 || nPage > WM_F4B24_IPC_SETTING_LP_MAX) nPage = 0;
	ExecuteSettingDialog(hWnd, table[nPage]);
}

// M3Uファイルを保存ダイアログを出す
int SaveM3UDialog(LPTSTR szDir, LPTSTR szDefTitle){
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

/* コマンドラインパラメータの展開 */
HMODULE ExpandArgs(int *pARGC, LPTSTR **pARGV){
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
	HMODULE h = LoadLibraryA("MSVCRT.DLL");
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






/*

	--- リストコントロール ---

*/

void LV_SetState(HWND hLV, int nItem, int nState){
	ListView_SetItemState(hLV, nItem, nState, nState);
}

static void LV_ClearStateAll(HWND hLV, int nState){
	ListView_SetItemState(hLV, -1, 0, nState);
}

// 選択状態クリア
void LV_ClearSelect(HWND hLV){
	LV_ClearStateAll(hLV, (LVIS_SELECTED | LVIS_FOCUSED));
}

// ハイライト状態クリア
void LV_ClearHilite(HWND hLV){
	LV_ClearStateAll(hLV, LVIS_DROPHILITED);
}


static void LV_SingleSelectViewSub(HWND hLV, int nIndex, int flag){
	LV_ClearSelect(hLV);
	if (nIndex >= 0){
		LV_SetState(hLV, nIndex, (LVIS_SELECTED | LVIS_FOCUSED));
		if (flag & 1) ListView_EnsureVisible(hLV, nIndex, TRUE);
		if (flag & 2) PostMessage(hLV, LVM_ENSUREVISIBLE, (WPARAM)nIndex, (LPARAM)TRUE);
	}
}

// 全ての選択状態を解除後、指定インデックスのアイテムを選択
void LV_SingleSelect(HWND hLV, int nIndex){
	LV_SingleSelectViewSub(hLV, nIndex, 0);
}

// 全ての選択状態を解除後、指定インデックスのアイテムを選択、表示
void LV_SingleSelectView(HWND hLV, int nIndex){
	LV_SingleSelectViewSub(hLV, nIndex, 1);
}

// 全ての選択状態を解除後、指定インデックスのアイテムを選択、表示予約
void LV_SingleSelectViewP(HWND hLV, int nIndex) {
	LV_SingleSelectViewSub(hLV, nIndex, 3);
}

int LV_GetNextSelect(HWND hLV, int nIndex){
	return ListView_GetNextItem(hLV, nIndex, LVNI_SELECTED);
}

int LV_GetCount(HWND hLV){
	return ListView_GetItemCount(hLV);
}

int LV_HitTest(HWND hLV, LONG lPos){
	LVHITTESTINFO pinfo;
	pinfo.pt.x = (short)LOWORD(lPos);
	pinfo.pt.y = (short)HIWORD(lPos);
	return ListView_HitTest(hLV, &pinfo);
}




/*

	--- 文字列 ---

*/
void lstrcpyntA(LPSTR lpDst, LPCTSTR lpSrc, int nDstMax){
#if defined(UNICODE)
	WideCharToMultiByte(CP_ACP, 0, lpSrc, -1, lpDst, nDstMax, NULL, NULL);
#else
	lstrcpyn(lpDst, lpSrc, nDstMax);
#endif
}

void lstrcpyntW(LPWSTR lpDst, LPCTSTR lpSrc, int nDstMax){
#if defined(UNICODE)
	lstrcpyn(lpDst, lpSrc, nDstMax);
#else
	MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, lpDst, nDstMax); //S-JIS->Unicode
#endif
}

void lstrcpynAt(LPTSTR lpDst, LPCSTR lpSrc, int nDstMax){
#if defined(UNICODE)
	MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, lpDst, nDstMax); //S-JIS->Unicode
#else
	lstrcpyn(lpDst, lpSrc, nDstMax);
#endif
}

void lstrcpynWt(LPTSTR lpDst, LPCWSTR lpSrc, int nDstMax){
#if defined(UNICODE)
	lstrcpyn(lpDst, lpSrc, nDstMax);
#else
	WideCharToMultiByte(CP_ACP, 0, lpSrc, -1, lpDst, nDstMax, NULL, NULL);
#endif
}

typedef struct StringList {
	struct StringList *pNext;
	TCHAR szString[1];
} STRING_LIST, *LPSTRING_LIST;

static LPSTRING_LIST lpTypelist = NULL;

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

void ClearTypelist(){
	StringListFree(&lpTypelist);
}

static int AddTypelist(LPTSTR szExt){
	if (!szExt || !*szExt) return TRUE;
	if (StringListFindI(&lpTypelist, szExt)) return TRUE;
	return StringListAdd(&lpTypelist, szExt);
}

LPTSTR GetTypelist(int nIndex){
	static TCHAR szNull[1] = {0};
	LPSTRING_LIST lpbm = StringListWalk(&lpTypelist, nIndex);
	return lpbm ? lpbm->szString : szNull;
}

static void AddType(LPTSTR lpszType){
	LPTSTR e = StrStr(lpszType, TEXT("."));
	AddTypelist(e ? e + 1 : lpszType);
}

void AddTypes(LPCSTR lpszTypes) {
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

// 対応拡張子リストを取得する
void SendSupportList(HWND hWnd){
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



/*

	--- 演奏時間関係 ---

*/
float ChPosToSec(DWORD hCh, QWORD qPos) {
	return (float)BASS_ChannelBytes2Seconds(hCh, qPos);
}

float TrackPosToSec(QWORD qPos) {
	return ChPosToSec(g_cInfo[g_bNow].hChan, qPos);
}

QWORD TrackGetPos(){
	CHANNELINFO *pCh = &g_cInfo[g_bNow];
	return BASS_ChannelGetPosition(pCh->hChan, BASS_POS_BYTE) - pCh->qStart;
}

BOOL TrackSetPos(QWORD qPos) {
	CHANNELINFO *pCh = &g_cInfo[g_bNow];
	return BASS_ChannelSetPosition(pCh->hChan, qPos + pCh->qStart, BASS_POS_BYTE);
}



/*

	--- メモリ管理 ---

*/

#define ALTYPE 0
#if (ALTYPE == 1)
/* リークチェック */
void *HAlloc(DWORD dwSize){
	return malloc(dwSize);
}
void *HZAlloc(DWORD dwSize){
	return calloc(dwSize, 1);
}
void *HRealloc(LPVOID pPtr, DWORD dwSize){
	return realloc(pPtr, dwSize);
}
void HFree(LPVOID pPtr){
	free(pPtr);
}
#elif (ALTYPE == 2)
/* 開放ポインタアクセスチェック */
typedef struct {
	DWORD dwSize;
} HW;
LPVOID HAlloc(DWORD dwSize){
	HW *p = (HW *)VirtualAlloc(0, sizeof(HW) + dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (!p) return NULL;
	p->dwSize = dwSize;
	ZeroMemory(p + 1, dwSize);
	return p + 1;
}
LPVOID HZAlloc(DWORD dwSize){
	return HAlloc(dwSize);
}
LPVOID HRealloc(LPVOID pPtr, DWORD dwSize){
	HW *o = ((HW *)pPtr) - 1;
	LPVOID n = HAlloc(dwSize);
	if (n) {
		CopyMemory(n, pPtr, o->dwSize);
		HFree(pPtr);
	}
	return n;
}
void HFree(LPVOID pPtr){
	DWORD dwOldProtect;
	HW *o = ((HW *)pPtr) - 1;
	VirtualProtect(o, o->dwSize, PAGE_NOACCESS, &dwOldProtect);
}
#else
void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}
void *HZAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
}
void *HRealloc(LPVOID pPtr, DWORD dwSize){
	return HeapReAlloc(GetProcessHeap(), 0, pPtr, dwSize);
}
void HFree(LPVOID pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}
#endif

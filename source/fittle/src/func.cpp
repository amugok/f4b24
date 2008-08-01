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
#include "bass_tag.h"
#include "archive.h"

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

// 選択状態クリア
void ListView_ClearSelect(HWND hLV){
	ListView_SetItemState(hLV, -1, 0, (LVIS_SELECTED | LVIS_FOCUSED));
}

static void ListView_SingleSelectViewSub(HWND hLV, int nIndex, int flag){
	ListView_ClearSelect(hLV);
	if (nIndex >= 0){
		ListView_SetItemState(hLV, nIndex, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
		if (flag & 1) ListView_EnsureVisible(hLV, nIndex, TRUE);
		if (flag & 2) PostMessage(hLV, LVM_ENSUREVISIBLE, (WPARAM)nIndex, (LPARAM)TRUE);
	}
}

// 全ての選択状態を解除後、指定インデックスのアイテムを選択
void ListView_SingleSelect(HWND hLV, int nIndex){
	ListView_SingleSelectViewSub(hLV, nIndex, 0);
}

// 全ての選択状態を解除後、指定インデックスのアイテムを選択、表示
void ListView_SingleSelectView(HWND hLV, int nIndex){
	ListView_SingleSelectViewSub(hLV, nIndex, 1);
}

// 全ての選択状態を解除後、指定インデックスのアイテムを選択、表示予約
void ListView_SingleSelectViewP(HWND hLV, int nIndex) {
	ListView_SingleSelectViewSub(hLV, nIndex, 3);
}

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
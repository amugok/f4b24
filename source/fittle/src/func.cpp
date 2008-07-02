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

//Int型で設定ファイル書き込み
int WritePrivateProfileInt(LPTSTR szAppName, LPTSTR szKeyName, int nData, LPTSTR szINIPath){
	TCHAR szTemp[100];

	wsprintf(szTemp, TEXT("%d"), nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}

BOOL GetTimeAndSize(LPCTSTR pszFilePath, LPTSTR pszFileSize, LPTSTR pszFileTime){
	HANDLE hFile;
	FILETIME ft;
	SYSTEMTIME st;
	FILETIME lt;
	DWORD dwSize;

	hFile = CreateFile(pszFilePath, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE){
		lstrcpy(pszFileSize, TEXT("-"));
		lstrcpy(pszFileTime, TEXT("-"));
		return FALSE;
	}

	GetFileTime(hFile, NULL, NULL, &ft);
	FileTimeToLocalFileTime(&ft, &lt);
	FileTimeToSystemTime(&lt, &st);
	wsprintf(pszFileTime, TEXT("%02d/%02d/%02d %02d:%02d:%02d"),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	dwSize = GetFileSize(hFile, NULL);
	wsprintf(pszFileSize, TEXT("%d KB"), dwSize/1024);

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

void GetModuleParentDir(LPTSTR szParPath){
	TCHAR szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98以降
	*PathFindFileName(szParPath) = TEXT('\0');
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

#ifdef _DEBUG
#if 1
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
#else
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
		CopyMemory(n, o + 1, o->dwSize);
		HFree(pPtr);
	}
	return n;
}
void HFree(LPVOID pPtr){
	DWORD dwOldProtect;
	HW *o = ((HW *)pPtr) - 1;
	VirtualProtect(o, o->dwSize, PAGE_GUARD, &dwOldProtect);
}
#endif
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
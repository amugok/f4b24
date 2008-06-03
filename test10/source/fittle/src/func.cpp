/*
 * Func.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	便利な関数(あんまり便利じゃないかも)
//

#include "func.h"
#include "archive.h"
#include "cuesheet.h"

/* ファイル名のポインタを取得 */
char *GetFileName(char *szIn){
	char *p, *q;

	p = q = szIn;
	if(IsURLPath(szIn)) return q;
	if(IsCueSheetPath(szIn)) return StrStr(szIn, ".cue/") + 5;
	while(*p){
		if(IsDBCSLeadByte(*p)){
			p++;
		}else{
			if(*p=='\\' || *p=='/'/* || *p=='?'*/) q = p + 1;
		}
		p++;
	}
	return q;
}

//親フォルダを取得
int GetParentDir(char *pszFilePath, char *pszParPath){
	char szLongPath[MAX_FITTLE_PATH] = {0};

	GetLongPathName(pszFilePath, szLongPath, MAX_FITTLE_PATH); //98以降のみ
	if(!FILE_EXIST(pszFilePath)){
		//ファイル、ディレクトリ以外
		return OTHERS;
	}else if(GetFileAttributes(pszFilePath) & FILE_ATTRIBUTE_DIRECTORY){
		//ディレクトリだった場合
		lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
		return FOLDERS;
	}else{
		//ファイルだった場合

		if(IsPlayList(szLongPath)){
			// リスト
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			return LISTS;
		}else if(IsCueSheet(szLongPath)){
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			return CUESHEETS;
		}else if(IsArchive(szLongPath)){
			// アーカイブ
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			return ARCHIVES;
		}else{
			// 音楽ファイル
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			*PathFindFileName(pszParPath) = '\0';
			return FILES;
		}
	}
}

/*プレイリストかどうか判断*/
BOOL IsPlayList(char *szFilePath){
	char *p;

	if(GetFileAttributes(szFilePath) & FILE_ATTRIBUTE_DIRECTORY) return FALSE;
	if((p = PathFindExtension(szFilePath)) == NULL || !*p) return FALSE;
	p++;
	if(lstrcmpi(p, "M3U")==0 || lstrcmpi(p, "PLS")==0)
		return TRUE;
	else
		return FALSE;
}

//Int型で設定ファイル書き込み
int WritePrivateProfileInt(char *szAppName, char *szKeyName, int nData, char *szINIPath){
	char szTemp[100];

	wsprintf(szTemp, "%d", nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}

BOOL GetTimeAndSize(const char *pszFilePath, char *pszFileSize, char *pszFileTime){
	HANDLE hFile;
	FILETIME ft;
	SYSTEMTIME st;
	FILETIME lt;
	DWORD dwSize;

	hFile = CreateFile(pszFilePath, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE){
		lstrcpy(pszFileSize, "-");
		lstrcpy(pszFileTime, "-");
		return FALSE;
	}

	GetFileTime(hFile, NULL, NULL, &ft);
	FileTimeToLocalFileTime(&ft, &lt);
	FileTimeToSystemTime(&lt, &st);
	wsprintf(pszFileTime, "%02d/%02d/%02d %02d:%02d:%02d",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	dwSize = GetFileSize(hFile, NULL);
	wsprintf(pszFileSize, "%d KB", dwSize/1024);

	CloseHandle(hFile);

	return TRUE;
}

void SetOLECursor(int nIndex){
	static HMODULE hMod = NULL;
	HCURSOR hCur;

	if(!hMod){
		hMod = GetModuleHandle("ole32");
	}
	hCur = LoadCursor(hMod, MAKEINTRESOURCE(nIndex));
	SetCursor(hCur);
	return;
}

void GetModuleParentDir(char *szParPath){
	char szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98以降
	*PathFindFileName(szParPath) = '\0';
	return;
}

LPSTR MyPathAddBackslash(LPSTR pszPath){
	if(PathIsDirectory(pszPath)){
		return PathAddBackslash(pszPath);
	}else if(IsPlayList(pszPath) || IsArchive(pszPath)){
		return pszPath;
	}else{
		return PathAddBackslash(pszPath);
	}
}
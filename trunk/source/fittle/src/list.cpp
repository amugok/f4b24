/*
 * List.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	FILEINFO構造体のリスト構造を操作
//

#include "list.h"
#include "fittle.h"
#include "func.h"
#include "bass_tag.h"
#include "archive.h"

// ローカル関数
static void Merge(struct FILEINFO **, struct FILEINFO **);
static int CompareNode(struct FILEINFO *, struct FILEINFO *, int);

// リスト構造にセルを追加
struct FILEINFO **AddList(struct FILEINFO **ptr, LPTSTR szFileName, LPTSTR pszFileSize, LPTSTR pszFileTime){
	struct FILEINFO *NewFileInfo;

	if(*ptr==NULL){
		NewFileInfo = (struct FILEINFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct FILEINFO));
		lstrcpyn(NewFileInfo->szFilePath, szFileName, MAX_FITTLE_PATH);
		lstrcpyn(NewFileInfo->szSize, pszFileSize, 50);
		lstrcpyn(NewFileInfo->szTime, pszFileTime, 50);
		NewFileInfo->bPlayFlag = FALSE;
		NewFileInfo->pNext = *ptr;
		*ptr = NewFileInfo;
		return ptr;
	}else{
		return AddList(&((*ptr)->pNext), szFileName, pszFileSize, pszFileTime);
	}
}

// 一つのセルを削除
int DeleteAList(struct FILEINFO *pDel, struct FILEINFO **pRoot){
	struct FILEINFO **pTmp;

	for(pTmp = pRoot;*pTmp!=NULL;pTmp=&(*pTmp)->pNext){
		if(*pTmp==pDel){
			*pTmp = pDel->pNext; // 削除セルを飛ばして連結
			HeapFree(GetProcessHeap(), 0, pDel);
			break;
		}
	}
	return 0;
}

// リスト構造のセルを全て削除
int DeleteAllList(struct FILEINFO **pRoot){
	struct FILEINFO *pNxt;
	struct FILEINFO *pTmp;
	HANDLE hPH = GetProcessHeap();

	pTmp = *pRoot;
	while(pTmp){
		pNxt = pTmp->pNext;
		HeapFree(hPH, 0, pTmp);
		pTmp = pNxt;
	}
	*pRoot = NULL;
	return 0;
}

// リスト構造のセルの総数を取得
int GetListCount(struct FILEINFO *pRoot){
	int i = 0;

	while(pRoot){
		i++;
		pRoot = pRoot->pNext;
	}
	return i;
}

// インデックス -> ポインタ
struct FILEINFO *GetPtrFromIndex(struct FILEINFO *pRoot, int nIndex){
	struct FILEINFO *pTmp;
	int i;

	if(nIndex==-1) return NULL;
	pTmp = pRoot;
	for(i=1;i<=nIndex;i++){
		if(!pTmp){
			MessageBox(NULL, TEXT("リストのインデックスが不正です。"), TEXT("Fittle"), MB_OK | MB_ICONWARNING);
			break;
		}
		pTmp = pTmp->pNext;
	}
	return pTmp;
}

// ポインタ -> インデックス
int GetIndexFromPtr(struct FILEINFO *pRoot, struct FILEINFO *pTarget){
	struct FILEINFO *pTmp;
	int i = 0;
	
	if(!pTarget) return -1;
	for(pTmp=pRoot;pTmp!=NULL;pTmp=pTmp->pNext){
		if(pTmp==pTarget) return i;
		i++;
	}
	return 0; // 見つからなかった場合
}

// パス -> インデックス
int GetIndexFromPath(struct FILEINFO *pRoot, LPTSTR szFilePath){
	struct FILEINFO *pTmp;
	int i = -1;

	for(pTmp=pRoot;pTmp!=NULL;pTmp=pTmp->pNext){
		i++;
		if(lstrcmpi(pTmp->szFilePath, szFilePath)==0) return i;
	}
	return -1;
}

// すべて未再生にする
int SetUnPlayed(struct FILEINFO *pRoot){
	struct FILEINFO *pTmp = NULL;
	int i = 0;

	for(pTmp=pRoot;pTmp!=NULL;pTmp=pTmp->pNext){
		pTmp->bPlayFlag = FALSE;
		i++;
	}
	return i;
}

// 未再生インデックス -> 普通のインデックス
int GetUnPlayedIndex(struct FILEINFO *pRoot, int nIndex){
	struct FILEINFO *pTmp;
	int i = 0, j= 0;

	for(pTmp = pRoot;pTmp!=NULL;pTmp = pTmp->pNext){
		if(!pTmp->bPlayFlag)
			if(i++==nIndex) return j;
		j++;
	}
	return 0;
}

// 未再生のファイルの数を取得
int GetUnPlayedFile(struct FILEINFO *ptr){
	int i = 0;

	for(;ptr!=NULL;ptr = ptr->pNext)
		if(ptr->bPlayFlag==FALSE) i++;
	return i;
}

// リスト構造にフォルダ以下のファイルを追加
BOOL SearchFolder(struct FILEINFO **pRoot, LPTSTR pszSearchPath, BOOL bSubFlag){
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	TCHAR szSearchDirW[MAX_FITTLE_PATH];
	TCHAR szFullPath[MAX_FITTLE_PATH];
	TCHAR szSize[50], szTime[50];
	struct FILEINFO **pTale = pRoot;
	SYSTEMTIME sysTime;
	FILETIME locTime;
	
	if(!(GetFileAttributes(pszSearchPath) & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
	PathAddBackslash(pszSearchPath);
	wsprintf(szSearchDirW, TEXT("%s*.*"), pszSearchPath);
	hFind = FindFirstFile(szSearchDirW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			if(!(lstrcmp(wfd.cFileName, TEXT("."))==0 || lstrcmp(wfd.cFileName, TEXT(".."))==0)){
				if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
					if(CheckFileType(wfd.cFileName)){
						wsprintf(szFullPath, TEXT("%s%s"), pszSearchPath, wfd.cFileName); 
						wsprintf(szSize, TEXT("%d KB"), wfd.nFileSizeLow/1024);
						FileTimeToLocalFileTime(&(wfd.ftLastWriteTime), &locTime);
						FileTimeToSystemTime(&locTime, &sysTime);
						wsprintf(szTime, TEXT("%02d/%02d/%02d %02d:%02d:%02d"),
							sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
						pTale = AddList(pTale, szFullPath, szSize, szTime);
					}if(g_cfg.nZipSearch && bSubFlag && IsArchive(wfd.cFileName)){	// ZIPも検索
						wsprintf(szFullPath, TEXT("%s%s"), pszSearchPath, wfd.cFileName);
						ReadArchive(pRoot, szFullPath);
					}
				}else{
					// サブフォルダも検索
					if(bSubFlag && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)){
						wsprintf(szFullPath, TEXT("%s%s"), pszSearchPath, wfd.cFileName); 
						SearchFolder(pRoot, szFullPath, TRUE);
					}
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	return TRUE;
}

// M3Uファイルを読み込み、リストビューに追加
int ReadM3UFile(struct FILEINFO **pSub, LPTSTR pszFilePath){
	HANDLE hFile;
	DWORD dwSize = 0L;
	LPSTR lpszBuf;
	DWORD dwAccBytes;
	BOOL fUTF8 = FALSE;
	int i=0, j=0;
	CHAR szTempPath[MAX_FITTLE_PATH];

	TCHAR szCombine[MAX_FITTLE_PATH];
	TCHAR szParPath[MAX_FITTLE_PATH];

	WCHAR szWideTemp[MAX_FITTLE_PATH];
#ifdef UNICODE
#else
	CHAR szAnsiTemp[MAX_FITTLE_PATH];
#endif
	TCHAR szTstrTemp[MAX_FITTLE_PATH];

	TCHAR szTime[50] = TEXT("-"), szSize[50] = TEXT("-");
	struct FILEINFO **pTale = pSub;

#ifdef _DEBUG
		DWORD dTime;
		TCHAR szBuff[100];
		dTime = GetTickCount();
#endif

	if((hFile = CreateFile(pszFilePath, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))==INVALID_HANDLE_VALUE) return -1;
	//GetParentDir(pszFilePath, szParPath);

	lstrcpyn(szParPath, pszFilePath, MAX_FITTLE_PATH);
	*(PathFindFileName(szParPath)-1) = '\0';

	dwSize = GetFileSize(hFile, NULL);
	lpszBuf = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize + sizeof(CHAR));
	if(!lpszBuf) return -1;
	ReadFile(hFile, lpszBuf, dwSize, &dwAccBytes, NULL);
	lpszBuf[dwAccBytes] = '\0';

#ifdef _DEBUG
		wsprintf(szBuff, TEXT("ReadAlloc time: %d ms\n"), GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

	// 読み込んだバッファを処理
	if(lpszBuf[0] != '\0'){
		// UTF-8 BOM
		if (lpszBuf[0] == '\xef' && lpszBuf[1] == '\xbb' && lpszBuf[2] == '\xbf'){
			fUTF8 = TRUE;
			i = 3;
		}
		do {
			if(lpszBuf[i]=='\0' || lpszBuf[i]=='\n' || lpszBuf[i]=='\r'){
				szTempPath[j] = '\0';
				if(j != 0){
					// plsチェック
					if(!StrCmpNA(szTempPath, "File", 4) && StrStrA(szTempPath, "=")){
						lstrcpynA(szTempPath, StrStrA(szTempPath, "=") + 1, MAX_FITTLE_PATH);
					}
					MultiByteToWideChar(fUTF8 ? CP_UTF8 : CP_ACP, 0, szTempPath, -1, szWideTemp, MAX_FITTLE_PATH);
					// URLチェック
					if(IsURLPathA(szTempPath)){
						lstrcpy(szSize, TEXT("-"));
						lstrcpy(szTime, TEXT("-"));
#ifdef UNICODE
						pTale = AddList(pTale, szWideTemp, szSize, szTime);
#else
						WideCharToMultiByte(CP_ACP, 0, szWideTemp, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
						pTale = AddList(pTale, szAnsiTemp, szSize, szTime);
#endif
					}else{
						// 相対パスの処理
						//if(!(szTempPath[1] == TEXT(':') && szTempPath[2] == TEXT('\\'))){
						if(PathIsRelativeA(szTempPath)){
#ifdef UNICODE
							wsprintfW(szTstrTemp, L"%s\\%s", szParPath, szWideTemp);
#else
							WideCharToMultiByte(CP_ACP, 0, szWideTemp, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
							wsprintfA(szTstrTemp, "%s\\%s", szParPath, szAnsiTemp);
#endif
							PathCanonicalize(szCombine, szTstrTemp);
						}else{
#ifdef UNICODE
							lstrcpyn(szCombine, szWideTemp, MAX_FITTLE_PATH);
#else
							WideCharToMultiByte(CP_ACP, 0, szWideTemp, -1, szCombine, MAX_FITTLE_PATH, NULL, NULL);
#endif
						}
						if(!g_cfg.nExistCheck || (FILE_EXIST(szCombine) || IsArchivePath(szCombine)))
						{
							if(g_cfg.nTimeInList){
								GetTimeAndSize(szCombine, szSize, szTime);
							}
							pTale = AddList(pTale, szCombine, szSize, szTime);
						}
					}
				}
				j=0;
			}else{
				szTempPath[j] = lpszBuf[i];
				j++;
			}
		}while(lpszBuf[i++] != '\0');
	}
	CloseHandle(hFile);
	HeapFree(GetProcessHeap(), 0, lpszBuf);

#ifdef _DEBUG
		wsprintf(szBuff, TEXT("ReadPlaylist time: %d ms\n"), GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

	return 0;
}

// リストをM3Uファイルで保存
BOOL WriteM3UFile(struct FILEINFO *pRoot, LPTSTR szFile, int nMode){
	FILEINFO *pTmp;
	HANDLE hFile;
	DWORD dwAccBytes;
	TCHAR szLine[MAX_FITTLE_PATH];
#ifdef UNICODE
#else
	WCHAR szWideTemp[MAX_FITTLE_PATH];
#endif
	CHAR szAnsiTemp[MAX_FITTLE_PATH * 3];

		// ファイルの書き込み
	hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if(nMode==3 || nMode==4){
		WriteFile(hFile, "\xef\xbb\xbf", 3, &dwAccBytes, NULL);
	}

	// ここでszBuffに保存内容を転写
	for(pTmp = pRoot; pTmp!=NULL; pTmp = pTmp->pNext){
		if(nMode==2 || nMode==4){
			// 相対パスの処理
			PathRelativePathTo(szLine, szFile, 0, pTmp->szFilePath, 0);
		}else{
			lstrcpyn(szLine, pTmp->szFilePath, MAX_FITTLE_PATH);
		}
		if(nMode==3 || nMode==4){
#ifdef UNICODE
			WideCharToMultiByte(CP_UTF8, 0, szLine, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
#else
			MultiByteToWideChar(CP_ACP, 0, szLine, -1, szWideTemp, MAX_FITTLE_PATH);
			WideCharToMultiByte(CP_UTF8, 0, szWideTemp, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
#endif
			WriteFile(hFile, szAnsiTemp, lstrlenA(szAnsiTemp), &dwAccBytes, NULL);
		}else{
#ifdef UNICODE
			WideCharToMultiByte(CP_ACP, 0, szLine, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
			WriteFile(hFile, szAnsiTemp, lstrlenA(szAnsiTemp), &dwAccBytes, NULL);
#else
			WriteFile(hFile, szLine, lstrlenA(szLine), &dwAccBytes, NULL);
#endif
		}

		WriteFile(hFile, "\r\n", 2, &dwAccBytes, NULL);
	}
	CloseHandle(hFile);
	return TRUE;
}

static LPTSTR SafePathFindExtensionPlusOne(LPTSTR lpszPath){
	static TCHAR safezero[4] = { 0,0,0,0 };
	LPTSTR p = PathFindExtension(lpszPath);
	return (p && *p) ? p + 1 : safezero;
}

int CompareNode(struct FILEINFO *pLeft, struct FILEINFO *pRight, int nSortType){
	switch(nSortType){
		case 0:		// フルパス
			return lstrcmpi(pLeft->szFilePath, pRight->szFilePath);
		case 1:		// ファイル名昇順
			return lstrcmpi(GetFileName(pLeft->szFilePath), GetFileName(pRight->szFilePath));
		case -1:	// ファイル名降順
			return -1*lstrcmpi(GetFileName(pLeft->szFilePath), GetFileName(pRight->szFilePath));
		case 2:		// サイズ昇順
			return StrToInt(pLeft->szSize) - StrToInt(pRight->szSize);
		case -2:	// サイズ降順
			return StrToInt(pRight->szSize) - StrToInt(pLeft->szSize);
		case 3:		// 種類昇順
			return lstrcmpi(IsURLPath(pLeft->szFilePath)?TEXT("URL"):SafePathFindExtensionPlusOne(pLeft->szFilePath), StrStr(pRight->szFilePath, TEXT("://"))?TEXT("URL"):SafePathFindExtensionPlusOne(pRight->szFilePath));
		case -3:	// 種類昇順
			return -1*lstrcmpi(IsURLPath(pLeft->szFilePath)?TEXT("URL"):SafePathFindExtensionPlusOne(pLeft->szFilePath), StrStr(pRight->szFilePath, TEXT("://"))?TEXT("URL"):SafePathFindExtensionPlusOne(pRight->szFilePath));
		case 4:		// 更新日時昇順
			return lstrcmp(pLeft->szTime, pRight->szTime);
		case -4:	// 更新日時降順
			return -1*lstrcmp(pLeft->szTime, pRight->szTime);
		default:
			return 0;
	}
}

// 二つのリストをマージ。ppLeftがマージ後のルートになる
void Merge(struct FILEINFO **ppLeft, struct FILEINFO **ppRight, int nSortType){
	struct FILEINFO *pNewRoot;
	struct FILEINFO *pNow;

	// 頭の処理
	if(!(*ppRight)
	|| ((*ppLeft) && CompareNode(*ppLeft, *ppRight, nSortType)<=0)){
		pNewRoot = pNow = *ppLeft;
		*ppLeft = (*ppLeft)->pNext;
	}else{
		pNewRoot = pNow = *ppRight;
		*ppRight = (*ppRight)->pNext;
	}

	// それ以降の処理
	while(*ppLeft || *ppRight){
		if(!(*ppRight)
		|| ((*ppLeft) && CompareNode(*ppLeft, *ppRight, nSortType)<=0)){
			pNow->pNext = *ppLeft;	// pNowに連結
			pNow = *ppLeft;			// pNowを移行
			*ppLeft = (*ppLeft)->pNext;	// ppLeftを削除
		}else{
			pNow->pNext = *ppRight;	// pNowに連結
			pNow = *ppRight;			// pNowを移行
			*ppRight = (*ppRight)->pNext;	// ppLeftを削除
		}
	}

	// ppLeftに返してやる
	*ppLeft = pNewRoot;	
	return;
}

// リスト構造をマージソート
void MergeSort(struct FILEINFO **ppRoot, int nSortType){
	struct FILEINFO *pLeft;
	struct FILEINFO *pRight;
	struct FILEINFO *pMiddle;
	int nCount;

	nCount = GetListCount(*ppRoot);
	if(nCount<=1) return;

	// リストを二分割
	pLeft = *ppRoot;
	pMiddle = GetPtrFromIndex(*ppRoot, nCount/2-1);
	pRight = pMiddle->pNext;
	pMiddle->pNext = NULL;

	// 再帰的呼び出し
	MergeSort(&pLeft, nSortType);
	MergeSort(&pRight, nSortType);

	// 二つのリストをマージ
	Merge(&pLeft, &pRight, nSortType);
	*ppRoot = pLeft;

	return;
}

int SearchFiles(struct FILEINFO **ppRoot, LPTSTR szFilePath, BOOL bSub){
	TCHAR szTime[50], szSize[50];

#ifdef _DEBUG
		DWORD dTime;
		TCHAR szBuff[100];
		dTime = GetTickCount();
#endif

	if(PathIsDirectory(szFilePath)){
		SearchFolder(ppRoot, szFilePath, bSub);	/* フォルダ */

#ifdef _DEBUG
		wsprintf(szBuff, TEXT("ReadFolder time: %d ms\n"), GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

		return FOLDERS;
	}else if(IsPlayList(szFilePath)){
		ReadM3UFile(ppRoot, szFilePath);			/* プレイリスト */

//#ifdef _DEBUG
//		wsprintf(szBuff, TEXT("ReadPlaylist time: %d ms\n"), GetTickCount() - dTime);
//		OutputDebugString(szBuff);
//#endif

		return LISTS;
	}else if(IsArchive(szFilePath)){
		ReadArchive(ppRoot, szFilePath);			/* アーカイブ */

#ifdef _DEBUG
		wsprintf(szBuff, TEXT("ReadArchive time: %d ms\n"), GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

		return ARCHIVES;
	}else if(FILE_EXIST(szFilePath) && CheckFileType(szFilePath)){
		GetTimeAndSize(szFilePath, szSize, szTime);		/* 単体ファイル */
		AddList(ppRoot, szFilePath, szSize, szTime);
		return FILES;
	}else if(IsURLPath(szFilePath)){
		AddList(ppRoot, szFilePath, TEXT("-"), TEXT("-"));
		return URLS;
	}
	return OTHERS;
}

int LinkCheck(struct FILEINFO **ppRoot){
	struct FILEINFO *pTmp;
	struct FILEINFO *pDel;
	int i=0;

	for(pTmp = *ppRoot;pTmp;){
		if(!FILE_EXIST(pTmp->szFilePath) && !IsArchivePath(pTmp->szFilePath) && !IsURLPath(pTmp->szFilePath)){
			pDel = pTmp;
			pTmp = pTmp->pNext;
			DeleteAList(pDel, ppRoot);
		}else{
			pTmp = pTmp->pNext;
			i++;
		}
	}
	return i;
}
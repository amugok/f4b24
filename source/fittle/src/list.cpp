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
static void CALLBACK LXAddColumn(HWND hList, int nColumn, LPVOID pLabel, int nWidth, int nFmt);
static LPVOID CALLBACK LXGetFileName(LPVOID pFileInfo);
static BOOL CALLBACK LXCheckPath(LPVOID pFileInfo, int nCheck);
static int CALLBACK LXStrCmp(LPCVOID pStrLeft, LPCVOID pStrRight);

#include "f4b24lx.h"

static F4B24LX_INTERFACE m_lxif = {
	1,
	38,
#ifdef UNICODE
	1,
#else
	0,
#endif
	LXLoadMusic,
	LXFreeMusic,
	LXGetTag,
	LXAddColumn,
	LXGetFileName,
	LXCheckPath,
	LXStrCmp,
	0
};

static const BYTE m_aDefaultColumns[] = { 0, 1, 2, 3 };
static int m_nNumColumns = 0;
static LPBYTE m_pColumnTable = NULL;


// リスト構造にセルを追加
struct FILEINFO **AddList(struct FILEINFO **ptr, LPTSTR szFileName, LPTSTR pszFileSize, LPTSTR pszFileTime){
	struct FILEINFO *NewFileInfo;

	if(*ptr==NULL){
		NewFileInfo = (struct FILEINFO *)HZAlloc(sizeof(struct FILEINFO));
		lstrcpyn(NewFileInfo->szFilePath, szFileName, MAX_FITTLE_PATH);
		lstrcpyn(NewFileInfo->szSize, pszFileSize, 50);
		lstrcpyn(NewFileInfo->szTime, pszFileTime, 50);
		NewFileInfo->bPlayFlag = FALSE;
		NewFileInfo->pNext = *ptr;
		*ptr = NewFileInfo;
		if (m_lxif.HookOnAlloc) m_lxif.HookOnAlloc(NewFileInfo);
		return ptr;
	}else{
		return AddList(&((*ptr)->pNext), szFileName, pszFileSize, pszFileTime);
	}
}

static void FreeListNode(FILEINFO *pDel){
	if (m_lxif.HookOnFree) m_lxif.HookOnFree(pDel);
	HFree(pDel);
}

// 一つのセルを削除
int DeleteAList(struct FILEINFO *pDel, struct FILEINFO **pRoot){
	struct FILEINFO **pTmp;

	for(pTmp = pRoot;*pTmp!=NULL;pTmp=&(*pTmp)->pNext){
		if(*pTmp==pDel){
			if (pDel == g_pNextFile) g_pNextFile = NULL;
			*pTmp = pDel->pNext; // 削除セルを飛ばして連結
			FreeListNode(pDel);
			break;
		}
	}
	return 0;
}

// リスト構造のセルを全て削除
int DeleteAllList(struct FILEINFO **pRoot){
	struct FILEINFO *pNxt;
	struct FILEINFO *pTmp;

	pTmp = *pRoot;
	while(pTmp){
		if (pTmp == g_pNextFile) g_pNextFile = NULL;
		pNxt = pTmp->pNext;
		FreeListNode(pTmp);
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
						FormatDateTime(szTime, &(wfd.ftLastWriteTime));
						pTale = AddList(pTale, szFullPath, szSize, szTime);
					}if(g_cfg.nZipSearch && bSubFlag && IsArchiveFast(wfd.cFileName)){	// ZIPも検索
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

	WASTR szPath;
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

	WAstrcpyT(&szPath, pszFilePath);
	hFile = WAOpenFile(&szPath);
	if(hFile == INVALID_HANDLE_VALUE) return -1;

	lstrcpyn(szParPath, pszFilePath, MAX_FITTLE_PATH);
	*(PathFindFileName(szParPath)-1) = '\0';

	dwSize = GetFileSize(hFile, NULL);
	lpszBuf = (LPSTR)HZAlloc(dwSize + sizeof(CHAR));
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
						lstrcpynWt(szAnsiTemp, szWideTemp, MAX_FITTLE_PATH);
						pTale = AddList(pTale, szAnsiTemp, szSize, szTime);
#endif
					}else{
						// 相対パスの処理
						//if(!(szTempPath[1] == TEXT(':') && szTempPath[2] == TEXT('\\'))){
						if(PathIsRelativeA(szTempPath)){
#ifdef UNICODE
							wsprintfW(szTstrTemp, L"%s\\%s", szParPath, szWideTemp);
#else
							wsprintfA(szTstrTemp, "%s\\%S", szParPath, szWideTemp);
#endif
							PathCanonicalize(szCombine, szTstrTemp);
						}else{
							lstrcpynWt(szCombine, szWideTemp, MAX_FITTLE_PATH);
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
	HFree(lpszBuf);

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

	WASTR szPath;
	TCHAR szLine[MAX_FITTLE_PATH];
#ifdef UNICODE
#else
	WCHAR szWideTemp[MAX_FITTLE_PATH];
#endif
	CHAR szAnsiTemp[MAX_FITTLE_PATH * 3];

	// ファイルの書き込み
	WAstrcpyT(&szPath, szFile);
	hFile = WACreateFile(&szPath);
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
			lstrcpyntW(szWideTemp, szLine, MAX_FITTLE_PATH);
			WideCharToMultiByte(CP_UTF8, 0, szWideTemp, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
#endif
			WriteFile(hFile, szAnsiTemp, lstrlenA(szAnsiTemp), &dwAccBytes, NULL);
		}else{
#ifdef UNICODE
			lstrcpyntA(szAnsiTemp, szLine, MAX_FITTLE_PATH);
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
	}else if(IsPlayListFast(szFilePath)){
		ReadM3UFile(ppRoot, szFilePath);			/* プレイリスト */

//#ifdef _DEBUG
//		wsprintf(szBuff, TEXT("ReadPlaylist time: %d ms\n"), GetTickCount() - dTime);
//		OutputDebugString(szBuff);
//#endif

		return LISTS;
	}else if(IsArchiveFast(szFilePath)){
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

int GetColumnNum(){
	return m_pColumnTable ? m_nNumColumns : (sizeof(m_aDefaultColumns)/sizeof(m_aDefaultColumns[0]));
}
int GetColumnType(int nColumn) {
	if (nColumn < GetColumnNum())
		return m_pColumnTable ? m_pColumnTable[nColumn] : m_aDefaultColumns[nColumn];
	return -1;
}

static void AddListColumn(HWND hList, int c, LPTSTR l, int w, int fmt){
	LVCOLUMN lvcol;
	lvcol.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcol.fmt = fmt;
	lvcol.cx = w;
	lvcol.iSubItem = 0;
	lvcol.pszText = l;
	ListView_InsertColumn(hList, c, &lvcol);
}

static void CALLBACK LXAddColumn(HWND hList, int nColumn, LPVOID pLabel, int nWidth, int nFmt){
	AddListColumn(hList, nColumn, (LPTSTR)pLabel, nWidth, nFmt);
}

static LPVOID CALLBACK LXGetFileName(LPVOID pFileInfo){
	struct FILEINFO *pItem = (struct FILEINFO *)pFileInfo;
	return GetFileName(pItem->szFilePath);
}
	
static BOOL CALLBACK LXCheckPath(LPVOID pFileInfo, int nCheck){
	struct FILEINFO *pItem = (struct FILEINFO *)pFileInfo;
	switch (nCheck) {
	case F4B24LX_CHECK_URL:
		return StrStrI(pItem->szFilePath, TEXT("://")) != NULL;
	case F4B24LX_CHECK_CUE:
		return StrStrI(pItem->szFilePath, TEXT(".cue/")) != NULL;
	case F4B24LX_CHECK_ARC:
		return StrStrI(pItem->szFilePath, TEXT("/")) != NULL;
//		return IsArchivePath(pItem->szFilePath);
	}
	return FALSE;
}

static int CALLBACK LXStrCmp(LPCVOID pStrLeft, LPCVOID pStrRight){
	if (g_cfg.nListSort == 2) {
		static int (CALLBACK *pStrCmpLogicalW)(LPCWSTR psz1, LPCWSTR psz2) = 0;
		if (!pStrCmpLogicalW)
			pStrCmpLogicalW = (int (CALLBACK *)(LPCWSTR psz1, LPCWSTR psz2))GetProcAddress(GetModuleHandleA("shlwapi.dll"), "StrCmpLogicalW");
		if (pStrCmpLogicalW){
#ifdef UNICODE
			return pStrCmpLogicalW((LPCWSTR)pStrLeft, (LPCWSTR)pStrRight);
#else
			WCHAR bufL[MAX_PATH];
			WCHAR bufR[MAX_PATH];
			lstrcpyntW(bufL, (LPCSTR)pStrLeft, MAX_PATH);
			lstrcpyntW(bufR, (LPCSTR)pStrRight, MAX_PATH);
			return pStrCmpLogicalW(bufL, bufR);
#endif
		}
		return lstrcmpi((LPCTSTR)pStrLeft, (LPCTSTR)pStrRight);
	}
	if (g_cfg.nListSort == 1)
		return lstrcmp((LPCTSTR)pStrLeft, (LPCTSTR)pStrRight);
	return lstrcmpi((LPCTSTR)pStrLeft, (LPCTSTR)pStrRight);
}

static void AddStandardColumn(HWND hList, int c, LPTSTR l, int w, int t){
	char szKey[7] = "Width0";
	szKey[5] = '0' + t;
	AddListColumn(hList, c, l, WAGetIniInt("Column", szKey, w), t == 1 ? LVCFMT_RIGHT : LVCFMT_LEFT);
}

static LPCTSTR GetPathType(struct FILEINFO *pInfo){
	return IsURLPath(pInfo->szFilePath) ? TEXT("URL") : SafePathFindExtensionPlusOne(pInfo->szFilePath);
}

static int CompareNode(struct FILEINFO *pLeft, struct FILEINFO *pRight, int nSortType){
	int nColumn;
	int nType;
	if (nSortType == 0)
		return lstrcmpi(pLeft->szFilePath, pRight->szFilePath);	// フルパス
	else if (nSortType < 0)
		return -CompareNode(pLeft, pRight, -nSortType);	// 逆順
	nColumn = nSortType - 1;
	nType = GetColumnType(nColumn);
	switch(nType){
		case 0:		// ファイル名昇順
			return LXStrCmp(GetFileName(pLeft->szFilePath), GetFileName(pRight->szFilePath));
		case 1:		// サイズ昇順
			return StrToInt(pLeft->szSize) - StrToInt(pRight->szSize);
		case 2:		// 種類昇順
			return LXStrCmp(GetPathType(pLeft), GetPathType(pRight));
		case 3:		// 更新日時昇順
			return lstrcmp(pLeft->szTime, pRight->szTime);
		default:
			if (m_lxif.HookCompareColumnText) return m_lxif.HookCompareColumnText(pLeft, pRight, nColumn, nType);
			break;
	}
	return 0;
}

void GetColumnText(struct FILEINFO *pTmp, int nRow, int nColumn, LPTSTR pWork, int nWorkMax){
	LPCTSTR pText = 0;
	struct FILEINFO *pItem = GetPtrFromIndex(pTmp, nRow);
	int nType = GetColumnType(nColumn);
	switch(nType){
		case 0:
			pText = GetFileName(pItem->szFilePath);
			break;
		case 1:
			pText = pItem->szSize;
			break;
		case 2:
			LPTSTR pszPath;
			pszPath = pItem->szFilePath;
			if(IsURLPath(pszPath)){
				pText = TEXT("URL");
			}else if(IsArchivePath(pszPath) && GetArchiveItemType(pszPath, pWork, nWorkMax)){
				return;
			}else{
				LPTSTR p = PathFindExtension(pszPath);
				if (p && *p) pText = p + 1;
			}
			break;
		case 3:
			pText = pItem->szTime;
			break;
		default:
			if (m_lxif.HookGetColumnText) m_lxif.HookGetColumnText(pItem, nRow, nColumn, nType, pWork, nWorkMax);
			break;
	}
	if (pText) lstrcpyn(pWork, pText, nWorkMax);
}

static void AddColumn(HWND hList, int nColumn){
	int nType = GetColumnType(nColumn);
	switch(nType) {
	case 0:
		AddStandardColumn(hList, nColumn, TEXT("ファイル名"), 200, 0);
		break;
	case 1:
		AddStandardColumn(hList, nColumn, TEXT("サイズ"), 70, 1);
		break;
	case 2:
		AddStandardColumn(hList, nColumn, TEXT("種類"), 40, 2);
		break;
	case 3:
		AddStandardColumn(hList, nColumn, TEXT("更新日時"), 130, 3);
		break;
	default:
		if (m_lxif.HookAddColumn) m_lxif.HookAddColumn(hList, nColumn, nType);
		break;
	}
}

void AddColumns(HWND hList){
	int nMax = GetColumnNum();
	int c;
	for (c = 0; c < nMax; c++)
		AddColumn(hList, c);
}

void LoadColumnsOrder(){
	int nColumnNum = WAGetIniInt("Column", "ColumnNum", 0);
	if (m_pColumnTable) {
		HFree(m_pColumnTable);
		m_pColumnTable = 0;
	}
	if (nColumnNum) {
		int c;
		int nColumn;
		m_pColumnTable = (LPBYTE)HZAlloc(sizeof(BYTE) * nColumnNum);
		for (c = nColumn = 0; nColumn < nColumnNum; nColumn++){
			CHAR szSec[16];
			int nType;
			wsprintfA(szSec, "ColumnType%d", nColumn);
			nType = WAGetIniInt("Column", szSec, -1);
			if ((nType >= 0) && ((nType < 4) || (m_lxif.HookInitColumnOrder && m_lxif.HookInitColumnOrder(c, nType)))){
				m_pColumnTable[c++] = nType;
			}
		}
		m_nNumColumns = c;
	}
}
void SaveColumnsOrder(HWND hList){
	if (m_lxif.HookOnSave)
	{
		int nMax = GetColumnNum();
		int c;
		for (c = 0; c < nMax; c++) {
			m_lxif.HookOnSave(hList, c, GetColumnType(c), ListView_GetColumnWidth(hList, c));
		}
	}
}

LPVOID GetLXIf(){
	return &m_lxif;
}

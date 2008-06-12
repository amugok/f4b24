/*
 * Archive.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "bass_tag.h"
#include "archive.h"
#include "list.h"
#include "func.h"
#include "aplugin.h"

static ARCHIVE_PLUGIN_INFO *pTop = NULL;

/* 書庫プラグインを登録 */
static BOOL RegisterArchivePlugin(FARPROC lpfnProc, HWND hwndMain, HMODULE hmodPlugin){
	GetAPluginInfoFunc GetAPluginInfo = (GetAPluginInfoFunc)lpfnProc;
	if (GetAPluginInfo){
		ARCHIVE_PLUGIN_INFO *pNewPlugins = GetAPluginInfo();
		if (pNewPlugins){
			ARCHIVE_PLUGIN_INFO *pNext = pNewPlugins;
			do{
				if (pNext->nAPDKVer >= 1)
				{
					pNext->hwndMain = hwndMain;
					pNext->hmodPlugin = hmodPlugin;
				}
				pNext = pNext->pNext;
			} while (pNext);
			for (pNext = pNewPlugins; pNext->pNext; pNext = pNext->pNext);
			pNext->pNext = pTop;
			pTop = pNewPlugins;
			return TRUE;
		}
	}
	return FALSE;
}

/* 書庫パスから書庫部分をコピーし書庫内ファイル名部分を返す */
static LPTSTR GetArchivePath(LPTSTR pDst, LPTSTR pSrc, int nDstMax){
	int i;
	for(i = 0;i < nDstMax - 1 && pSrc[i] && pSrc[i] != TEXT('/'); i++){
		pDst[i] = pSrc[i];
	}
	pDst[i] = TEXT('\0');
	if (pSrc[i] != '/') return NULL;
	return pSrc + i + 1;
}


/* 書庫ファイルのパスに対応するプラグインを取得 */
ARCHIVE_PLUGIN_INFO *GetAPlugin(char *szFilePath){
	char *p;

	if(!pTop) return NULL;

	if(!PathIsDirectory(szFilePath)){
		if((p = PathFindExtension(szFilePath)) != NULL && *p){
			ARCHIVE_PLUGIN_INFO *pPlugin = pTop;
			p++;
			while (pPlugin)
			{
				if (pPlugin->IsArchiveExt(p)) return pPlugin;
				pPlugin = pPlugin->pNext;
			}
		}
	}
	return NULL;
}

/* アーカイブかどうか判断 */
BOOL IsArchive(char *szFilePath)
{
	return GetAPlugin(szFilePath) != NULL;
}

static char *CheckArchivePath(char *pszFilePath){
	ARCHIVE_PLUGIN_INFO *pPlugin = pTop;
	while (pPlugin)
	{
		char *pRet = pPlugin->CheckArchivePath(pszFilePath);
		if (pRet) return pRet;
		pPlugin = pPlugin->pNext;
	}
	return 0;
}

/* アーカイブパスか判断 */
BOOL IsArchivePath(char *pszFilePath){
	char szArchivePath[MAX_PATH];
	char *p = CheckArchivePath(pszFilePath);
	return p && GetArchivePath(szArchivePath, pszFilePath, MAX_PATH) && FILE_EXIST(szArchivePath);
}

typedef struct {
	struct FILEINFO **pTale;
	char *pszFilePath;
} ENUMWORK;

static void CALLBACK AddListProc(char *pszFileName, char *pszSize, char *pszTime, void *pData){
	ENUMWORK *pWork = (ENUMWORK *)pData;
	pWork->pTale = AddList(pWork->pTale, pszFileName, pszSize, pszTime);
}

static BOOL CALLBACK CheckFileTypeProc(char *pszFileName){
	return CheckFileType(pszFileName);
}

static void CALLBACK ArchiveEnumProc(char *pszFileName, DWORD dwSize, FILETIME ft, void *pData){
	ENUMWORK *pWork = (ENUMWORK *)pData;
	char szTime[50] = "-", szSize[50] = "-";
	char szFullPath[MAX_FITTLE_PATH];
	if(CheckFileType(pszFileName)){
		SYSTEMTIME st;
		size_t l;
		FileTimeToSystemTime(&ft, &st);
		GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime, 50);
		l = lstrlen(szTime);
		if (l < 50) szTime[l++] = ' ';
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime + l, 50 - l);
		wsprintf(szFullPath, "%s/%s", pWork->pszFilePath, pszFileName);
		wsprintf(szSize, "%d KB", dwSize/1024);
		pWork->pTale = AddList(pWork->pTale, szFullPath, szSize, szTime);
	}
}

/* 圧縮ファイル内のファイル一覧を取得 */
BOOL ReadArchive(struct FILEINFO **pSub, char *pszFilePath){
	ARCHIVE_PLUGIN_INFO *pPlugin = GetAPlugin(pszFilePath);
	ENUMWORK ew;
	if (!pPlugin) return FALSE;
	ew.pTale = pSub;
	ew.pszFilePath = pszFilePath;
	if (pPlugin->nAPDKVer >= 2 && pPlugin->EnumArchive2){
		return pPlugin->EnumArchive2(pszFilePath, &AddListProc, &CheckFileTypeProc, &ew);
	}
	return pPlugin->EnumArchive(pszFilePath, &ArchiveEnumProc, &ew);
}

/* 圧縮ファイルをメモリに展開 */
BOOL AnalyzeArchivePath(CHANNELINFO *pInfo, char *pszArchivePath, char *pszStart, char *pszEnd){
	char *p = GetArchivePath(pszArchivePath, p=pInfo->szFilePath, MAX_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(pszArchivePath) : 0;

	void *pBuf;
	DWORD dwSize;

	if (!pPlugin) return FALSE;

	if (pPlugin->nAPDKVer >= 2 && pPlugin->ResolveIndirect){
		if (pPlugin->ResolveIndirect(pszArchivePath, p, pszStart, pszEnd)){
			pInfo->qDuration = 0;
			return TRUE;
		}
		return FALSE;
	}

	if (pPlugin->ExtractArchive(pszArchivePath, p, &pBuf, &dwSize)){
		pInfo->qDuration = dwSize;
		pInfo->pBuff = (LPBYTE)pBuf;
		return TRUE;
	}
	return FALSE;
}

BOOL InitArchive(char *pszPath, HWND hWnd){
	char szPathW[MAX_PATH];
	char szDllPath[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd = {0};

	//lstrcat(pszPath, "Plugins");
	wsprintf(szPathW, "%s*.fap", pszPath);

	hFind = FindFirstFile(szPathW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			HINSTANCE hDll;
			wsprintf(szDllPath, "%s%s", pszPath, wfd.cFileName);
			hDll = LoadLibrary(szDllPath);
			if(hDll){
				FARPROC pfnAPlugin = GetProcAddress(hDll, "GetAPluginInfo");
				if (!pfnAPlugin || !RegisterArchivePlugin(pfnAPlugin, hWnd, hDll))
				{
					FreeLibrary(hDll);
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}

	return TRUE;
}

int GetArchiveIconIndex(char *pszPath){
	SHFILEINFO shfinfo = {0};
	char szArchivePath[MAX_FITTLE_PATH];
	GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	SHGetFileInfo(szArchivePath, FILE_ATTRIBUTE_NORMAL, &shfinfo, sizeof(SHFILEINFO),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
	if (shfinfo.hIcon) DestroyIcon(shfinfo.hIcon);
	return shfinfo.iIcon;
}

BOOL GetArchiveTagInfo(LPSTR pszPath, TAGINFO *pTagInfo){
	char szArchivePath[MAX_FITTLE_PATH];
	char *p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 2 || !pPlugin->GetBasicTag) return FALSE;
	ZeroMemory(pTagInfo, sizeof(TAGINFO));
	return pPlugin->GetBasicTag(szArchivePath, p, pTagInfo->szTrack, pTagInfo->szTitle, pTagInfo->szAlbum, pTagInfo->szArtist);
}

HICON GetArchiveItemIcon(char *pszPath){
	char szStart[100], szEnd[100];
	SHFILEINFO shfinfo = {0};
	char szArchivePath[MAX_FITTLE_PATH];
	char *p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 2 || !pPlugin->ResolveIndirect || !pPlugin->ResolveIndirect(szArchivePath, p, szStart, szEnd)){
		lstrcpyn(szArchivePath, pszPath, MAX_PATH);
	}
	SHGetFileInfo(szArchivePath, FILE_ATTRIBUTE_NORMAL, &shfinfo, sizeof(shfinfo), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON); 
	return shfinfo.hIcon;
}

BOOL GetArchiveItemType(char *pszPath, char *pBuf, int nBufMax){
	char szArchivePath[MAX_FITTLE_PATH];
	char *p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 2 || !pPlugin->GetItemType){
		return FALSE;
	}
	return pPlugin->GetItemType(szArchivePath, p, pBuf, nBufMax);
}

char *GetArchiveItemFileName(char *pszPath){
	char szArchivePath[MAX_FITTLE_PATH];
	char *p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 2 || !pPlugin->GetItemFileName){
		return 0;
	}
	return pPlugin->GetItemFileName(szArchivePath, p);
}

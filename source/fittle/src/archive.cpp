/*
 * Archive.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

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
	char *p;
	BOOL b;

	p = CheckArchivePath(pszFilePath);
	if(!p) return FALSE;

	*(p+4) = '\0';
	b = FILE_EXIST(pszFilePath);
	*(p+4) = '/';
	return b;
}

typedef struct {
	struct FILEINFO **pTale;
	char *pszFilePath;
} ENUMWORK;

static void CALLBACK ArchiveEnumProc(char *pszFileName, DWORD dwSize, FILETIME ft, void *pData)
{
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
	ew.pTale = pSub;
	ew.pszFilePath = pszFilePath;
	return pPlugin ? pPlugin->EnumArchive(pszFilePath, &ArchiveEnumProc, &ew) : FALSE;
}

/* 圧縮ファイルをメモリに展開 */
BOOL AnalyzeArchivePath(CHANNELINFO *pInfo, char *pszArchivePath){
	ARCHIVE_PLUGIN_INFO *pPlugin;
	void *pBuf;
	DWORD dwSize;
	char *p;
	int i=0;

	/* アーカイブを含むパスを分解 */
	for(p=pInfo->szFilePath;*p;p++){
		if(*p=='/')	break;
		pszArchivePath[i++] = *p;
	}
	if (*p!='/') return FALSE;
	pszArchivePath[i] = '\0';
	p++;

	pPlugin = GetAPlugin(pszArchivePath);

	if (pPlugin && pPlugin->ExtractArchive(pszArchivePath, p, &pBuf, &dwSize)){
		pInfo->qDuration = dwSize;
		pInfo->pBuff = (LPBYTE)pBuf;
		return TRUE;
	}
	return FALSE;
}

BOOL IsArchiveInitialized()
{
/* 
	return pTop != NULL;
*/
	return TRUE;
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



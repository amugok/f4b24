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
#define APDK_VER 3
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
				if (pNext->nAPDKVer >= 1){
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
	if (pSrc[i] != TEXT('/')) return NULL;
	return pSrc + i + 1;
}


/* 書庫ファイルのパスに対応するプラグインを取得 */
ARCHIVE_PLUGIN_INFO *GetAPlugin(LPTSTR szFilePath){
	LPTSTR p;

	if(!pTop) return NULL;

	if(!PathIsDirectory(szFilePath)){
		if((p = PathFindExtension(szFilePath)) != NULL && *p){
			ARCHIVE_PLUGIN_INFO *pPlugin = pTop;
			p++;
			while (pPlugin){
				if (pPlugin->IsArchiveExt(p)) return pPlugin;
				pPlugin = pPlugin->pNext;
			}
		}
	}
	return NULL;
}

/* アーカイブかどうか判断 */
BOOL IsArchive(LPTSTR szFilePath){
	return GetAPlugin(szFilePath) != NULL;
}

static LPTSTR CheckArchivePath(LPTSTR pszFilePath){
	ARCHIVE_PLUGIN_INFO *pPlugin = pTop;
	while (pPlugin){
		LPTSTR pRet = pPlugin->CheckArchivePath(pszFilePath);
		if (pRet) return pRet;
		pPlugin = pPlugin->pNext;
	}
	return 0;
}

/* アーカイブパスか判断 */
BOOL IsArchivePath(LPTSTR pszFilePath){
	TCHAR szArchivePath[MAX_PATH];
	LPTSTR p = CheckArchivePath(pszFilePath);
	return p && GetArchivePath(szArchivePath, pszFilePath, MAX_PATH) && FILE_EXIST(szArchivePath);
}

typedef struct {
	struct FILEINFO **pTale;
	LPTSTR pszFilePath;
} ENUMWORK;

static void CALLBACK AddListProc(LPTSTR pszFileName, LPTSTR pszSize, LPTSTR pszTime, void *pData){
	ENUMWORK *pWork = (ENUMWORK *)pData;
	pWork->pTale = AddList(pWork->pTale, pszFileName, pszSize, pszTime);
}

static BOOL CALLBACK CheckFileTypeProc(LPTSTR pszFileName){
	return CheckFileType(pszFileName);
}

static void CALLBACK ArchiveEnumProc(LPTSTR pszFileName, DWORD dwSize, FILETIME ft, void *pData){
	ENUMWORK *pWork = (ENUMWORK *)pData;
	TCHAR szTime[50] = TEXT("-"), szSize[50] = TEXT("-");
	TCHAR szFullPath[MAX_FITTLE_PATH];
	if(CheckFileType(pszFileName)){
		SYSTEMTIME st;
		size_t l;
		FileTimeToSystemTime(&ft, &st);
		GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime, 50);
		l = lstrlen(szTime);
		if (l < 50) szTime[l++] = TEXT(' ');
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime + l, 50 - l);
		wsprintf(szFullPath, TEXT("%s/%s"), pWork->pszFilePath, pszFileName);
		wsprintf(szSize, TEXT("%d KB"), dwSize/1024);
		pWork->pTale = AddList(pWork->pTale, szFullPath, szSize, szTime);
	}
}

/* 圧縮ファイル内のファイル一覧を取得 */
BOOL ReadArchive(struct FILEINFO **pSub, LPTSTR pszFilePath){
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
BOOL AnalyzeArchivePath(CHANNELINFO *pInfo, LPTSTR pszArchivePath, LPTSTR pszStart, LPTSTR pszEnd){
	LPTSTR p = GetArchivePath(pszArchivePath, p=pInfo->szFilePath, MAX_PATH);
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

BOOL InitArchive(LPTSTR pszPath, HWND hWnd){
	TCHAR szPathW[MAX_PATH];
	TCHAR szDllPath[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd = {0};

	//lstrcat(pszPath, "Plugins");
	wsprintf(szPathW, TEXT("%s*.fap"), pszPath);

	hFind = FindFirstFile(szPathW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			HINSTANCE hDll;
			wsprintf(szDllPath, TEXT("%s%s"), pszPath, wfd.cFileName);
			hDll = LoadLibrary(szDllPath);
			if(hDll){
#ifdef UNICODE
				FARPROC pfnAPlugin = GetProcAddress(hDll, "GetAPluginInfoW");
#else
				FARPROC pfnAPlugin = GetProcAddress(hDll, "GetAPluginInfo");
#endif
				if (!pfnAPlugin || !RegisterArchivePlugin(pfnAPlugin, hWnd, hDll)){
					FreeLibrary(hDll);
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}

	return TRUE;
}

int GetArchiveIconIndex(LPTSTR pszPath){
	SHFILEINFO shfinfo = {0};
	TCHAR szArchivePath[MAX_FITTLE_PATH];
	GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	SHGetFileInfo(szArchivePath, FILE_ATTRIBUTE_NORMAL, &shfinfo, sizeof(SHFILEINFO),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
	if (shfinfo.hIcon) DestroyIcon(shfinfo.hIcon);
	return shfinfo.iIcon;
}

BOOL GetArchiveTagInfo(LPTSTR pszPath, TAGINFO *pTagInfo){
	TCHAR szArchivePath[MAX_FITTLE_PATH];
	LPTSTR p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 2 || !pPlugin->GetBasicTag) return FALSE;
	ZeroMemory(pTagInfo, sizeof(TAGINFO));
	return pPlugin->GetBasicTag(szArchivePath, p, pTagInfo->szTrack, pTagInfo->szTitle, pTagInfo->szAlbum, pTagInfo->szArtist);
}

HICON GetArchiveItemIcon(LPTSTR pszPath){
	TCHAR szStart[100], szEnd[100];
	SHFILEINFO shfinfo = {0};
	TCHAR szArchivePath[MAX_FITTLE_PATH];
	LPTSTR p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 2 || !pPlugin->ResolveIndirect || !pPlugin->ResolveIndirect(szArchivePath, p, szStart, szEnd)){
		lstrcpyn(szArchivePath, pszPath, MAX_PATH);
	}
	SHGetFileInfo(szArchivePath, FILE_ATTRIBUTE_NORMAL, &shfinfo, sizeof(shfinfo), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON); 
	return shfinfo.hIcon;
}

BOOL GetArchiveItemType(LPTSTR pszPath, LPTSTR pBuf, int nBufMax){
	TCHAR szArchivePath[MAX_FITTLE_PATH];
	LPTSTR p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 2 || !pPlugin->GetItemType){
		return FALSE;
	}
	return pPlugin->GetItemType(szArchivePath, p, pBuf, nBufMax);
}

LPTSTR GetArchiveItemFileName(LPTSTR pszPath){
	TCHAR szArchivePath[MAX_FITTLE_PATH];
	LPTSTR p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 2 || !pPlugin->GetItemFileName){
		return 0;
	}
	return pPlugin->GetItemFileName(szArchivePath, p);
}

BOOL GetArchiveGain(LPTSTR pszPath, float *pGain, DWORD hBass){
	TCHAR szArchivePath[MAX_FITTLE_PATH];
	LPTSTR p = GetArchivePath(szArchivePath, pszPath, MAX_FITTLE_PATH);
	ARCHIVE_PLUGIN_INFO *pPlugin = p ? GetAPlugin(szArchivePath) : 0;
	if (!pPlugin || pPlugin->nAPDKVer < 3 || !pPlugin->GetGain){
		return FALSE;
	}
	return pPlugin->GetGain(szArchivePath, p, pGain, hBass);
}

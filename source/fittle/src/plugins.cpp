/*
 * Plugins.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugins.h"
#include "plugin.h"
#include <shlwapi.h>

static FITTLE_PLUGIN_INFO *m_fpis[256] = {0};
static int m_nPluginCount = 0;

static BOOL CALLBACK RegisterPlugin(HMODULE hPlugin, HWND hWnd){
	if (m_nPluginCount < 256) {
		GetPluginInfoFunc GetPluginInfo;
		FITTLE_PLUGIN_INFO *fpi;
		FARPROC fnTarCheck = GetProcAddress(hPlugin, "TarGetVersion");
		GetPluginInfo = (GetPluginInfoFunc)GetProcAddress(hPlugin, "GetPluginInfo");
		if(!fnTarCheck && GetPluginInfo){
			fpi = GetPluginInfo();
			if(fpi->nPDKVer==PDK_VER){
				fpi->hParent = hWnd;
				fpi->hDllInst = hPlugin;
				if(fpi->OnInit()){
					m_fpis[m_nPluginCount++] = fpi;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}
void InitPlugins(HWND hWnd){
	EnumPlugins(NULL, TEXT(""), TEXT("*.dll"), RegisterPlugin, hWnd);
}

void QuitPlugins(){
	int i;

	for(i=0;i<m_nPluginCount;i++){
		m_fpis[i]->OnQuit();
	}
	return;
}

void OnTrackChagePlugins(){
	int i;

	for(i=0;i<m_nPluginCount;i++){
		m_fpis[i]->OnTrackChange();
	}
	return;
}

void OnStatusChangePlugins(){
	int i;

	for(i=0;i<m_nPluginCount;i++){
		m_fpis[i]->OnStatusChange();
	}
	return;
}

static BOOL EnumFilesAndPlugins(HMODULE hParent, LPCTSTR lpszSubDir, LPCTSTR lpszMask, BOOL (CALLBACK * lpfnFileProc)(LPCTSTR lpszPath, HWND hWnd), BOOL (CALLBACK * lpfnPluginProc)(HMODULE hPlugin, HWND hWnd), HWND hWnd){
	TCHAR szDir[MAX_PATH];
	TCHAR szPath[MAX_PATH];
	WIN32_FIND_DATA wfd;
	HANDLE hFind;
	BOOL fRet = FALSE;
	GetModuleFileName(hParent, szPath, MAX_PATH);
	*PathFindFileName(szPath) = TEXT('\0');
	PathCombine(szDir, szPath, lpszSubDir);
	PathCombine(szPath, szDir, lpszMask);

	hFind = FindFirstFile(szPath, &wfd);

	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			HMODULE hPlugin;
			PathCombine(szPath, szDir, wfd.cFileName);
			if (lpfnFileProc && lpfnFileProc(szPath, hWnd)) {
				fRet = TRUE;
			}
			if (lpfnPluginProc) {
				hPlugin = LoadLibrary(szPath);
				if(hPlugin) {
					if (lpfnPluginProc(hPlugin, hWnd)){
						fRet = TRUE;
					}else{
						FreeLibrary(hPlugin);
					}
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	return fRet;
}

BOOL EnumPlugins(HMODULE hParent, LPCTSTR lpszSubDir, LPCTSTR lpszMask, BOOL (CALLBACK * lpfnPluginProc)(HMODULE hPlugin, HWND hWnd), HWND hWnd){
	return EnumFilesAndPlugins(hParent, lpszSubDir, lpszMask, 0, lpfnPluginProc, hWnd);
}

BOOL EnumFiles(HMODULE hParent, LPCTSTR lpszSubDir, LPCTSTR lpszMask, BOOL (CALLBACK * lpfnFileProc)(LPCTSTR lpszPath, HWND hWnd), HWND hWnd){
	return EnumFilesAndPlugins(hParent, lpszSubDir, lpszMask, lpfnFileProc, 0, hWnd);
}

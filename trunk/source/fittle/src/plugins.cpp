/*
 * Plugins.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugins.h"
#include "plugin.h"

static FITTLE_PLUGIN_INFO *m_fpis[256] = {0};
static int m_nPluginCount = 0;

void InitPlugins(LPTSTR pszPath, HWND hWnd){
	TCHAR szPathW[MAX_PATH];
	TCHAR szDllPath[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd = {0};
	HINSTANCE hDll;
	GetPluginInfoFunc GetPluginInfo;
	FITTLE_PLUGIN_INFO *fpi;

	//lstrcat(pszPath, TEXT("Plugins"));
	wsprintf(szPathW, TEXT("%s*.dll"), pszPath);

	hFind = FindFirstFile(szPathW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			wsprintf(szDllPath, TEXT("%s%s"), pszPath, wfd.cFileName);
			hDll = LoadLibrary(szDllPath);
			if(hDll){
				/*
					TAR32.DLLがGetPluginInfoをエクスポートしている問題に対処	
				*/
				FARPROC fnTarCheck = GetProcAddress(hDll, "TarGetVersion");
				GetPluginInfo = (GetPluginInfoFunc)GetProcAddress(hDll, "GetPluginInfo");
				if(!fnTarCheck && GetPluginInfo){

					fpi = GetPluginInfo();
					if(fpi->nPDKVer==PDK_VER){
						m_fpis[m_nPluginCount++] = fpi;
						fpi->hParent = hWnd;
						fpi->hDllInst = hDll;
						if(!fpi->OnInit()){
							FreeLibrary(hDll);
							m_nPluginCount--;
						}
					}
				}else{
					FreeLibrary(hDll);
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}

	return;
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


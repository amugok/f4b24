/*
 * Plugins.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "fittle.h"
#include "plugin.h"
#include "plugins.h"
#include <shlwapi.h>

static FITTLE_PLUGIN_INFO *m_fpis[256] = {0};
static int m_nPluginCount = 0;

static BOOL CALLBACK RegisterPlugin(HMODULE hPlugin, LPVOID user){
	HWND hWnd = (HWND)user;
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
	WAEnumPlugins(NULL, "Plugins\\fgp\\", "*.fgp", RegisterPlugin, hWnd);
	WAEnumPlugins(NULL, "Plugins\\Fittle\\", "*.dll", RegisterPlugin, hWnd);
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


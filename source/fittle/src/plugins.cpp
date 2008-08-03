/*
 * Plugins.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "fittle.h"
#include "plugin.h"
#include "gplugin.h"
#include "plugins.h"
#include "func.h"
#include <shlwapi.h>

static struct GENERAL_PLUGIN_NODE {
	struct GENERAL_PLUGIN_NODE *pNext;
	int nType;
	union {
		LPVOID lpvoid;
		FITTLE_PLUGIN_INFO *pFittle;	/* nTyope == 0 */
		GENERAL_PLUGIN_INFO *pGeneral;	/* nTyope == 1 */
	} info;
	HMODULE hDll;
} *m_pGeneralPluginList = NULL;
static HWND m_hWnd = NULL;

static BOOL InitPlugin(GENERAL_PLUGIN_NODE *pNode){
	if (pNode->nType == 1) {
		return pNode->info.pGeneral->OnEvent(m_hWnd, GENERAL_PLUGIN_EVENT_INIT0);
	}
	if (pNode->info.pFittle->nPDKVer!=PDK_VER) return FALSE;
	pNode->info.pFittle->hParent = m_hWnd;
	pNode->info.pFittle->hDllInst = pNode->hDll;
	return pNode->info.pFittle->OnInit();
}

static BOOL RegisterGeneralPluginSub(HMODULE hPlugin, int nType, LPCSTR lpszProcName) {
	FARPROC lpfnProc = GetProcAddress(hPlugin, lpszProcName);
	if (lpfnProc){
		struct GENERAL_PLUGIN_NODE *pNewNode = (struct GENERAL_PLUGIN_NODE *)HAlloc(sizeof(struct GENERAL_PLUGIN_NODE));
		if (pNewNode) {
			pNewNode->nType = nType;
			pNewNode->info.lpvoid = ((LPVOID (CALLBACK *)())lpfnProc)();
			pNewNode->hDll = hPlugin;
			pNewNode->pNext = NULL;
			if (pNewNode->info.lpvoid && InitPlugin(pNewNode)){
				if (m_pGeneralPluginList) {
					struct GENERAL_PLUGIN_NODE *pList;
					for (pList = m_pGeneralPluginList; pList->pNext; pList = pList->pNext);
					pList->pNext = pNewNode;
				} else {
					m_pGeneralPluginList = pNewNode;
				}
				return TRUE;
			}
			HFree(pNewNode);
		}
	}
	return FALSE;
}

static BOOL CALLBACK RegisterGeneralPlugin(HMODULE hPlugin, LPVOID user){
	return RegisterGeneralPluginSub(hPlugin, 1, "GetGPluginInfo");
}

static BOOL CALLBACK RegisterFittlePlugin(HMODULE hPlugin, LPVOID user){
	return RegisterGeneralPluginSub(hPlugin, 0, "GetPluginInfo");
}

static void RaiseEventPlugins(GENERAL_PLUGIN_EVENT eCode){
	struct GENERAL_PLUGIN_NODE *pList = m_pGeneralPluginList;
	while (pList) {
		struct GENERAL_PLUGIN_NODE *pNext = pList->pNext;
		if (pList->nType == 1) {
			pList->info.pGeneral->OnEvent(m_hWnd, eCode);
		} else if (eCode == GENERAL_PLUGIN_EVENT_TRACK_CHANGE) {
			pList->info.pFittle->OnTrackChange();
		} else if (eCode == GENERAL_PLUGIN_EVENT_STATUS_CHANGE) {
			pList->info.pFittle->OnStatusChange();
		} else if (eCode == GENERAL_PLUGIN_EVENT_QUIT) {
			pList->info.pFittle->OnQuit();
		}
		pList = pNext;
	}
}

void InitGeneralPlugins(HWND hWnd){
	m_hWnd = hWnd;
	WAEnumPlugins(NULL, "Plugins\\fgp\\", "*.fgp", RegisterGeneralPlugin, 0);
}

void InitFittlePlugins(HWND hWnd){
	RaiseEventPlugins(GENERAL_PLUGIN_EVENT_INIT);
	WAEnumPlugins(NULL, "Plugins\\Fittle\\", "*.dll", RegisterFittlePlugin, 0);
	RaiseEventPlugins(GENERAL_PLUGIN_EVENT_INIT2);
}

static void FreePlugins(){
	struct GENERAL_PLUGIN_NODE *pList = m_pGeneralPluginList;
	m_pGeneralPluginList = NULL;
	while (pList) {
		struct GENERAL_PLUGIN_NODE *pNext = pList->pNext;
		if (pList->nType == 1) {
			FreeLibrary(pList->hDll);
		}
		HFree(pList);
		pList = pNext;
	}
}

void QuitPlugins(){
	RaiseEventPlugins(GENERAL_PLUGIN_EVENT_QUIT);
	FreePlugins();
}

void OnTrackChagePlugins(){
	RaiseEventPlugins(GENERAL_PLUGIN_EVENT_TRACK_CHANGE);
}

void OnStatusChangePlugins(){
	RaiseEventPlugins(GENERAL_PLUGIN_EVENT_STATUS_CHANGE);
}

void OnConfigChangePlugins(){
	RaiseEventPlugins(GENERAL_PLUGIN_EVENT_CONFIG_CHANGE);
}

BOOL OnWndProcPlugins(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *lplresult) {
	GENERAL_PLUGIN_HOOK_WNDPROC hwk = { hWnd, msg, wp, lp, 0 };
	struct GENERAL_PLUGIN_NODE *pList = m_pGeneralPluginList;
	while (pList) {
		struct GENERAL_PLUGIN_NODE *pNext = pList->pNext;
		if (pList->nType == 1) {
			if (pList->info.pGeneral->HookWndProc(&hwk)) {
				*lplresult = hwk.lMsgResult;
				return TRUE;
			}
		}
		pList = pNext;
	}
	*lplresult = hwk.lMsgResult;
	return FALSE;
}

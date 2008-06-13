/*
 * bkimg
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include <commctrl.h>

#define FILE_EXIST(X) (GetFileAttributes(X)==0xFFFFFFFF ? FALSE : TRUE)

BOOL OnInit();
void OnQuit();
void OnTrackChange();
void OnStatusChange();
void OnConfig();

FITTLE_PLUGIN_INFO fpi = {
	PDK_VER,
	OnInit,
	OnQuit,
	OnTrackChange,
	OnStatusChange,
	OnConfig,
	NULL,
	NULL
};

static HWND hLV;

/* �G���g���|���g */
BOOL WINAPI _DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
    return TRUE;
}

/* �N�����Ɉ�x�����Ă΂�܂� */
BOOL OnInit(){
	LVBKIMAGE lvbki;
	char szPath[MAX_PATH];
	int nLen;

	CoInitialize(NULL);

	nLen = GetModuleFileName(fpi.hDllInst, szPath, MAX_PATH);
	lstrcpy(&szPath[nLen-3], "bmp");

	if(!FILE_EXIST(szPath)){
		CoUninitialize();
		return FALSE;
	}

	ZeroMemory(&lvbki, sizeof(LVBKIMAGE));
	lvbki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
	lvbki.pszImage = szPath;

	hLV = (HWND)SendMessage(fpi.hParent, WM_FITTLE, GET_LISTVIEW, 0);

	ListView_SetBkImage(hLV, &lvbki);
	ListView_SetTextBkColor(hLV, CLR_NONE);

	return TRUE;
}

/* �I�����Ɉ�x�����Ă΂�܂� */
void OnQuit(){
	LVBKIMAGE lvbki;

	// �w�i�����ɖ߂��Ȃ��Ɨ�����
	ZeroMemory(&lvbki, sizeof(LVBKIMAGE));
	lvbki.ulFlags = LVBKIF_SOURCE_NONE;
	ListView_SetBkImage(hLV, &lvbki);

	CoUninitialize();
	return;
}

/* �Ȃ��ς�鎞�ɌĂ΂�܂� */
void OnTrackChange(){
	return;
}

/* �Đ���Ԃ��ς�鎞�ɌĂ΂�܂� */
void OnStatusChange(){
	return;
}

/* �ݒ��ʂ��Ăяo���܂��i�������j*/
void OnConfig(){
	return;
}

#ifdef __cplusplus
extern "C"
{
#endif
__declspec(dllexport) FITTLE_PLUGIN_INFO *GetPluginInfo(){
	return &fpi;
}
#ifdef __cplusplus
}
#endif
/*
 * opentext
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include <shlwapi.h>
#include <commctrl.h>

#pragma comment(lib, "shlwapi.lib")

#define IDM_OPENTEXT 45111	// �l�͓K���B���Ԃ�Ȃ��悤�ɂ���@�\���~�����Ƃ���
#define ID_TOOLBAR	106

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

static WNDPROC hOldProc;

// �T�u�N���X�������E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_COMMAND:
			if(LOWORD(wp)==IDM_OPENTEXT){
				char *pszPath = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);
				char szPath[MAX_PATH*2];
				lstrcpyn(szPath, pszPath, MAX_PATH*2);
				PathRenameExtension(szPath, ".txt");
				if(PathFileExists(szPath)){
					ShellExecute(fpi.hParent, "open", szPath, NULL, NULL, SW_SHOWNORMAL);
				}
				return 0;
			}
			break;
	}
	return CallWindowProc(hOldProc, hWnd, msg, wp, lp);
}

/* �G���g���|���g */
BOOL WINAPI _DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
    return TRUE;
}

/* �N�����Ɉ�x�����Ă΂�܂� */
BOOL OnInit(){
	hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);
	HWND hToolbar = (HWND)SendMessage(fpi.hParent, WM_FITTLE, GET_CONTROL, ID_TOOLBAR);

	TBADDBITMAP tbab = {HINST_COMMCTRL, IDB_STD_SMALL_COLOR};
	SendMessage(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);

	TBBUTTON tbb[2] = {
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
		{9+STD_FILENEW, IDM_OPENTEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	};
	SendMessage(hToolbar, TB_ADDBUTTONS, 2, (LPARAM)&tbb);

	return TRUE;
}

/* �I�����Ɉ�x�����Ă΂�܂� */
void OnQuit(){
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
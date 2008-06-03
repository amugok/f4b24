/*
 * taskvol
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

#define IDM_VOLUP	40058
#define IDM_VOLDOWN	40059

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

#pragma data_seg(".sharedata")
HHOOK	g_hHook = NULL;
HWND	g_hTaskWnd = NULL;
HWND	g_hParent = NULL;
#pragma data_seg()

HINSTANCE	g_hInst = NULL;
WNDPROC		g_DefProc = NULL;

/* �T�u�N���X�������v���V�[�W�� */
LRESULT CALLBACK SubClassProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp){
	switch(msg)	{
		case WM_MOUSEWHEEL:
			if((signed short)HIWORD(wp) > 0){
				SendMessage(g_hParent, WM_COMMAND, IDM_VOLUP, 0);
			}else{
				SendMessage(g_hParent, WM_COMMAND, IDM_VOLDOWN, 0);
			}
			return 0;

		case WM_APP + 5:
			SetWindowLong(hWnd, GWL_WNDPROC, (LONG)g_DefProc);
			return 0;
	}
	return CallWindowProc(g_DefProc, hWnd , msg , wp , lp);
}

/* �t�b�N�v���V�[�W�� */
LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPCWPSTRUCT	lpcs = (LPCWPSTRUCT)lParam;

	if(nCode == HC_ACTION){
		if(g_DefProc == NULL && lpcs->hwnd == g_hTaskWnd)		{
			g_DefProc = (WNDPROC)SetWindowLong(g_hTaskWnd, GWL_WNDPROC, (LONG)SubClassProc);
		}
	}
	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

/* �G���g���|���g */
BOOL WINAPI _DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
    return TRUE;
}

/* �N�����Ɉ�x�����Ă΂�܂� */
BOOL OnInit(){
	DWORD idThread;

	g_hTaskWnd = FindWindow("Shell_TrayWnd", NULL);
	if(g_hTaskWnd == NULL){
		return FALSE;
	}

	idThread = GetWindowThreadProcessId(g_hTaskWnd, NULL);
    g_hHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, fpi.hDllInst, idThread);
	if(g_hHook == NULL){
		return FALSE;
	}

	SendMessage(g_hTaskWnd, WM_SIZE, SIZE_RESTORED, 0);

	g_hParent = fpi.hParent;

	return TRUE;}

/* �I�����Ɉ�x�����Ă΂�܂� */
void OnQuit(){
	SendMessage(g_hTaskWnd, WM_APP + 5, 0, 0);
	if(g_hHook){
		UnhookWindowsHookEx(g_hHook);
	}
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
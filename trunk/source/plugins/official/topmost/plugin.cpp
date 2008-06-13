/*
 * topmost
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"

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
	static BOOL s_bTopMost = FALSE;
	switch(msg){
		case WM_NCMBUTTONUP:
			s_bTopMost = !s_bTopMost;
			SetWindowPos(fpi.hParent, s_bTopMost?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);			
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
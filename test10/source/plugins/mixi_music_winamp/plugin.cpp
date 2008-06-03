/*
 * plugin.cpp - mixi_music_winamp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include "MainWnd.h"

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

HINSTANCE g_hInst = NULL;;
HWND g_hMainWnd = NULL;
HANDLE hThread  = NULL;

/* �X���b�h */
DWORD WINAPI ThreadFunc(LPVOID /*param*/)
{
	return MyWinMain();
}

/* �G���g���|���g */
BOOL WINAPI _DllMainCRTStartup(HANDLE /*hModule*/, DWORD /*dwFunction*/, LPVOID /*lpNot*/){
    return TRUE;
}

/* �N�����Ɉ�x�����Ă΂�܂� */
BOOL OnInit(){
	// �_�~�[�E�B���h�E������Ĕ�����
	DWORD tid;
	hThread = CreateThread(NULL, 0, ThreadFunc, 0, 0, &tid);
	g_hInst = fpi.hDllInst;
	return hThread?TRUE:FALSE;
}

/* �I�����Ɉ�x�����Ă΂�܂� */
void OnQuit(){
	// �㏈���ł�
	SendMessage(g_hMainWnd, WM_DESTROY, 0, 0);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return;
}

/* �Ȃ��ς�鎞�ɌĂ΂�܂� */
void OnTrackChange(){
	// gen_mixi_for_winamp.dll�̓^�C�g���o�[�̃e�L�X�g�̐؂�ւ��Ő؂�ւ��̊��m���Ă���ۂ�
	TAGINFO *ti = (TAGINFO *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);
	char szBuff[512] = {0};
	wsprintf(szBuff, "%s - %s - Winamp", ti->szArtist, ti->szTitle);
	SetWindowText(g_hMainWnd, szBuff);
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
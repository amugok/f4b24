/*
 * xdelfile
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../fittle/resource/resource.h"
#include "../../fittle/src/plugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shell32.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static HMODULE hDLL = 0;

static BOOL OnInit();
static void OnQuit();
static void OnTrackChange();
static void OnStatusChange();
static void OnConfig();

static FITTLE_PLUGIN_INFO fpi = {
	PDK_VER,
	OnInit,
	OnQuit,
	OnTrackChange,
	OnStatusChange,
	OnConfig,
	NULL,
	NULL
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static WNDPROC hOldProc;

// �T�u�N���X�������E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_COMMAND:
		if(LOWORD(wp)==IDM_LIST_DELFILE){
			return 0;
		}
		break;
	case WM_INITMENUPOPUP:
		DeleteMenu((HMENU)wp, IDM_LIST_DELFILE, MF_BYCOMMAND);
		break;
	}
	return CallWindowProc(hOldProc, hWnd, msg, wp, lp);
}

/* �N�����Ɉ�x�����Ă΂�܂� */
static BOOL OnInit(){
	hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);
	return TRUE;
}

/* �I�����Ɉ�x�����Ă΂�܂� */
static void OnQuit(){
	return;
}

/* �Ȃ��ς�鎞�ɌĂ΂�܂� */
static void OnTrackChange(){
}

/* �Đ���Ԃ��ς�鎞�ɌĂ΂�܂� */
static void OnStatusChange(){
}

/* �ݒ��ʂ��Ăяo���܂��i�������j*/
static void OnConfig(){
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
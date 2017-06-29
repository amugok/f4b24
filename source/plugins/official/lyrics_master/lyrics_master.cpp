/*
 * lyrics_master
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */


#ifdef _WIN64
#pragma comment(lib,"..\\..\\..\\..\\extra\\smartvc14\\smartvc14_x64.lib")
#pragma comment(linker,"/ENTRY:SmartDllStartup")
#else
#pragma comment(lib,"..\\..\\..\\..\\extra\\smartvc14\\smartvc14_x86.lib")
#pragma comment(linker,"/ENTRY:SmartDllStartup@12")
#endif

#include "../../../fittle/src/plugin.h"
#include <shlwapi.h>
#include <commctrl.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

#define IDM_LYRICSMASTER 45112	// �l�͓K���B���Ԃ�Ȃ��悤�ɂ���@�\���~�����Ƃ���
#define IDM_EXPLORE 40151
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

/* �^�O��� */
typedef struct{
	char szTitle[256];
	char szArtist[256];
	char szAlbum[256];
	char szTrack[10];
}TAGINFO;

static WNDPROC hOldProc;

char szPath[MAX_PATH];

// �T�u�N���X�������E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_COMMAND:
			if(LOWORD(wp)==IDM_LYRICSMASTER){
				TAGINFO *pTagInfo = (TAGINFO *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);
				char szParam[256*2];
				wsprintf(szParam, "multi \"%s\" \"%s\"", pTagInfo->szTitle, pTagInfo->szArtist);
				if(lstrlen(pTagInfo->szTitle)*lstrlen(pTagInfo->szArtist)!=0){
					ShellExecute(fpi.hParent, NULL, szPath, szParam, NULL, SW_SHOWNORMAL);
				}
				return 0;
			}
			break;
	}
	return CallWindowProc(hOldProc, hWnd, msg, wp, lp);
}

/* �N�����Ɉ�x�����Ă΂�܂� */
BOOL OnInit(){
	char szINIPath[MAX_PATH];
	GetModuleFileName(fpi.hDllInst, szINIPath, MAX_PATH);
	PathRenameExtension(szINIPath, ".ini");
	GetPrivateProfileString("Main", "Path", "ExtSupport.exe", szPath, MAX_PATH, szINIPath);

	hOldProc = (WNDPROC)SetWindowLongPtr(fpi.hParent, GWLP_WNDPROC, (LONG_PTR)WndProc);

	HMENU hMenu = (HMENU)SendMessage(fpi.hParent, WM_FITTLE, GET_MENU, 0);
	InsertMenu(hMenu, IDM_EXPLORE, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDM_LYRICSMASTER, "&Lyrics Master");
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
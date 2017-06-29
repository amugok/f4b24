/*
 * ranking
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

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")

#include "../../../fittle/src/plugin.h"
#include <shlwapi.h>
#include "svector.h"

extern "C" int atexit( void ( __cdecl *func )( void ) )
{
	return 0;
}

#define MAX_FITTLE_PATH MAX_PATH*2

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

typedef struct {
	int nCount;
	char szPath[MAX_FITTLE_PATH];
	int nDay;
}ITEM;

#define DEFAULT_SURVIVAL 7

SimpleVector<ITEM> Array;
char szPLPath[MAX_FITTLE_PATH] = {0};
char szDBPath[MAX_FITTLE_PATH] = {0};
UINT_PTR uTimerId = 0;
int nSurvival;

static HMODULE hMSVCRTDLL = 0;
typedef void *(__cdecl * LPFOPEN)(const char* filename, const char* mode);
typedef int(__cdecl * LPFCLOSE)(void * stream);
typedef char *(__cdecl * LPFGETS)(char *str, int n, void * stream);
typedef int (__cdecl * LPFPUTS)(const char *str, void * stream);
typedef int (__cdecl * LPTIME)(int *timer );
static LPFOPEN lpfopen = 0;
static LPFCLOSE lpfclose = 0;
static LPFGETS lpfgets = 0;
static LPFPUTS lpfputs = 0;
static LPTIME lptime = 0;

static BOOL InitMsvcrt(){
	HMODULE hMod = LoadLibraryA("msvcrt.dll");
	if (hMod) {
		lpfopen = (LPFOPEN)GetProcAddress(hMod, "fopen");
		lpfclose = (LPFCLOSE)GetProcAddress(hMod, "fclose");
		lpfgets = (LPFGETS)GetProcAddress(hMod, "fgets");
		lpfputs = (LPFPUTS)GetProcAddress(hMod, "fputs");
		lptime = (LPTIME)GetProcAddress(hMod, "time");
		if (lpfopen && lpfclose && lpfgets && lpfputs && lptime) {
			return TRUE;
		}
		FreeLibrary(hMod);
	}
	return FALSE;
}

int lpfputsx(const char *str, void * stream)
{
	int r = lpfputs(str, stream);
	if (r < 0)
	{
		return r;
	}
	r = lpfputs("\n", stream);
	return r;
}

int lpfputix(int i, void * stream)
{
	char buf[32];
	wnsprintfA(buf, sizeof(buf), "%d\n", i);
	int r = lpfputs(buf, stream);
	return r;
}

char * lpfgetsx(char *str, int n, void * stream)
{
	char * r = lpfgets(str, n, stream);
	int l = lstrlenA(str);
	if (l > 0 && str[l - 1] == '\n')
	{
		str[l - 1] = 0;
	}
	return r;
}


// M3U�������o��
void WritePlaylist(LPCSTR pszPath){

	SimpleVector<ITEM>::iterator i;
	
	void * ofs = lpfopen(pszPath, "wt");

	for(i=Array.begin();i!=Array.end();i++){
		lpfputsx(i->szPath, ofs);
	}

	lpfclose(ofs);
	return;
}

// �f�[�^�x�[�X�ǂݏo��
void ReadDatabase(LPCSTR pszPath){
	char szLine[MAX_FITTLE_PATH];
	int nCount;
	int nDay;

	nDay = (int)lptime(NULL)/60/60;

	void * ifs = lpfopen(pszPath, "rt");

	lpfgetsx(szLine, MAX_FITTLE_PATH, ifs);
	nCount = StrToIntA(szLine);
	if(nCount==0) return;
	ITEM item;
	for(int i=0;i<nCount;i++){
		lpfgetsx(item.szPath, MAX_FITTLE_PATH, ifs);
		lpfgetsx(szLine, MAX_FITTLE_PATH, ifs);
		item.nCount = StrToIntA(szLine);
		lpfgetsx(szLine, MAX_FITTLE_PATH, ifs);
		item.nDay = StrToIntA(szLine);
		if(item.nCount==0) break;
		if(nSurvival<0 || nDay-item.nDay<=nSurvival*24){	// �������Ԉȏ�Đ����Ȃ��ꍇ�͓o�^���Ȃ�
			Array.push_back(item);
		}
	}
	lpfclose(ifs);
	return;
}

// �f�[�^�x�[�X�������o��
void WriteDatabase(LPCSTR pszPath){

	SimpleVector<ITEM>::iterator i;

	void * ofs = lpfopen(pszPath, "wt");

	lpfputix(Array.size() , ofs);	// �A�C�e���̐����o��
	for(i=Array.begin();i!=Array.end();i++){
		lpfputsx(i->szPath, ofs);
		lpfputix(i->nCount, ofs);
		lpfputix(i->nDay, ofs);
	}
	lpfclose(ofs);
	return;
}

/* �N�����Ɉ�x�����Ă΂�܂� */
BOOL OnInit(){
	char szINIPath[MAX_FITTLE_PATH] = {0};

	if (!InitMsvcrt())
	{
		return FALSE;
	}
	
	// �p�X�̎擾
	GetModuleFileName(fpi.hDllInst, szINIPath, MAX_FITTLE_PATH);
	PathRenameExtension(szINIPath, ".ini");
	lstrcpyn(szDBPath, szINIPath, MAX_FITTLE_PATH);
	PathRenameExtension(szDBPath, ".dat");
	GetPrivateProfileString("Main", "Path", "", szPLPath, MAX_FITTLE_PATH, szINIPath);
	if(!*szPLPath){
		lstrcpyn(szPLPath, szINIPath, MAX_FITTLE_PATH);
		PathRenameExtension(szPLPath, ".m3u");
	}
	nSurvival = GetPrivateProfileInt("Main", "Survival", DEFAULT_SURVIVAL, szINIPath);

	ReadDatabase(szDBPath);

	return TRUE;
}

/* �I�����Ɉ�x�����Ă΂�܂� */
void OnQuit(){
	WriteDatabase(szDBPath);
	Array.clear();
	return;
}

VOID CALLBACK TimerProc(HWND /*hWnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/){
	char *pszPath;
	ITEM item = {0};
	SimpleVector<ITEM>::iterator i;

	// �^�C�}�[���폜
	KillTimer(NULL, uTimerId);
	uTimerId = 0;

	// �p�X�̎擾
	pszPath = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);
	lstrcpy(item.szPath, pszPath);
	item.nCount = 1;
	item.nDay = (int)lptime(NULL)/60/60;	// �b->����

	//char dbgmsg[1024];
	//wsprintfA(dbgmsg, "OnTimer:%s\n", item.szPath);
	//OutputDebugStringA(dbgmsg);

	if (item.szPath[0])
	{
		//OutputDebugStringA("X1\n");

		// ����p�X�̌���
		for(i=Array.begin();i!=Array.end();i++){
			if(!lstrcmpi(i->szPath, item.szPath)){
				item.nCount = i->nCount+1;
				Array.erase(i);
				break;
			}
		}

		//OutputDebugStringA("X2\n");

		// �}���ʒu�̌���
		for(i=Array.begin();i!=Array.end();i++){
			if(i->nCount<=item.nCount) break;
		}

		//OutputDebugStringA("X3\n");

		Array.insert(i, item);

		//OutputDebugStringA("X4\n");

		// �t�@�C���ɏ�������
		WritePlaylist(szPLPath);

		//OutputDebugStringA("X5\n");

		WriteDatabase(szDBPath);

		//OutputDebugStringA("X6\n");
	}

	return;
}

/* �Ȃ��ς�鎞�ɌĂ΂�܂� */
void OnTrackChange(){
	if(uTimerId) KillTimer(NULL, uTimerId);
	int nLen = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_DURATION, 0);

	//char dbgmsg[1024];
	//wsprintfA(dbgmsg, "OnTrackChange:%d\n", nLen);
	//OutputDebugStringA(dbgmsg);

	uTimerId = SetTimer(NULL, 0, nLen*500, TimerProc);	// �Ȃ̔����ŃC�x���g���Ă΂��悤�ɂ���
	//uTimerId = SetTimer(NULL, 0, 5000, TimerProc);	// �Ȃ̔����ŃC�x���g���Ă΂��悤�ɂ���
	return;
}

/* �Đ���Ԃ��ς�鎞�ɌĂ΂�܂� */
void OnStatusChange(){
	if(uTimerId!=0){
		int nStatus = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0);

		//char dbgmsg[1024];
		//wsprintfA(dbgmsg, "OnStatusChange:%d\n", nStatus);
		//OutputDebugStringA(dbgmsg);

		if(nStatus==0){
			KillTimer(NULL, uTimerId);
			uTimerId = 0;
		}
	}
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

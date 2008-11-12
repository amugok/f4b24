/*
 * ranking
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include <time.h>
#include <shlwapi.h>
#include <list>
#include <fstream>

#pragma comment(lib, "shlwapi.lib")

#define MAX_FITTLE_PATH MAX_PATH*2

using namespace std;

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

list<ITEM> Array;
char szPLPath[MAX_FITTLE_PATH] = {0};
char szDBPath[MAX_FITTLE_PATH] = {0};
UINT_PTR uTimerId = 0;
int nSurvival;

// M3U�������o��
void WritePlaylist(LPCSTR pszPath){
	ofstream ofs(pszPath);

	list<ITEM>::iterator i;

	for(i=Array.begin();i!=Array.end();i++){
		ofs << i->szPath << endl;
	}
	ofs.close();	// �f�X�g���N�^�����邩�炢��Ȃ����낤���ǂ���Ɨ�����
	return;
}

// �f�[�^�x�[�X�ǂݏo��
void ReadDatabase(LPCSTR pszPath){
	char szLine[MAX_FITTLE_PATH];
	int nCount;
	int nDay;

	nDay = (int)time(NULL)/60/60;

	ifstream ifs(pszPath);
	ifs.getline(szLine, MAX_FITTLE_PATH);
	nCount = atoi(szLine);
	if(nCount==0) return;
	ITEM item;
	for(int i=0;i<nCount;i++){
		ifs.getline(item.szPath, MAX_FITTLE_PATH);
		ifs.getline(szLine, MAX_FITTLE_PATH);
		item.nCount = atoi(szLine);
		ifs.getline(szLine, MAX_FITTLE_PATH);
		item.nDay = atoi(szLine);
		if(item.nCount==0) break;
		if(nSurvival<0 || nDay-item.nDay<=nSurvival*24){	// �������Ԉȏ�Đ����Ȃ��ꍇ�͓o�^���Ȃ�
			Array.push_back(item);
		}
	}
	ifs.close();
	return;
}

// �f�[�^�x�[�X�������o��
void WriteDatabase(LPCSTR pszPath){
	ofstream ofs(pszPath);

	list<ITEM>::iterator i;

	ofs << Array.size() << endl;	// �A�C�e���̐����o��
	for(i=Array.begin();i!=Array.end();i++){
		ofs << i->szPath << endl;
		ofs << i->nCount << endl;
		ofs << i->nDay << endl;
	}
	ofs.close();
	return;
}

/* �G���g���|���g */
BOOL WINAPI _DllMainCRTStartup(HANDLE /*hModule*/, DWORD /*dwFunction*/, LPVOID /*lpNot*/){
    return TRUE;
}

/* �N�����Ɉ�x�����Ă΂�܂� */
BOOL OnInit(){
	char szINIPath[MAX_FITTLE_PATH] = {0};

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

VOID CALLBACK TimerProc(HWND /*hWnd*/, UINT /*uMsg*/, UINT /*idEvent*/, DWORD /*dwTime*/){
	char *pszPath;
	ITEM item = {0};
	list<ITEM>::iterator i;

	// �p�X�̎擾
	pszPath = (char *)SendMessage(fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0);
	lstrcpy(item.szPath, pszPath);
	item.nCount = 1;
	item.nDay = (int)time(NULL)/60/60;	// �b->����

	// ����p�X�̌���
	for(i=Array.begin();i!=Array.end();i++){
		if(!lstrcmpi(i->szPath, pszPath)){
			item.nCount = i->nCount+1;
			Array.erase(i);
			break;
		}
	}

	// �}���ʒu�̌���
	for(i=Array.begin();i!=Array.end();i++){
		if(i->nCount<=item.nCount) break;
	}
	Array.insert(i, item);

	// �t�@�C���ɏ�������
	WritePlaylist(szPLPath);
	WriteDatabase(szDBPath);

	// �^�C�}�[���폜
	KillTimer(NULL, uTimerId);
	uTimerId = 0;

	// For Debugging
	//MessageBeep(MB_OK);

	return;
}

/* �Ȃ��ς�鎞�ɌĂ΂�܂� */
void OnTrackChange(){
	if(uTimerId) KillTimer(NULL, uTimerId);
	int nLen = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_DURATION, 0);
	uTimerId = SetTimer(NULL, 0, nLen*1000/2, TimerProc);	// �Ȃ̔����ŃC�x���g���Ă΂��悤�ɂ���
	return;
}

/* �Đ���Ԃ��ς�鎞�ɌĂ΂�܂� */
void OnStatusChange(){
	if(uTimerId!=0){
		int nStatus = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0);
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

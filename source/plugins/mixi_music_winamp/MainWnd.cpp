/*
 * MainWnd.cpp - mixi_music_winamp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "MainWnd.h"
#include <shlwapi.h>
//#include "resource.h"
#include "Plugin.h"
#include <tchar.h>
#include "GEN.H"
#include "wa_ipc.h"

extern HINSTANCE g_hInst;
extern HWND g_hMainWnd;
extern FITTLE_PLUGIN_INFO fpi;

#define HEADER_SIZE 10

HINSTANCE hDll = NULL;
winampGeneralPurposePlugin *wgpp = NULL;
char szPath[MAX_PATH] = {0};

/* �E�B���h�E�쐬�� */
LRESULT OnCreate(HWND hWnd){
	int nLen = GetModuleFileName(g_hInst, szPath, MAX_PATH);
	szPath[nLen - 3] = 'm';
	szPath[nLen - 2] = 'p';
	szPath[nLen - 1] = '3';

	winampGeneralPurposePluginGetter winampGetGeneralPurposePlugin;

	hDll = LoadLibrary(_T("gen_mixi_for_winamp.dll"));
	if(!hDll) return 0;
	winampGetGeneralPurposePlugin = (winampGeneralPurposePluginGetter)GetProcAddress(hDll, _T("winampGetGeneralPurposePlugin"));

	wgpp = winampGetGeneralPurposePlugin();
	if(wgpp && wgpp->version==GPPHDR_VER){
		wgpp->hDllInstance = hDll;
		wgpp->hwndParent = hWnd;
		wgpp->init();
	}
	return 0;
}

// �z���g��SafeInt�Ȃ񂾂��ǂ悭�킩��Ȃ��̂œK���ɃT�C�Y������
void WriteSafeInt(LPSTR pszBuff, int nSize){
	for(int i=3;i>=0;i--){
		pszBuff[i] = (char)nSize % 0xff;
		nSize /= 0xff;
	}
	return;
}

// �t���[���̏�������
int WriteFrame(LPSTR pszBuff, LPCSTR pszName, LPCSTR pszData){
	int nSize;
	// �t���[����
	lstrcpy(pszBuff, pszName);
	// �T�C�Y(4byte)
	nSize = lstrlen(pszData) + 2;
	WriteSafeInt(pszBuff + 4, nSize);
	// �f�[�^(�I�[�������܂�)
	lstrcpy(pszBuff + HEADER_SIZE + 1, pszData);
	return nSize + HEADER_SIZE;
}

LPSTR MakeDataToStation(LPSTR pszPath){
	TAGINFO *pTag = (TAGINFO *)SendMessage(fpi.hParent, WM_FITTLE, GET_TITLE, 0);

	// �����Ń_�~�[�̃t�@�C�����쐬���܂��B
	char szBuff[sizeof(TAGINFO)] = {0};
	HANDLE hFile = CreateFile(pszPath,
		GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return NULL;

	// �w�b�_�̏�������
	lstrcat(szBuff, "ID3");
	szBuff[3] = (char)3;	// ID3v2.3�ɂ���B

	int nTotal = HEADER_SIZE;

	// �t���[���̏�������
	nTotal += WriteFrame(szBuff + nTotal, "TPE1", pTag->szArtist);
	nTotal += WriteFrame(szBuff + nTotal, "TIT2", pTag->szTitle);
	nTotal += WriteFrame(szBuff + nTotal, "TALB", pTag->szAlbum);
	nTotal += WriteFrame(szBuff + nTotal, "TRCK", pTag->szTrack);

	// �S�̂̃T�C�Y������
	WriteSafeInt(szBuff + 6, nTotal);

	DWORD dwSize = 0;
	WriteFile(hFile, szBuff, (DWORD)nTotal, &dwSize, NULL);
	CloseHandle(hFile);

	return pszPath;
}

/* �E�B���h�E�v���V�[�W�� */
LRESULT CALLBACK WndProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp){

#ifdef _DEBUG
	// �ǂ�ȃ��b�Z�[�W�����Ƃ肳��Ă��邩������ŊĎ����܂��B
	TCHAR szDebugStr[100];
	wsprintf(szDebugStr, _T("msg=%#04lX wp=%d lp=%d\n"), msg, wp, lp);
	OutputDebugString(szDebugStr);
#endif

	switch (msg) {
		case WM_CREATE:
			return OnCreate(hWnd);

		case WM_WA_IPC:
			switch(lp){
				case IPC_ISPLAYING:
					// 0:��~�� 1:�Đ��� 3:�ꎞ��~��
					return SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0);

				case IPC_GETOUTPUTTIME:
					int ret;
					if(wp==0){
						// ���݂̈ʒu���~���b�ŕԂ�
						ret = 1000*(int)SendMessage(fpi.hParent, WM_FITTLE, GET_POSITION, 0);
					}else if(wp==1){
						// �Ȃ̒�����b�ŕԂ�
						ret = (int)SendMessage(fpi.hParent, WM_FITTLE, GET_DURATION, 0);
					}else{
						// �G���[
						ret = -1;
					}
					return ret;

				case IPC_GETLISTPOS:
					// �_�~�[��Ԃ�
					return 0;

				case IPC_GETPLAYLISTFILE:
					// Index�̒l�ɂ�����炸�_�~�[�̃t�@�C����Ԃ��B
					return (LRESULT)MakeDataToStation(szPath);

			}
			return 0;

		case WM_DESTROY:
			if(wgpp) wgpp->quit();
			if(hDll) FreeLibrary(hDll);
			PostQuitMessage(0);
			return 0;

	}
	return DefWindowProc(hWnd , msg , wp , lp);
}

/* WinMain�̑�֊֐� */
int WINAPI MyWinMain(){
	WNDCLASSEX wc;
	HWND hWnd;
	MSG msg;
	TCHAR szClassName[] =_T("Finamp v1.x");

	// �E�B���h�E�E�N���X�̓o�^
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc; //�v���V�[�W����
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInst; //�C���X�^���X
	wc.hIcon = LoadIcon(NULL , IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL , IDI_APPLICATION);
	wc.hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,	0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;	//���j���[��
	wc.lpszClassName = (LPCSTR)szClassName;
	if(!RegisterClassEx(&wc)) return 0;

	// �E�B���h�E�쐬
	hWnd = CreateWindowEx(0,
			szClassName,
			szClassName,			// �^�C�g���o�[�ɂ��̖��O���\������܂�
			WS_OVERLAPPEDWINDOW,	// �E�B���h�E�̎��
			CW_USEDEFAULT,			// �w���W
			CW_USEDEFAULT,			// �x���W
			CW_USEDEFAULT,			// ��
			CW_USEDEFAULT,			// ����
			NULL,					// �e�E�B���h�E�̃n���h���A�e�����Ƃ���NULL
			NULL,					// ���j���[�n���h���A�N���X���j���[���g���Ƃ���NULL
			g_hInst,				// �C���X�^���X�n���h��
			NULL);
	if(!hWnd) return 0;

	g_hMainWnd = hWnd;

	UpdateWindow(hWnd);

	while (GetMessage(&msg , NULL , 0 , 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

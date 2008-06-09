/*
 * gesture
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#define WINVER		0x0400	// 98�ȍ~
#define _WIN32_IE	0x0400	// IE4�ȍ~

#pragma comment(lib, "shlwapi.lib")

#include "plugin.h"
#include <commctrl.h>
#include <shlwapi.h>

#define ID_STATUS	110
enum DIRECTION{ END=0, RIGHT, LEFT, UP, DOWN};

#define CHECK_DISTANCE 20*20
#define MAX_GES_COUNT 100

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

/* �W�F�X�`���[�A�C�e�� */
typedef struct{
	int nCmdId;
	int data[MAX_GES_COUNT];
} GESDATA;

/* �K���L���[ */
typedef struct {
	int data[MAX_GES_COUNT];
	int qp;
} QUEUE;

HWND hStatus;
HHOOK hHook;
int nGesCount = 0;
GESDATA GesData[100] = {0};

int CalcOffsetSquare(int s, int t){
	return (s - t)*(s - t);
}

LRESULT CALLBACK MyHookProc(int nCode, WPARAM wp, LPARAM lp){
	#define msg ((MSG *)lp)

	static POINT ptBase;
	static char szStatus[100];
	static QUEUE queue = {0};
	static BOOL bDown = FALSE;
	static BOOL bHook = TRUE;

	int nXs, nYs, i, j, nDir;
	POINT pt;
	char szGesture[100];

	if(nCode<0)
		return CallNextHookEx(hHook, nCode, wp, lp);
	
	switch(msg->message){
		case WM_RBUTTONDOWN:
			if(bHook && !bDown){
				// �W�F�X�`���[�J�n
				OutputDebugString("D");
				SetCapture(fpi.hParent);
				bDown = TRUE;

				// ��_��ݒ�
				ptBase.x = msg->pt.x;
				ptBase.y = msg->pt.y;

				// �L���[��������
				queue.qp = 0;

				// �X�e�[�^�X�o�[�̕������o����
				SendMessage(hStatus, SB_GETTEXT, (WPARAM)0, (LPARAM)szStatus);
				msg->message = WM_NULL;
			}
			break;

		case WM_MOUSEMOVE:
			if(bHook && bDown){
				// �}�E�X�ړ���
				OutputDebugString("M");

				// ��_����̋��������߂�
				pt.x = msg->pt.x;
				pt.y = msg->pt.y;
				nXs = CalcOffsetSquare(pt.x, ptBase.x);
				nYs = CalcOffsetSquare(pt.y, ptBase.y);

				// ������CHECK_DISTANCE�ȏ�
				if(nXs + nYs >= CHECK_DISTANCE){
					if(nXs>=nYs){
						// �E����
						if(pt.x>ptBase.x){
							nDir = RIGHT;
						}else{
							nDir = LEFT;
						}
					}else{
						// �����E
						if(pt.y>ptBase.y){
							nDir = DOWN;
						}else{
							nDir = UP;
						}
					}

			
					// �O��̕����ƈ������W�F�X�`���[�Ƃ��ĔF��
					if(queue.qp==0 || nDir!=queue.data[queue.qp-1]){
						queue.data[queue.qp++] = nDir;
					}

					// ��_��ύX
					ptBase.x = pt.x;
					ptBase.y = pt.y;

					// �W�F�X�`���[��\��
					lstrcpy(szGesture, "Gesture: ");
					for(i=0;i<queue.qp;i++){
						switch(queue.data[i]){
							case RIGHT:
								lstrcat(szGesture, "��");
								break;
							case LEFT:
								lstrcat(szGesture, "��");
								break;
							case DOWN:
								lstrcat(szGesture, "��");
								break;
							case UP:
								lstrcat(szGesture, "��");
								break;
						}
					}
					PostMessage(hStatus, SB_SETTEXT, 0|0, (LPARAM)szGesture);
				}
			}
			break;

		case WM_RBUTTONUP:
			if(!bHook){
				bHook = TRUE;	// �}�E�X�A�b�v������������t�b�N�ĊJ
			}else if(bDown){
				// �W�F�X�`���I��
				OutputDebugString("U\n");
				bDown = FALSE;
				ReleaseCapture();
				PostMessage(hStatus, SB_SETTEXT, 0|0, (LPARAM)szStatus);

				if(queue.qp<=0){
					// �W�F�X�`���[����Ȃ��ĉE�N���b�N
					pt.x = msg->pt.x;
					pt.y = msg->pt.y;
					ScreenToClient(WindowFromPoint(msg->pt), &pt);

					bHook = FALSE;
					PostMessage(WindowFromPoint(msg->pt), WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));

					/* ���������������̂��킩��Ȃ��̂ŕۗ�
					RECT rcDisp;
					GetClientRect(WindowFromPoint(msg->pt), &rcDisp);
					PostMessage(WindowFromPoint(msg->pt), WM_SIZE, 0, MAKELPARAM(rcDisp.right, rcDisp.bottom));
					*/

					PostMessage(WindowFromPoint(msg->pt), WM_RBUTTONUP, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
				}else{
					// �W�F�X�`���[�A�N�V����
					for(i=0;i<nGesCount;i++){
						// �O�����珇�ԂɈ�r���m�F
						for(j=0;j<queue.qp;j++){
							if(queue.data[j]!=GesData[i].data[j]) break;
						}
						// �Ō�܂ň�v���Ă�����R�}���h�𓊂��ďI��
						if(j==queue.qp && GesData[i].data[queue.qp]==END){
							PostMessage(fpi.hParent, WM_COMMAND, MAKEWPARAM(GesData[i].nCmdId, 0), 0);
							break;
						}
					}
				}
				msg->message = WM_NULL;
			}
			break;
	}

	return CallNextHookEx(hHook, nCode, wp, lp);
}

/* �G���g���|���g */
BOOL WINAPI _DllMainCRTStartup(HANDLE /*hModule*/, DWORD /*dwFunction*/, LPVOID /*lpNot*/){
    return TRUE;
}

/* �N�����Ɉ�x�����Ă΂�܂� */
BOOL OnInit(){
	char szCfgPath[MAX_PATH];
	int nLen, nID;
	int i=0,k;
	char c;
	char szLine[256] = {0};
	char *p;
	HANDLE hFile;
	DWORD dwAccBytes = 1;

	// �ݒ�t�@�C���̃p�X���擾
	nLen = GetModuleFileName(fpi.hDllInst, szCfgPath, MAX_PATH);
	szCfgPath[nLen-3] = 'c';
	szCfgPath[nLen-2] = 'f';
	szCfgPath[nLen-1] = 'g';

	hFile = CreateFile(szCfgPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;	// �ݒ�t�@�C�����ǂ߂Ȃ��Ȃ�I��

	while(dwAccBytes){
		ReadFile(hFile, &c, sizeof(char), &dwAccBytes, NULL);
		switch(c){
			//case '\r':
			case '\n':
			case '\0':
				szLine[i++] = '\0';
				OutputDebugString(szLine);
				OutputDebugString("\t");
				// �R�}���hID�����
				nID = StrToInt(szLine);
				if(nID>0){
					GesData[nGesCount].nCmdId = nID;
					p = szLine + lstrlen(szLine) + 1;
					OutputDebugString(p);
					OutputDebugString("\n");
					k = 0;
					for(;*p;p++){
						switch(*p){
							case 'R':
								GesData[nGesCount].data[k++] = RIGHT;
								break;
							case 'L':
								GesData[nGesCount].data[k++] = LEFT;
								break;
							case 'U':
								GesData[nGesCount].data[k++] = UP;
								break;
							case 'D':
								GesData[nGesCount].data[k++] = DOWN;
								break;
						}
					}
					GesData[nGesCount++].data[k] = END;
				}
				i = 0;
				break;

			case ' ':	// �z���C�g�X�y�[�X��������I�[�����Ŗ��߂�
			case '\t':
				szLine[i++] = '\0';
				break;

			default :
				szLine[i++] = c;
				break;

		}
	};

	CloseHandle(hFile);

	// �X�e�[�^�X�o�[�̃n���h���擾
	hStatus = (HWND)SendMessage(fpi.hParent, WM_FITTLE, GET_CONTROL, ID_STATUS);
	// ���[�J���t�b�N
	hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)MyHookProc, (HINSTANCE)GetWindowLong(fpi.hParent, GWL_HINSTANCE), GetWindowThreadProcessId(fpi.hParent, NULL));

	return (BOOL)hHook;
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
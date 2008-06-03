#include <windows.h>

/* PDK�̃o�[�W���� */
#define PDK_VER 0

/* �\���̐錾 */
typedef struct{
	int nPDKVer;
	BOOL (*OnInit)();
	void (*OnQuit)();
	void (*OnTrackChange)();
	void (*OnStatusChange)();
	void (*OnConfig)();
	HWND hParent;
	HINSTANCE hDllInst;
} FITTLE_PLUGIN_INFO;

/* ���b�Z�[�WID */
#define WM_FITTLE WM_USER+2

#define GET_TITLE 100
/* �^�C�g�����擾���܂� */
// pszTitle = (const tchar *)SendMessage(hWnd, WM_FITTLE, GET_TITLE, 0);

#define GET_ARTIST 101
/* �A�[�e�B�X�g���擾���܂� */
// pszArtist = (const char *)SendMessage(hWnd, WM_FITTLE, GET_ARTIST, 0);

#define GET_PLAYING_PATH 102
/* ���ݍĐ����̃t�@�C���̃t���p�X���擾���܂� */
// pszPath = (const char *)SendMessage(hWnd, WM_FITTLE, GET_PLAYING_PATH, 0);

#define GET_PLAYING_INDEX 103
/* ���ݍĐ����̋Ȃ̃C���f�b�N�X���擾���܂� */
// nIndex = (int)SendMessage(hWnd, WM_FITTLE, GET_PLAYING_INDEX, 0);

#define GET_STATUS 104
/* �Đ���Ԃ��擾���܂� */
/*
	0 : ��~��
	1 : �Đ���
	2 : �X�g�[����
	3 : �ꎞ��~��
*/
// nStatus = (int)SendMessage(hWnd, WM_FITTLE, GET_STATUS, 0);

#define GET_POSITION 105
/* ���݂̈ʒu��b�P�ʂŎ擾���܂� */
// nPos = (int)SendMessage(hWnd, WM_FITTLE, GET_POSITION, 0);

#define GET_DURATION 106
/* ���ݍĐ����̃t�@�C���̒�����b�P�ʂŎ擾���܂� */
// nLen = (int)SendMessage(hWnd, WM_FITTLE, GET_DURATION, 0);

#define GET_LISTVIEW 107
/* ���X�g�r���[�̃n���h�����擾���܂� */
// hList = (HWND)SendMessage(hWnd, WM_FITTLE, GET_LISTVIEW, nIndex);
/*
	nIndex = �����牽�Ԗڂ̃^�u�Ɋ֘A�t����ꂽ���X�g�r���[�����w�肵�܂�
	���̒l���w�肷��ƃJ�����g�̃��X�g�r���[�̃n���h����Ԃ��܂�
*/

#define GET_CONTROL 108
/* �R���g���[���̃n���h�����擾���܂� */
// hCtrl = (HWND)SendMessage(hWnd, WM_FITTLE, GET_CONTROL, id);

/* id �̒l�ɂ���Ė߂�l�������܂�
	#define ID_COMBO	101
	#define ID_TREE		102
	#define TD_SEEKBAR	104
	#define ID_TOOLBAR	106
	#define ID_VOLUME	107
	#define ID_TAB		108
	#define ID_REBAR	109
	#define ID_STATUS	110
*/

#define GET_CURPATH 109
/* ���݂̃c���[�̃p�X���擾���܂� */
// pszPath = (const char *)SendMessage(hWnd, WM_FITTLE, GET_CURPATH, 0);

#define GET_MENU 110



/* �C�ɂ��Ȃ��ŉ����� */
typedef FITTLE_PLUGIN_INFO *(*GetPluginInfoFunc)();

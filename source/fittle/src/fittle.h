/*
 * Fittle.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef FITTLE_H_
#define FITTLE_H_

#define MAX_FITTLE_PATH 260*2

#define WINVER		0x0400	// 98�ȍ~
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4�ȍ~

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>

#ifndef GetWindowLongPtr
#define GetWindowLongPtr GetWindowLong
#define SetWindowLongPtr SetWindowLong
#define GWLP_WNDPROC GWL_WNDPROC
#define GWLP_USERDATA GWL_USERDATA
#define GWLP_HINSTANCE GWL_HINSTANCE
#define DWORD_PTR DWORD
#endif
#ifndef MAXLONG_PTR
typedef long LONG_PTR;
#endif

#include "../resource/resource.h"

#include "../../../extra/bass24/bass.h"

// �z�C�[��
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL	0x020A
#endif

// �萔��`
#define ID_COMBO	101
#define ID_TREE		102
#define TD_SEEKBAR	104
#define ID_LIST		105
#define ID_TOOLBAR	106
#define ID_VOLUME	107
#define ID_TAB		108
#define ID_REBAR	109
#define ID_STATUS	110
#define ID_SEEKTIMER		200	// �Đ��ʒu�\���^�C�}�[
#define ID_TIPTIMER			201	// �c�[���`�b�v�����^�C�}�[
#define ID_SCROLLTIMERUP	202	// �h���b�O�������X�N���[���^�C�}�[
#define ID_SCROLLTIMERDOWN	203
#define ID_LBTNCLKTIMER		204
#define ID_RBTNCLKTIMER		205
#define ID_MBTNCLKTIMER		206
#define WM_TRAY		(WM_APP + 1)	// �^�X�N�g���C�̃��b�Z�[�W
#define SPLITTER_WIDTH	5		// �X�v���b�^�̕�
#define HOTKEY_COUNT	8		// �z�b�g�L�[�̐�
#define SLIDER_DIVIDED	1000	// �V�[�N�o�[���̕�����
#define IDM_SENDPL_FIRST 50000	// �v���C���X�g�ɑ��铮�I���j���[
#define IDM_BM_FIRST 60000		// �����蓮�I���j���[
#define FITTLE_WIDTH 435		// �E�B���h�E��
#define FITTLE_HEIGHT 356		// �E�B���h�E����
#define BAND_COUNT 3			// ���o�[�̃o���h�̐�

struct FILEINFO{
	TCHAR szFilePath[MAX_FITTLE_PATH];	// �t�@�C���p�X
	TCHAR szSize[50];			// �T�C�Y
	TCHAR szTime[50];			// �X�V����
	BOOL bPlayFlag;				// �Đ��ς݃`�F�b�N�t���O
	struct FILEINFO *pNext;		// ���̃��X�g�ւ̃|�C���^
};

typedef struct {
	HSTREAM hChan;				// �X�g���[���n���h��
	TCHAR szFilePath[MAX_FITTLE_PATH];	// �t���p�X
	QWORD qStart;				// �Đ��J�n�ʒu
	QWORD qDuration;			// �Đ��I���ʒu
	LPBYTE pBuff;				// �������o�b�t�@
} CHANNELINFO;

struct CONFIG{
	int nBkColor;				// �w�i�̐F
	int nTextColor;				// �����̐F
	int nPlayTxtCol;			// �Đ��ȕ�����
	int nPlayBkCol;				// �Đ��Ȕw�i
	int nPlayView;				// �Đ��Ȃ̕\�����@
	int nHighTask;				// �V�X�e���̗D��x
	int nGridLine;				// �O���b�h���C����\��
	int nSingleExpand;			// �t�H���_��������J���Ȃ�
	int nExistCheck;			// ���݊m�F
	int nTimeInList;			// �v���C���X�g�ōX�V�������擾����
	int nTreeIcon;				// �c���[�A�R���{�̃A�C�R���\��
	int nTrayOpt;				// �^�X�N�g���C���[�h
	int nHideShow;				// �B���t�H���_��\�����邩
	int nTabBottom;				// �^�u�����ɕ\������
	int nTabMulti;				// ���i�ŕ\������
	int nAllSub;				// �S�Ẵt�H���_���T�u�t�H���_������
	int nPathTip;				// �q���g�Ńt���p�X��\��
	int nInfoTip;				// �Ȗ����m�点�@�\
	int nTagReverse;			// �^�C�g���A�A�[�e�B�X�g�𔽓]
	int nShowHeader;			// �w�b�_�R���g���[����\������
	int nSeekAmount;			// �V�[�N��
	int nVolAmount;				// ���ʕω���(�B���ݒ�?)
	int nResume;				// �I�����ɍĐ����Ă����Ȃ��N�����ɂ��Đ�����
	int nResPosFlag;			// �I�����̍Đ��ʒu���L�^��������
	int nCloseMin;				// ����{�^���ōŏ�������
	int nZipSearch;				// �T�u�t�H���_�������ň��k�t�@�C������������
	int nTabHide;				// �^�u����̎��̓^�u���B��
	int nOut32bit;				// 32bit(float)�ŏo�͂���
	int nFadeOut;				// ��~���Ƀt�F�[�h�A�E�g����

	TCHAR szStartPath[MAX_FITTLE_PATH];	// �X�^�[�g�A�b�v�p�X
	TCHAR szFilerPath[MAX_FITTLE_PATH];	// �t�@�C���̃p�X

	int nHotKey[HOTKEY_COUNT];	// �z�b�g�L�[

	int nTrayClick[6];			// �N���b�N���̓���

	TCHAR szFontName[32];		// �t�H���g�̖��O
	int nFontHeight;			// �t�H���g�̍���
	int nFontStyle;				// �t�H���g�̃X�^�C��

	TCHAR szToolPath[MAX_FITTLE_PATH];

	/* ��� */

	int nTreeWidth;				// �c���[�̕�
	int nCompact;				// �R���p�N�g���[�h
	int nResPos;
	int nTreeState;
	int nShowStatus;

	int nMiniPanelEnd;

	TCHAR szLastFile[MAX_FITTLE_PATH];
};

extern struct CONFIG g_cfg;
extern CHANNELINFO g_cInfo[2];
extern BOOL g_bNow;

LRESULT CALLBACK NewListProc(HWND, UINT, WPARAM, LPARAM); //���X�g�r���[�p�̃v���V�[�W��
BOOL CheckFileType(LPTSTR);

#endif
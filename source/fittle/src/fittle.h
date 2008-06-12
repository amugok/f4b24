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
#define MAX_EXT_COUNT 30		// �����g���q�̐�
#define MAX_BM_SIZE 10			// ������̐�
#define BAND_COUNT 3			// ���o�[�̃o���h�̐�

struct FILEINFO{
	char szFilePath[MAX_FITTLE_PATH];	// �t�@�C���p�X
	char szSize[50];			// �T�C�Y
	char szTime[50];			// �X�V����
	BOOL bPlayFlag;				// �Đ��ς݃`�F�b�N�t���O
	struct FILEINFO *pNext;		// ���̃��X�g�ւ̃|�C���^
};

typedef struct {
	HSTREAM hChan;				// �X�g���[���n���h��
	char szFilePath[MAX_FITTLE_PATH];	// �t���p�X
	QWORD qStart;				// �Đ��J�n�ʒu
	QWORD qDuration;			// �Đ��I���ʒu
	LPBYTE pBuff;				// �������o�b�t�@
} CHANNELINFO;

struct CONFIG{
	int nTrayOpt;				// �^�X�N�g���C���[�h
	int nInfoTip;				// �Ȗ����m�点�@�\
	int nHighTask;				// �V�X�e���̗D��x
	int nTreeIcon;				// �c���[�A�R���{�̃A�C�R���\��
	int nHideShow;				// �B���t�H���_��\�����邩
	int nAllSub;				// �S�Ẵt�H���_���T�u�t�H���_������
	int nPathTip;				// �q���g�Ńt���p�X��\��
	int nGridLine;				// �O���b�h���C����\��
	int nSingleExpand;			// �t�H���_��������J���Ȃ�
	int nBMRoot;				// ����������[�g�Ƃ��Ĉ�����
	int nBMFullPath;			// ��������t���p�X�ŕ\��
	int nTagReverse;			// �^�C�g���A�A�[�e�B�X�g�𔽓]
	int nTimeInList;			// �v���C���X�g�ōX�V�������擾����
	int nHotKey[HOTKEY_COUNT];	// �z�b�g�L�[
	int nTreeWidth;				// �c���[�̕�
	int nExistCheck;
	int nCompact;				// �R���p�N�g���[�h
	int nFontHeight;			// �t�H���g�̍���
	int nFontStyle;				// �t�H���g�̃X�^�C��
	int nTextColor;				// �����̐F
	int nBkColor;				// �w�i�̐F
	int nPlayTxtCol;			// �Đ��ȕ�����
	int nPlayBkCol;				// �Đ��Ȕw�i
	int nPlayView;				// �Đ��Ȃ̕\�����@
	int nSeekAmount;			// �V�[�N��
	int nVolAmount;
	int nTabMulti;
	int nTabHide;				// �^�u����̎��̓^�u���B��
	int nShowHeader;			// �w�b�_�R���g���[����\������
	int nTrayClick[6];			// �N���b�N���̓���
	int nResume;
	int nResPosFlag;
	int nResPos;
	int nTreeState;
	int nShowStatus;
	int nCloseMin;
	int nZipSearch;
	int nTabBottom;
	int nOut32bit;
	int nFadeOut;

	int nMiniPanelEnd;

	char szStartPath[MAX_FITTLE_PATH];	// �X�^�[�g�A�b�v�p�X
	char szTypeList[MAX_EXT_COUNT][5];		// �����g���q
	char szBMPath[MAX_BM_SIZE][MAX_FITTLE_PATH];	// ������
	char szFontName[32];		// �t�H���g�̖��O
	char szFilerPath[MAX_FITTLE_PATH];	// �t�@�C���̃p�X
	char szWAPPath[MAX_FITTLE_PATH];
	char szLastFile[MAX_FITTLE_PATH];
	char szToolPath[MAX_FITTLE_PATH];
};

extern struct CONFIG g_cfg;
extern CHANNELINFO g_cInfo[2];
extern BOOL g_bNow;

LRESULT CALLBACK NewListProc(HWND, UINT, WPARAM, LPARAM); //���X�g�r���[�p�̃v���V�[�W��
BOOL CheckFileType(char *);

#endif
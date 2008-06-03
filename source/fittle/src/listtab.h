/*
 * ListTab.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _LISTTAB_H_
#define _LISTTAB_H_

#include <windows.h>
#include <commctrl.h>
#include "list.h"

#define ID_LIST 105
#define STACK_SIZE 10

// �I������Ă���^�u����ListTab�n���h�����擾
#define GetCurListTab(hTab) \
	GetListTab(hTab, TabCtrl_GetCurSel(hTab))

struct LISTTAB{
	char	szTitle[MAX_FITTLE_PATH];	// �^�u�̃^�C�g��
	HWND	hList;						// ���X�g�̃n���h��
	struct FILEINFO *pRoot;				// ���X�g�\���̃��[�g
	int		nPlaying;					// �Đ����̃C���f�b�N�X
	struct FILEINFO *pPrevStk[STACK_SIZE];	// �O�̋ȗ����̃X�^�b�N
	int		nStkPtr;					// �O�̋Ȃ̃X�^�b�N�|�C���^
	BOOL	bFocusRect;					// �^�u���t�H�[�J�X�\�����Ă邩
	int		nSortState;					// �\�[�g���
};

struct LISTTAB *MakeNewTab(HWND, char *, int);
struct LISTTAB *GetListTab(HWND, int);
int TraverseList(struct LISTTAB *);
int ChangeOrder(struct LISTTAB *, int);
int DeleteFiles(struct LISTTAB *);
void SendToPlaylist(struct LISTTAB *, struct LISTTAB *);
int MoveTab(HWND, int, int);
struct FILEINFO *PopPrevious(struct LISTTAB *);
struct FILEINFO *GetPlaying(struct LISTTAB *);
int PushPlaying(struct LISTTAB *, struct FILEINFO *);
int GetStackPtr(struct LISTTAB *);
BOOL SavePlaylists(HWND);
BOOL LoadPlaylists(HWND);
int RenameListTab(HWND, int, char *);
int RemoveListTab(HWND, int);
int InsertList(struct LISTTAB *, int, struct FILEINFO *);
BOOL CALLBACK TabNameDlgProc(HWND, UINT, WPARAM, LPARAM);
void SortListTab(struct LISTTAB *, int);

#endif
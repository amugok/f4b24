/*
 * Tree.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	�c���[�r���[���R���{�{�b�N�X�𑀍�
//

#include "fittle.h"
#include "tree.h"
#include "func.h"
#include "bass_tag.h"
#include "archive.h"
#include <assert.h>

// ���[�J���֐�
static BOOL CheckHaveChild(char *);
static int DeleteAllChildNode(HWND, HTREEITEM);

// �A�C�R���C���f�b�N�X
static int m_iFolderIcon = -1;
static int m_iListIcon = -1;

// �}�N��
#define OnEndDragTree()	ReleaseCapture()

// �w�肵���R���{�{�b�N�X�Ɏg�p�\�ȃh���C�u���*/
int SetDrivesToCombo(HWND hCB){
	DWORD dwSize;
	char *szBuff;
	COMBOBOXEXITEM citem = {0};
	int i,j;
	char szDrawBuff[MAX_FITTLE_PATH];

	citem.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	citem.cchTextMax = MAX_FITTLE_PATH;
	SendMessage(hCB, CB_RESETCONTENT, 0, 0);

	// �h���C�u��
	dwSize = GetLogicalDriveStrings(0, NULL); // �T�C�Y���擾
	szBuff = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize+1);
	if(!szBuff){
		MessageBox(GetParent(hCB), "�R���{�{�b�N�X�̏������Ɏ��s���܂����B���������m�ۂ��Ă��������B", "Fittle", MB_OK);
		return -1;
	}
	GetLogicalDriveStrings(dwSize, (LPTSTR)szBuff);
	for(i=0;i<(int)(dwSize-1);i+=4){
		citem.pszText = &szBuff[i];
		citem.iItem = i/4;
		citem.lParam = citem.iImage = citem.iSelectedImage = -1;
		SendMessage(hCB, CBEM_INSERTITEM, 0, (LPARAM)&citem);
	}
	HeapFree(GetProcessHeap(), 0, szBuff);

	// ������t�H���_��
	i /= 4;
	for(j=0;j<MAX_BM_SIZE;j++){
		if((g_cfg.szBMPath[j][0]=='\\' && g_cfg.szBMPath[j][1]=='\\') || PathIsDirectory(g_cfg.szBMPath[j])){
			lstrcpyn(szDrawBuff, g_cfg.szBMPath[j], MAX_FITTLE_PATH);
			PathAddBackslash(szDrawBuff);
			citem.pszText = szDrawBuff;
			citem.iItem = i;
			citem.lParam = citem.iImage = citem.iSelectedImage = m_iFolderIcon;
			SendMessage(hCB, CBEM_INSERTITEM, 0, (LPARAM)&citem);
			i++;
		}else if(IsPlayList(g_cfg.szBMPath[j]) || IsArchive(g_cfg.szBMPath[j])){
			wsprintf(szDrawBuff, "%s", g_cfg.szBMPath[j]);
			citem.pszText = szDrawBuff;
			citem.iItem = i;
			citem.lParam = citem.iImage = citem.iSelectedImage = m_iFolderIcon;
			SendMessage(hCB, CBEM_INSERTITEM, 0, (LPARAM)&citem);
			i++;
		}
	}
	SendMessage(hCB, CB_SETCURSEL, 
		(WPARAM)SendMessage(hCB, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)(char *)"C:\\"), 0);
	// �A�C�R���擾
	if(g_cfg.nTreeIcon) RefreshComboIcon(hCB);
	return i;
}

// �R���{�{�b�N�X�̃A�C�R�������t���b�V��
void RefreshComboIcon(HWND hCB){
	int nCnt;
	int i;
	SHFILEINFO shfi;
	COMBOBOXEXITEM cbi = {0};
	char szPath[MAX_FITTLE_PATH];

	cbi.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	cbi.cchTextMax = MAX_FITTLE_PATH;
	cbi.pszText = szPath;
	nCnt = SendMessage(hCB, CB_GETCOUNT, 0, 0);
	for(i=0;i<nCnt;i++){
		cbi.iItem = i;
		SendMessage(hCB, CBEM_GETITEM, (WPARAM)0, (LPARAM)&cbi);
		if(cbi.pszText[3]) break;	// �t�H���_�ɂȂ�����o��
		SHGetFileInfo(cbi.pszText, 0, &shfi, sizeof(shfi), 
			SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
		DestroyIcon(shfi.hIcon);
		cbi.lParam = cbi.iImage = cbi.iSelectedImage = shfi.iIcon;
		SendMessage(hCB, CBEM_SETITEM, (WPARAM)0, (LPARAM)&cbi);
	}
	return;
}

// bShow��TRUE�Ȃ�A�C�R���\���AFALSE�Ȃ�A�C�R����\��
int InitTreeIconIndex(HWND hCB, HWND hTV, BOOL bShow){
	char szIconPath[MAX_FITTLE_PATH];
	SHFILEINFO shfi;
	HIMAGELIST hSysImglst;

	// �C���[�W���X�g���Z�b�g
	if(bShow){
		hSysImglst = (HIMAGELIST)SHGetFileInfo("", 0, &shfi,
			sizeof(shfi), SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
			ImageList_SetBkColor(hSysImglst, (COLORREF)g_cfg.nBkColor);
	}else{
		hSysImglst = NULL;
	}
	SendMessage(hCB, CBEM_SETIMAGELIST, 0, (LPARAM)hSysImglst);
	TreeView_SetImageList(hTV, hSysImglst, TVSIL_NORMAL);

	// �t�H���_�A�C�R���C���f�b�N�X�擾
	GetModuleParentDir(szIconPath);
	SHGetFileInfo(szIconPath, 0, &shfi, sizeof(SHFILEINFO),
		SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
	DestroyIcon(shfi.hIcon);
	m_iFolderIcon = shfi.iIcon;

	// ���X�g�A�C�R���C���f�b�N�X�擾
	m_iListIcon = GetArchiveIconIndex("*.m3u");

	return 0;
}

// �h���C�u�m�[�h�쐬
HTREEITEM MakeDriveNode(HWND hCB, HWND hTV){
	int nNowCBIndex;
	char szNowDrive[MAX_FITTLE_PATH] = {0};
	TV_INSERTSTRUCT tvi = {0};
	HTREEITEM hDriveNode;

	//�R���{�{�b�N�X�̑I�𕶎���擾
	nNowCBIndex = (int)SendMessage(hCB, CB_GETCURSEL, 0, 0);
	SendMessage(hCB, CB_GETLBTEXT, (WPARAM)nNowCBIndex, (LPARAM)szNowDrive);
	//�c���[�r���[�Ƀh���C�u�ǉ�
	TreeView_DeleteAllItems(hTV);
	tvi.hInsertAfter = TVI_LAST;
	tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.hParent = TVI_ROOT;
	tvi.item.pszText = szNowDrive;
	tvi.item.iImage = tvi.item.iSelectedImage = 
		(int)SendMessage(hCB, CB_GETITEMDATA, (int)nNowCBIndex, 0);
	hDriveNode = TreeView_InsertItem(hTV, &tvi);
	// �_�~�[�m�[�h��ǉ�
	tvi.hParent = hDriveNode;
	TreeView_InsertItem(hTV, &tvi);
	// �W�J
	TreeView_Expand(hTV, hDriveNode, TVE_EXPAND);
	return hDriveNode;
}

// �w��TreeView�̎w��m�[�h�Ƀc���[���K�w���
HTREEITEM MakeTwoTree(HWND hTV,	HTREEITEM hTarNode){
	HANDLE hFind;
	HTREEITEM hChildNode = NULL;
	TV_INSERTSTRUCT tvi = {0};
	WIN32_FIND_DATA wfd;
	char szParentPath[MAX_FITTLE_PATH];  //�e�f�B���N�g��(�t���p�X)
	char szParentPathW[MAX_FITTLE_PATH]; //�e�f�B���N�g��("\*.*"��ǉ�)
	char szTargetPath[MAX_FITTLE_PATH];  //�������ꂽ�f�B���N�g��(�t���p�X)

	DeleteAllChildNode(hTV, hTarNode);
	GetPathFromNode(hTV, hTarNode, szParentPath);

	if(szParentPath[3]=='\0') szParentPath[2] = '\0';	// �h���C�u�΍�
	wsprintf(szParentPathW, "%s\\*.*", szParentPath);
	tvi.hInsertAfter = TVI_SORT;
	tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.hParent = hTarNode;
	tvi.item.pszText = (char *)wfd.cFileName;
	hFind = FindFirstFile(szParentPathW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			// �S�~�ȊO�̃f�B���N�g����������ǉ�
			if(wfd.cFileName[0]!='\0' && lstrcmp((char *)wfd.cFileName, ".") && lstrcmp((char *)wfd.cFileName, "..")){
				if(g_cfg.nHideShow || !(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)){
					// �p�X�̊���
					wsprintf(szTargetPath,"%s\\%s", szParentPath, (char *)wfd.cFileName);
					if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
						//----�f�B���N�g���̏���----
						tvi.item.iImage = tvi.item.iSelectedImage = m_iFolderIcon;
						hChildNode = TreeView_InsertItem(hTV, &tvi);
						if(CheckHaveChild(szTargetPath)){
							tvi.hParent = hChildNode;
							TreeView_InsertItem(hTV, &tvi);
							tvi.hParent = hTarNode;
						}
					}else if(IsArchive(szTargetPath)){
						tvi.item.iImage = tvi.item.iSelectedImage = GetArchiveIconIndex(szTargetPath);
						hChildNode = TreeView_InsertItem(hTV, &tvi);
					}else if(IsPlayList(szTargetPath)){
						tvi.item.iImage = tvi.item.iSelectedImage = m_iListIcon;
						hChildNode = TreeView_InsertItem(hTV, &tvi);
					}
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	return hChildNode;
}

// �w��m�[�h�ȉ��Ƀf�B���N�g�������݂����TRUE��Ԃ�
BOOL CheckHaveChild(char *szTargetPath){
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	char szNowDir[MAX_FITTLE_PATH];
	char szTargetPathW[MAX_FITTLE_PATH];

	if(g_cfg.nAllSub) return TRUE;	// AvestaOption
	wsprintf(szTargetPathW, "%s\\*.*", szTargetPath);
	hFind = FindFirstFile(szTargetPathW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			wsprintf(szNowDir, "%s\\%s", szTargetPath, (char *)wfd.cFileName);
			//�S�~�ȊO�̃f�B���N�g����������TRUE
			if(lstrcmp((char *)wfd.cFileName, ".") && lstrcmp((char *)wfd.cFileName, "..")){
				if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || IsPlayList(szNowDir) || IsArchive(szNowDir)){
					FindClose(hFind);
					return TRUE;
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	return FALSE; //�q�f�B���N�g�����Ȃ����FALSE
}

// �p�X�����Ƀc���[�����@�������c
HTREEITEM MakeTreeFromPath(HWND hTV, HWND hCB, char *szSetPath){
	int nDriveIndex;
	char szDriveName[MAX_FITTLE_PATH];
	char szTempRoot[MAX_FITTLE_PATH] = {0};
	char szCompPath[MAX_FITTLE_PATH];
	char szLabel[MAX_FITTLE_PATH];
	char szErrorMsg[300];
	HTREEITEM hDriveNode, hCompNode, hFoundNode;
	TVITEM ti;
	int i, j, len;

#ifdef _DEBUG
	DWORD dTime;
	dTime = GetTickCount();
#endif

	// \���폜
	PathRemoveBackslash(szSetPath);

	// �p�X���L�����`�F�b�N
	if((!FILE_EXIST(szSetPath)) || (lstrlen(szSetPath)==0)){
		wsprintf(szErrorMsg, "%s\n�p�X��������܂���B", szSetPath);
		MessageBox(hTV, szErrorMsg, "Fittle", MB_OK | MB_ICONEXCLAMATION);
		return NULL;
	}

	// �h���C�u�{�b�N�X�̕ύX
	if(g_cfg.nBMRoot){
		for(i=0;i<MAX_BM_SIZE;i++){
			// ������I���Ŕ�����
			if(!g_cfg.szBMPath[i][0]) break;

			if(StrStrI(szSetPath, g_cfg.szBMPath[i])){
				len = lstrlen(g_cfg.szBMPath[i]);
				if(len>lstrlen(szTempRoot) && (szSetPath[len]=='\0' || szSetPath[len]=='\\')){
					lstrcpyn(szTempRoot, g_cfg.szBMPath[i], MAX_FITTLE_PATH);
				}
			}
		}
	}

	// ���[�g�ɂȂ邵���肪���݂���
	if(szTempRoot[0]){
		lstrcpyn(szDriveName, szTempRoot, MAX_FITTLE_PATH);
	}else{
		lstrcpyn(szDriveName, szSetPath, MAX_FITTLE_PATH);
		PathStripToRoot(szDriveName);
		//lstrcpyn(szDriveName, szSetPath, 4);
	}

	// �`�捂�����̂��ߍĕ`���j�~
	LockWindowUpdate(GetParent(hTV));

	// szDriveName�Ƀh���C�u��ݒ�
	MyPathAddBackslash(szDriveName);
	nDriveIndex = (int)SendMessage(hCB, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)szDriveName);

	assert(nDriveIndex>=0);

	if(nDriveIndex<0){
		return NULL;
	}

	SendMessage(hCB, CB_SETCURSEL, (WPARAM)nDriveIndex, 0);

	i = lstrlen(szDriveName);
	hDriveNode = MakeDriveNode(hCB, hTV);
	PathRemoveBackslash(szDriveName);
	// �n���ꂽ�p�X���h���C�u��������h���C�u�m�[�h��Ԃ��ďI���
	if(!lstrcmp(szSetPath, szDriveName)/*szSetPath[lstrlen(szDriveName)]=='\0'*/){
		TreeView_SelectItem(hTV, hDriveNode);
		// �ĕ`�������
		LockWindowUpdate(NULL);
		return hDriveNode;
	}

	// ������
	ti.mask = TVIF_TEXT ;
	ti.cchTextMax = MAX_FITTLE_PATH;
	ti.pszText = szLabel;
	hFoundNode = hDriveNode;
	// �h���C�u�ȍ~
	do{
		j=0;
		// �t�H���_�����擾
		do{
			szCompPath[j] = szSetPath[i];
			if(IsDBCSLeadByte(szSetPath[i])){
				// �Q�o�C�g�����̏���
				i++;
				j++;
				szCompPath[j] = szSetPath[i];
			}
			i++;
			j++;
		}while(!(szSetPath[i]=='\0' || szSetPath[i]=='\\'));
		szCompPath[j] = '\0';

		// �t�H���_���ƈ�v����m�[�h��W�J
		hCompNode = TreeView_GetChild(hTV, hFoundNode);
		while(hCompNode){
			ti.hItem = hCompNode;
			TreeView_GetItem(hTV, &ti);
			if(!lstrcmpi(szCompPath, ti.pszText)){
				// ����
				TreeView_Expand(hTV, ti.hItem, TVE_EXPAND);
				hFoundNode = ti.hItem;
				break;
			}else{
				// ���̃m�[�h
				hCompNode = TreeView_GetNextSibling(hTV, hCompNode);
			}
		}
		// ��v����t�H���_���Ȃ������ꍇ�i�B���t�H���_�Ȃǁj�͒ǉ�
		if(!hCompNode){
			TV_INSERTSTRUCT tvi;
			tvi.hInsertAfter = TVI_SORT;
			tvi.hParent = hFoundNode;
			tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvi.item.pszText = szCompPath;
			tvi.item.cchTextMax = MAX_FITTLE_PATH;
			tvi.item.iImage = tvi.item.iSelectedImage = m_iFolderIcon;
			hFoundNode = TreeView_InsertItem(hTV, &tvi);
			tvi.hParent = hFoundNode;
			TreeView_InsertItem(hTV, &tvi);	// �_�~�[��ǉ�
			TreeView_Expand(hTV, hFoundNode, TVE_EXPAND);
		}
	}while(szSetPath[i++]!='\0');
	TreeView_SelectItem(hTV, hFoundNode);

#ifdef _DEBUG
	char szBuff[100];
	wsprintf(szBuff, "Construct tree time: %d ms\n", GetTickCount() - dTime);
	OutputDebugString(szBuff);
#endif

	// �ĕ`�������
	LockWindowUpdate(NULL);

	return hFoundNode;
}

// �w�肵���c���[�r���[�̑I���m�[�h�̑S�q�m�[�h���폜
int DeleteAllChildNode(HWND hTV, HTREEITEM hTarNode){
	HTREEITEM hDelNode;
	HTREEITEM hNextNode;

	for(hDelNode = TreeView_GetChild(hTV, hTarNode);hDelNode;hDelNode = hNextNode){
		hNextNode = TreeView_GetNextSibling(hTV, hDelNode);
		TreeView_DeleteItem(hTV, hDelNode);
	}
	return 0;
}

// �w��c���[�r���[�̎w��m�[�h����t���p�X���擾
int GetPathFromNode(HWND hTV, HTREEITEM hTargetNode, char *pszBuff){
	TVITEM ti;
	HTREEITEM hParentNode;
	char szUnderPath[MAX_FITTLE_PATH];		// �I���m�[�h�ȉ��̃p�X��ۑ�
	char szNowNodeLabel[MAX_FITTLE_PATH];	// �I���m�[�h�̃��x��

	if(!hTargetNode){
		lstrcpy(pszBuff, "");
		return 0;
	}
	// �����ݒ�
	szUnderPath[0] = '\0';
	ti.mask = TVIF_TEXT;
	ti.hItem = hTargetNode;
	ti.pszText = szNowNodeLabel;
	ti.cchTextMax = MAX_FITTLE_PATH;

	// ���݂̃m�[�h�����擾
	TreeView_GetItem(hTV, &ti);
	lstrcpyn(szNowNodeLabel, ti.pszText, MAX_FITTLE_PATH);

	while(!(ti.pszText[1]==':' && ti.pszText[2]=='\\') && !(ti.pszText[0]=='\\' && ti.pszText[1]=='\\')){
		wsprintf(szNowNodeLabel, "%s\\", ti.pszText);
		lstrcat(szNowNodeLabel, szUnderPath);
		lstrcpyn(szUnderPath, szNowNodeLabel, MAX_FITTLE_PATH);
		// �e�m�[�h���烉�x�����擾
		hParentNode = TreeView_GetParent(hTV, ti.hItem);
		ti.hItem = hParentNode;
		TreeView_GetItem(hTV, &ti);
	}

	lstrcat(szNowNodeLabel, szUnderPath);
	PathRemoveBackslash(szNowNodeLabel);
	lstrcpyn(pszBuff, szNowNodeLabel, MAX_FITTLE_PATH);
	return 0;
}

//�c���[�̕\���ʒu�𒲐�
int MyScroll(HWND hTV){
	SCROLLINFO scrlinfo;
	RECT rcItem;

	scrlinfo.cbSize = sizeof(scrlinfo);
	scrlinfo.fMask = SIF_POS;
	GetScrollInfo(hTV, SB_HORZ, &scrlinfo);
	TreeView_GetItemRect(hTV, TreeView_GetFirstVisible(hTV), &rcItem, TRUE);
	scrlinfo.nPos += rcItem.left - 20 - 20*(TreeView_GetImageList(hTV, TVSIL_NORMAL)?1:0);
	SetScrollInfo(hTV, SB_HORZ, &scrlinfo, TRUE);
	return scrlinfo.nPos;
}

void OnBeginDragTree(HWND hTV){
	SetOLECursor(3);
	SetCapture(hTV);
	return;
}

// �R���{�{�b�N�X�̃v���V�[�W��
LRESULT CALLBACK NewComboProc(HWND hCB, UINT msg, WPARAM wp, LPARAM lp){
	static HBRUSH hBrush = NULL;
	int crlBk;

	switch(msg){
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
			if(hBrush)	DeleteObject(hBrush);	// �O�Ɏg���Ă����̂��폜

			SetTextColor((HDC)wp, (COLORREF)g_cfg.nTextColor);
			SetBkColor((HDC)wp, (COLORREF)g_cfg.nBkColor);
			crlBk = GetBkColor((HDC)wp);
			hBrush = (HBRUSH)CreateSolidBrush((COLORREF)crlBk);
			return (LRESULT)hBrush;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hCB, GWLP_USERDATA), hCB, msg, wp, lp);
}

// �G�f�B�b�g�{�b�N�X�̃v���V�[�W��
LRESULT CALLBACK NewEditProc(HWND hEB, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_CHAR:
			if((int)wp=='\t'){
				if(GetKeyState(VK_SHIFT) < 0){
					SetFocus(GetWindow(GetDlgItem(GetParent(GetParent(hEB)), ID_TAB), GW_CHILD));
				}else{
					SetFocus(GetNextWindow(GetParent(hEB), GW_HWNDNEXT));
				}
				return 0;
			}
			break;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hEB, GWLP_USERDATA), hEB, msg, wp, lp);
}

/*
 * ListTag.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	�^�u��PARAM�Ɋ֘A�t����ꂽLISTTAB�\���̂𑀍�
//

#include "fittle.h"
#include "listtab.h"
#include "func.h"
#include "bass_tag.h"
#include "archive.h"

// �V�����^�u���������쐬
struct LISTTAB *MakeNewTab(HWND hTab, LPTSTR szTabName, int nItem){
	struct LISTTAB *pNew = NULL;
	HWND hList = NULL;
	WNDPROC OldProc = NULL;
	TCITEM tci;

	pNew = (struct LISTTAB *)HAlloc(sizeof(struct LISTTAB));
	if(!pNew){
		MessageBox(GetParent(hTab), TEXT("�^�u�̍쐬�Ɏ��s���܂����B���������m�ۂ��Ă��������B"), TEXT("Fittle"), MB_OK);
		return NULL;
	}
	// �\���̂̏�����
	lstrcpyn(pNew->szTitle, szTabName, MAX_FITTLE_PATH);
	pNew->pRoot = NULL;
	pNew->nPlaying = -1;
	pNew->nStkPtr = -1;
	pNew->bFocusRect = FALSE;
	// �^�u�A�C�e���̐ݒ�
	tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tci.iImage = -1;
	tci.pszText = szTabName;
	tci.cchTextMax = MAX_FITTLE_PATH;
	tci.lParam = (LPARAM)pNew;	// �^�u�ɍ\���̂��֘A�t����
	if(nItem==-1) nItem = TabCtrl_GetItemCount(hTab);
	TabCtrl_InsertItem(hTab, nItem, &tci);

	// ���X�g�r���[
	hList = CreateWindowEx(WS_EX_CLIENTEDGE,
		WC_LISTVIEW,
		NULL,
		WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA | (g_cfg.nShowHeader?0:LVS_NOCOLUMNHEADER),// | WS_VISIBLE | WS_CLIPSIBLINGS //| WS_CLIPCHILDREN,
		0, 0, 500, 500,
		hTab,
		(HMENU)ID_LIST,
		(HINSTANCE)(LONG_PTR)GetWindowLongPtr(GetParent(hTab), GWLP_HINSTANCE),
		NULL);
	if(!hList){
		MessageBox(GetParent(hTab), TEXT("���X�g�̍쐬�Ɏ��s���܂����B���������m�ۂ��Ă��������B"), TEXT("Fittle"), MB_OK);
		return NULL;
	}
	pNew->hList = hList;

	// INI�t�@�C���̈ʒu���擾
	TCHAR szINIPath[MAX_FITTLE_PATH];
	GetModuleParentDir(szINIPath);
	lstrcat(szINIPath, TEXT("Fittle.ini"));

	// �J�����̐ݒ�
	LVCOLUMN lvcol;
	lvcol.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	lvcol.cx = GetPrivateProfileInt(TEXT("Column"), TEXT("Width0"), 200, szINIPath);
	lvcol.iSubItem = 0;
	lvcol.pszText = TEXT("�t�@�C����");
	ListView_InsertColumn(hList, 0, &lvcol);
	lvcol.cx = GetPrivateProfileInt(TEXT("Column"), TEXT("Width1"), 70, szINIPath);
	lvcol.fmt = LVCFMT_RIGHT;
	lvcol.pszText = TEXT("�T�C�Y");
	ListView_InsertColumn(hList, 1, &lvcol);
	lvcol.fmt = LVCFMT_LEFT;
	lvcol.cx = GetPrivateProfileInt(TEXT("Column"), TEXT("Width2"), 40, szINIPath);
	lvcol.pszText = TEXT("���");
	ListView_InsertColumn(hList, 2, &lvcol);
	lvcol.cx = GetPrivateProfileInt(TEXT("Column"), TEXT("Width3"), 130, szINIPath);
	lvcol.pszText = TEXT("�X�V����");
	ListView_InsertColumn(hList, 3, &lvcol);

	pNew->nSortState = GetPrivateProfileInt(TEXT("Column"), TEXT("Sort"), 0, szINIPath);;	// �t���p�X�Ń\�[�g

	ListView_SetExtendedListViewStyle(hList, (g_cfg.nGridLine?LVS_EX_GRIDLINES:0) | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | 0x00010000/*LVS_EX_DOUBLEBUFFER*/);

	//�T�u�N���X���B�O�̃v���V�[�W����Window�Ɋ֘A�t����B
	OldProc = (WNDPROC)(LONG_PTR)SetWindowLongPtr(hList,
		GWLP_WNDPROC, (LONG_PTR)NewListProc);
	SetWindowLongPtr(hList, GWLP_USERDATA, (LONG_PTR)OldProc);
	// ���X�g�r���[�̃h���b�O���h���b�v������
	DragAcceptFiles(hList, TRUE);

	// �J���[
	ListView_SetTextColor(hList, (COLORREF)g_cfg.nTextColor);
	ListView_SetTextBkColor(hList, (COLORREF)g_cfg.nBkColor);
	ListView_SetBkColor(hList, (COLORREF)g_cfg.nBkColor);
	return pNew;
}

// �C���f�b�N�X -> ���X�g�^�u�\���̂ւ̃|�C���^
struct LISTTAB *GetListTab(HWND hTab, int nIndex){
	TCITEM tci;

	tci.mask = TCIF_PARAM;
	TabCtrl_GetItem(hTab, nIndex, &tci);
	return (struct LISTTAB *)tci.lParam;
}

// ���X�g�\���𑖍�
int TraverseList(struct LISTTAB *pListTab){
	LVITEM item = {0};
	int i = 0;
	struct FILEINFO *pTmp = NULL;
	struct LISTTAB *pFolTab;
	BOOL bVisible = FALSE;


#ifdef _DEBUG
	DWORD dTime;
	TCHAR szBuff[100];
	dTime = GetTickCount();
#endif

	if(IsWindowVisible(pListTab->hList)){
		LockWindowUpdate(GetParent(GetParent(pListTab->hList)));
		ShowWindow(pListTab->hList, SW_HIDE);
		bVisible = TRUE;
	}

	//ListView_DeleteAllItems(pListTab->hList);
	//ListView_SetItemCountEx(pListTab->hList, 0, LVSICF_NOINVALIDATEALL);

	item.mask = LVIF_TEXT;
	item.iSubItem = 0;
	item.cchTextMax = MAX_FITTLE_PATH;
	item.pszText = LPSTR_TEXTCALLBACK;

	pFolTab = GetListTab(GetParent(pListTab->hList), 0);

	for(pTmp=pListTab->pRoot;pTmp!=NULL;pTmp = pTmp->pNext){
		item.iItem = i;
		// ��������
		if(pListTab==pFolTab && pListTab->nPlaying==-1
		&& !lstrcmp(pTmp->szFilePath, g_cInfo[g_bNow].szFilePath)){
			pListTab->nPlaying = i;
			pTmp->bPlayFlag = TRUE;
		}

		i++;
	}
	ListView_SetItemCountEx(pListTab->hList, i, LVSICF_NOINVALIDATEALL);
	ListView_SetItemState(pListTab->hList, -1, 0, (LVIS_SELECTED | LVIS_FOCUSED));
	ListView_SetItemState(pListTab->hList, 0, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
	
	if(bVisible){
		ShowWindow(pListTab->hList, SW_SHOW);
		LockWindowUpdate(NULL);
	}

#ifdef _DEBUG
	wsprintf(szBuff, TEXT("Travese  time:\t%d ms\n"), GetTickCount() - dTime);
	OutputDebugString(szBuff);
#endif

	return i;
}

/*List�\���̏��Ԃ�ւ���*/
int ChangeOrder(struct LISTTAB *pListTab, int nAfter){
	int nSelect;
	int nCount;
	struct FILEINFO *pSubRoot = NULL;
	struct FILEINFO *pSubTale = NULL;
	struct FILEINFO *pPlaying = NULL;

	//�擪�I���t�@�C���̃C���f�b�N�X�擾
	nSelect = ListView_GetNextItem(pListTab->hList, -1, LVNI_SELECTED);
	if(nSelect==-1)	return -1;	// �I���t�@�C����������Δ�����

	// ����
	nCount = ListView_GetItemCount(pListTab->hList);
	if(nCount==nAfter) nAfter--;
	if(nSelect<nAfter) nAfter++;

	//���ݍĐ����Ă���t�@�C���̃|�C���^���擾
	pPlaying = GetPtrFromIndex(pListTab->pRoot, pListTab->nPlaying);

	// �`�惍�b�N
	LockWindowUpdate(GetParent(pListTab->hList));

	while((nSelect = ListView_GetNextItem(pListTab->hList, -1, LVNI_SELECTED))!=-1){
		// �T�u���X�g�ɒǉ�
		if(!pSubRoot){
			pSubRoot = GetPtrFromIndex(pListTab->pRoot, nSelect);
			pSubTale = pSubRoot;
		}else{
			pSubTale->pNext = GetPtrFromIndex(pListTab->pRoot, nSelect);
			pSubTale = pSubTale->pNext;
		}
		// ���C�����X�g���番��
		if(nSelect==0){
			pListTab->pRoot = GetPtrFromIndex(pListTab->pRoot, 0)->pNext;
		}else{
			GetPtrFromIndex(pListTab->pRoot, nSelect-1)->pNext = GetPtrFromIndex(pListTab->pRoot, nSelect)->pNext;
		}
		// �ʒu����
		if(nSelect<nAfter){
			nAfter--;
		}
		// �\���폜
		ListView_DeleteItem(pListTab->hList, nSelect);
	}
	pSubTale->pNext = NULL;	// �T�u���[�g�����

	// �����������X�g������
	InsertList(pListTab, nAfter, pSubRoot);
	ListView_EnsureVisible(pListTab->hList, nAfter, FALSE);

	// �Đ����t�@�C���̃C���f�b�N�X��ύX
	pListTab->nPlaying = GetIndexFromPtr(pListTab->pRoot, pPlaying);

	// �`�惍�b�N����
	LockWindowUpdate(NULL);
	return 0;
}

/*���X�g�ƃ��X�g�r���[�A�C�e���𓯎��ɍ폜*/
int DeleteFiles(struct LISTTAB *pListTab){
	int nIndex;
	int nBefore = -1;

	nIndex = ListView_GetNextItem(pListTab->hList, -1, LVNI_SELECTED);
	while(nIndex!=-1){
		nBefore = nIndex;
		ListView_DeleteItem(pListTab->hList, nIndex);
		DeleteAList(GetPtrFromIndex(pListTab->pRoot, nIndex), &(pListTab->pRoot));
		if(nIndex<=pListTab->nPlaying) pListTab->nPlaying--;
		if(nIndex==ListView_GetItemCount(pListTab->hList)) nIndex--;
		nIndex = ListView_GetNextItem(pListTab->hList, -1, LVNI_SELECTED);
	}
	ListView_SetItemState(pListTab->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_SetItemState(pListTab->hList, nBefore, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	return nIndex;
}

// �t�@�C�����X�g -> �t�@�C�����X�g
void SendToPlaylist(struct LISTTAB *ltFrom, struct LISTTAB *ltTo){
	int nIndex = -1;
	FILEINFO *pTmp;

	while((nIndex = ListView_GetNextItem(ltFrom->hList, nIndex, LVNI_SELECTED))!=-1){
		pTmp = GetPtrFromIndex(ltFrom->pRoot, nIndex);
		AddList(&(ltTo->pRoot), pTmp->szFilePath, pTmp->szSize, pTmp->szTime);
	}
	TraverseList(ltTo);
}

// ���X�g�^�u���l�[��
int RenameListTab(HWND hTab, int nItem, LPTSTR szLabel){
	TCITEM tci;

	lstrcpyn(GetListTab(hTab, nItem)->szTitle, szLabel, MAX_FITTLE_PATH);
	tci.mask = TCIF_TEXT;
	tci.cchTextMax = MAX_FITTLE_PATH;
	tci.pszText = szLabel;
	TabCtrl_SetItem(hTab, nItem, &tci);
	return 0;
}

// ���X�g�^�u�폜
int RemoveListTab(HWND hTab, int nItem){
	RECT rcTab;
	DeleteAllList(&(GetListTab(hTab, nItem)->pRoot));
	DestroyWindow(GetListTab(hTab, nItem)->hList);
	HFree(GetListTab(hTab, nItem));
	TabCtrl_DeleteItem(hTab, nItem);
	// �^�u�̍s�����ς��ƃ��X�g�r���[������邽�߂�����C��
	GetClientRect(hTab, &rcTab);
	if(!(g_cfg.nTabHide && TabCtrl_GetItemCount(hTab)==1)){
		TabCtrl_AdjustRect(hTab, FALSE, &rcTab);
	}
	MoveWindow(GetCurListTab(hTab)->hList,
		rcTab.left,
		rcTab.top,
		rcTab.right - rcTab.left,
		rcTab.bottom - rcTab.top,
		TRUE);
	return 0;
}

// �^�u�ړ�
int MoveTab(HWND hTab, int nTarget, int nDir){
	TCITEM tciFrom, tciTo;
	TCHAR szFrom[MAX_FITTLE_PATH], szTo[MAX_FITTLE_PATH];

	// �����ݒ�
	tciFrom.mask = tciTo.mask = TCIF_PARAM | TCIF_TEXT;
	tciFrom.cchTextMax = tciTo.cchTextMax = MAX_FITTLE_PATH;
	tciFrom.pszText = szFrom;
	tciTo.pszText = szTo;

	// ���擾
	TabCtrl_GetItem(hTab, nTarget, &tciFrom);
	TabCtrl_GetItem(hTab, nTarget + nDir, &tciTo);

	// ���X���b�v
	TabCtrl_SetItem(hTab, nTarget, &tciTo);
	TabCtrl_SetItem(hTab, nTarget + nDir, &tciFrom);

	// �t�H�[�J�X�̈ړ�
	if(TabCtrl_GetCurFocus(hTab)==nTarget){
		TabCtrl_SetCurFocus(hTab, nTarget + nDir);
	}else if(TabCtrl_GetCurFocus(hTab)==nTarget + nDir){
		TabCtrl_SetCurFocus(hTab, nTarget);
	}
	//InvalidateRect(hTab, NULL, FALSE);
	
	return 0;
}

// �O�̋ȗ������擾
struct FILEINFO *PopPrevious(struct LISTTAB *pListTab){
	if(pListTab->nStkPtr<0) return NULL;
	return pListTab->pPrevStk[pListTab->nStkPtr--];
}

// �X�^�b�N�|�C���^���ړ��������ɗ������擾
struct FILEINFO *GetPlaying(struct LISTTAB *pListTab){
	if(pListTab->nStkPtr<0) return NULL;
	return pListTab->pPrevStk[pListTab->nStkPtr];
}

// �O�̋ȗ������v�b�V��
int PushPlaying(struct LISTTAB *pListTab, struct FILEINFO *pPrev){
	int i;

	if(pListTab->nStkPtr<STACK_SIZE-1){
		pListTab->pPrevStk[++(pListTab->nStkPtr)] = pPrev;
	}else{
		for(i=0;i<STACK_SIZE-1;i++){
			pListTab->pPrevStk[i] = pListTab->pPrevStk[i+1];
		}
		pListTab->pPrevStk[i] = pPrev;
	}
	return pListTab->nStkPtr;
}

// �X�^�b�N�|�C���^���擾
int GetStackPtr(struct LISTTAB *pListTab){
	return pListTab->nStkPtr;
}

// ���X�g���\�[�g
void SortListTab(struct LISTTAB *pLT, int nSortType){
	struct FILEINFO *pPlaying;

	pPlaying = GetPtrFromIndex(pLT->pRoot, pLT->nPlaying); 
	pLT->nSortState = nSortType;
	MergeSort(&(pLT->pRoot), nSortType);
	TraverseList(pLT);
	pLT->nPlaying = GetIndexFromPtr(pLT->pRoot, pPlaying);
	return;
}	

// �����̃��X�g�ɐV���ȃ��X�g��}��
int InsertList(struct LISTTAB *pTo, int nStart, struct FILEINFO *pSub){
	int nMainCount;
	int nSubCount;
	int i;

	if(!pSub) return 0;

	// �C���f�b�N�X�̏���
	nMainCount = ListView_GetItemCount(pTo->hList);
	nSubCount = GetListCount(pSub);
	if(nStart<0) nStart = nMainCount;

	// ���X�g�r���[�����Ȃ�`������b�N
	/*if(IsWindowVisible(pTo->hList)){
		LockWindowUpdate(GetParent(GetParent(pTo->hList)));
		ShowWindow(pTo->hList, SW_HIDE);
		bVisible = TRUE;
	}*/

	// ���X�g������
	GetPtrFromIndex(pSub, nSubCount-1)->pNext = GetPtrFromIndex(pTo->pRoot, nStart);
	if(nStart!=0){
		GetPtrFromIndex(pTo->pRoot, nStart-1)->pNext = pSub;
	}else{
		pTo->pRoot = pSub;
	}
	ListView_SetItemCountEx(pTo->hList, nMainCount + nSubCount, LVSICF_NOINVALIDATEALL);

	// �I����Ԃ̐ݒ�
	ListView_SetItemState(pTo->hList, nStart, LVIS_FOCUSED, LVIS_FOCUSED);
	for(i=0;i<nSubCount;i++){
		ListView_SetItemState(pTo->hList, i+nStart, LVNI_SELECTED, LVNI_SELECTED);
	}

	/*if(bVisible){
		ShowWindow(pTo->hList, SW_SHOW);
		LockWindowUpdate(NULL);
	}*/

	InvalidateRect(pTo->hList, NULL, FALSE);

	return 0;
}

BOOL LoadPlaylists(HWND hTab){
	TCHAR szColPath[MAX_FITTLE_PATH];
	TCHAR szTempPath[MAX_FITTLE_PATH];
	LPTSTR szBuff, p;
	TCHAR szTime[50] = TEXT("-"), szSize[50] = TEXT("-");
	int i;
	struct LISTTAB *pNew;
	struct FILEINFO **pTale;
	HANDLE hFile;
	DWORD dwSize;
	DWORD dwAccBytes;

	// �t�@�C�����I�[�v�������[�h
	GetModuleParentDir(szColPath);
#ifdef UNICODE
	lstrcat(szColPath, TEXT("Playlist.fpu"));
#else
	lstrcat(szColPath, TEXT("Playlist.fpf"));
#endif

	hFile = CreateFile(szColPath, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;

	dwSize = GetFileSize(hFile, NULL) + sizeof(TCHAR);
	szBuff = (LPTSTR)HZAlloc(dwSize);
	if(!szBuff){
		MessageBox(GetParent(hTab), TEXT("�������̊m�ۂɎ��s���܂����B"), TEXT("Fittle"), MB_OK);
		return FALSE;
	}
	ReadFile(hFile, szBuff, dwSize, &dwAccBytes, NULL);
	CloseHandle(hFile);
	szBuff[dwAccBytes / sizeof(TCHAR)] = TEXT('\0');

	// �ǂݍ��񂾃o�b�t�@������
	for(p=szBuff;*p!=TEXT('\0');){
#ifdef UNICODE
		if (*p == (WCHAR)0xfeff) p++;
#endif
		// �^�C�g���ǂݍ���
		for(i=0;*p!=TEXT('\n') && *p!=TEXT('\r');p++){
			szTempPath[i++] = *p;
		}
		szTempPath[i] = TEXT('\0');
		pNew = MakeNewTab(hTab, szTempPath, -1);
		pTale = &(pNew->pRoot);
		while (*p == TEXT('\n') || *p == TEXT('\r'))
			p++;

		// �t�@�C���ǂݍ���
		while(*p==TEXT('\t')){
			i = 0;
#ifdef UNICODE
			if (*p == (WCHAR)0xfeff) p++;
#endif
			for(p+=1;*p!=TEXT('\n') && *p!=TEXT('\r');p++){
				szTempPath[i++] = *p;
			}
			szTempPath[i] = TEXT('\0');
			if(!g_cfg.nExistCheck || FILE_EXIST(szTempPath) || IsArchivePath(szTempPath) || IsURLPath(szTempPath)){
				if(g_cfg.nTimeInList){
					GetTimeAndSize(szTempPath, szSize, szTime);
				}
				pTale = AddList(pTale, szTempPath, szSize, szTime);
			}
			while (*p == TEXT('\n') || *p == TEXT('\r'))
				p++;
		}
		TraverseList(pNew);
	}
	HFree(szBuff);
	return TRUE;
}

// mkimg.dll�̃o�O����̂��ߑS�Ẵ��X�g��j��
static void RemoveAllTabs(HWND hTab, int nTabCount){
	for (int i=nTabCount;i>=0;i--){
		//RemoveListTab
		LISTTAB *pTab = GetListTab(hTab, i);
		DeleteAllList(&(pTab->pRoot));
		DestroyWindow(pTab->hList);
		HFree(pTab);
		TabCtrl_DeleteItem(hTab, i);
	}
}

static int WriteBuff(LPTSTR szBuff, int nBuffSize, int nBuffPos, LPCTSTR pData){
	int l = lstrlen(pData);
	if (nBuffPos + l <= nBuffSize){
		CopyMemory(szBuff + nBuffPos, pData, l * sizeof(TCHAR));
		nBuffPos += l;
	}
	return nBuffPos;
}

BOOL SavePlaylists(HWND hTab){
	TCHAR szColPath[MAX_FITTLE_PATH];
	LPTSTR szBuff;
	int nTabCount;
	int nBuffSize = 0;
	int nBuffPos = 0;
	int i;
	struct LISTTAB *pListTab;
	struct FILEINFO *pTmp;
	HANDLE hFile;
	DWORD dwAccBytes;

	GetModuleParentDir(szColPath);
#ifdef UNICODE
	lstrcat(szColPath, TEXT("Playlist.fpu"));
#else
	lstrcat(szColPath, TEXT("Playlist.fpf"));
#endif

	nTabCount = TabCtrl_GetItemCount(hTab) - 1;
	if(nTabCount<=0){
		DeleteFile(szColPath);
		RemoveAllTabs(hTab, nTabCount);
		return FALSE;
	}

	for(i=1;i<=nTabCount;i++){
		nBuffSize += ListView_GetItemCount(GetListTab(hTab, i)->hList) + 2; //�^�C�g������+1
	}
#ifdef UNICODE
	nBuffSize = sizeof(TCHAR) + nBuffSize * (MAX_FITTLE_PATH + 2) * sizeof(TCHAR);
#else
	nBuffSize *= (MAX_FITTLE_PATH + 1) * sizeof(TCHAR);
#endif

	szBuff = (LPTSTR)HZAlloc(nBuffSize);
	if(!szBuff){
		MessageBox(GetParent(hTab), TEXT("�������̊m�ۂɎ��s���܂����B"), TEXT("Fittle"), MB_OK);
		RemoveAllTabs(hTab, nTabCount);
		return FALSE;
	}

#ifdef UNICODE
	nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\xfeff"));
#endif

	for(i=1;i<=nTabCount;i++){
		pListTab = GetListTab(hTab, i);
		nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, pListTab->szTitle);
#ifdef UNICODE
		nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\r\n"));
#else
		nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\n"));
#endif

		for(pTmp = pListTab->pRoot;pTmp!=NULL;pTmp = pTmp->pNext){
			nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\t"));	// �t�@�C�����ʎq(\t)��}��
			nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, pTmp->szFilePath);
#ifdef UNICODE
			nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\r\n"));
#else
			nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\n"));
#endif
		}
#ifdef UNICODE
		nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\r\n"));
#else
		nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\n"));
#endif
	}

	RemoveAllTabs(hTab, nTabCount);

	// �t�@�C���̏�������
	hFile = CreateFile(szColPath, GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE){
		HFree(szBuff);
		return FALSE;
	}
	WriteFile(hFile, szBuff, (DWORD)nBuffPos * sizeof(TCHAR), &dwAccBytes, NULL);
	CloseHandle(hFile);

	// �������̊J��
	HFree(szBuff);

	return TRUE;
}

BOOL CALLBACK TabNameDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	static LPTSTR pszText;

	switch(msg){	
		case WM_INITDIALOG:
			pszText = (LPTSTR)lp;
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), pszText);
			SendMessage(GetDlgItem(hDlg, IDC_EDIT1), EM_SETSEL, (WPARAM)0, (LPARAM)lstrlen(pszText));
			SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
			return FALSE;

		case WM_COMMAND:
			TCHAR szTemp[MAX_FITTLE_PATH];
			switch(LOWORD(wp)){
				case IDOK:	// �ݒ�ۑ�
					GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), szTemp, MAX_FITTLE_PATH);
					if(lstrlen(szTemp)==0) return TRUE;
					lstrcpyn(pszText, szTemp, MAX_FITTLE_PATH);
					EndDialog(hDlg, TRUE);
					return TRUE;

				case IDCANCEL:	// �ݒ��ۑ������ɏI��
					EndDialog(hDlg, FALSE);
					return TRUE;
			}
			return FALSE;

		// �I��
		case WM_CLOSE:
			EndDialog(hDlg, FALSE);
			return FALSE;

		default:
			return FALSE;
	}
}

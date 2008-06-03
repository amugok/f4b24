/*
 * FindDlg.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */


//
//	リストから検索ダイアログを操作
//

#include "finddlg.h"
#include "fittle.h"
#include "list.h"
#include "func.h"
#include "listtab.h"

static int TraverseList2(HWND, struct FILEINFO *, char *);
static LRESULT CALLBACK NewEditProc(HWND, UINT, WPARAM, LPARAM);
void ToPlayList(struct FILEINFO **, struct FILEINFO *, char *);

static WNDPROC s_hOldEditProc = NULL;

BOOL CALLBACK FindDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	static HWND hList = NULL;
	static HWND hEdit = NULL;
	static struct FILEINFO *pRoot = NULL;
	//NMHDR *nm = (NMHDR *)lp;
	int nIndex;

	switch (msg)
	{
		case WM_INITDIALOG:	// 初期化
			hList = GetDlgItem(hDlg, IDC_LIST1);
			hEdit = GetDlgItem(hDlg, IDC_EDIT1);
			pRoot = (struct FILEINFO *)lp;
			s_hOldEditProc = (WNDPROC)(LONG_PTR)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)NewEditProc);
			TraverseList2(hList, pRoot, NULL);
			return TRUE;

		case WM_COMMAND:
			//EditBox
			char szSearch[MAX_FITTLE_PATH];
			if(lp==(LPARAM)hEdit && HIWORD(wp)==EN_CHANGE){
				GetWindowText(hEdit, szSearch, MAX_FITTLE_PATH);
				TraverseList2(hList, pRoot, (szSearch[0]!='\0'?szSearch:NULL));
				return TRUE;
			}
			//ListBox
			if(lp==(LPARAM)hList && HIWORD(wp)==LBN_DBLCLK){
				SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
				return TRUE;
			}
			//Dialog
			switch(LOWORD(wp))
			{
				case IDOK:
					nIndex = GetIndexFromPtr(pRoot, (struct FILEINFO *)SendMessage(hList, LB_GETITEMDATA, SendMessage(hList, LB_GETCURSEL, 0, 0), 0));
					EndDialog(hDlg, nIndex);
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, -1);
					return TRUE;

				case IDPLAYLIST:	// プレイリストに送る
					HWND hTab = GetDlgItem(GetParent(hDlg), ID_TAB);
					GetWindowText(hEdit, szSearch, MAX_FITTLE_PATH);
					//if(lstrlen(szSearch)==0) lstrcpy(szSearch, "Default");
					LISTTAB *pNew = MakeNewTab(hTab, szSearch, -1);
					ToPlayList(&(pNew->pRoot), pRoot, szSearch);
					TraverseList(pNew);
					if(lstrlen(szSearch)==0){
						RenameListTab(hTab, TabCtrl_GetItemCount(hTab)-1, "Default");
					}

					TabCtrl_SetCurFocus(hTab, TabCtrl_GetItemCount(hTab)-1);
					EndDialog(hDlg, -1);
					return TRUE;
			}
			return FALSE;

		case WM_KEYDOWN:
			nIndex = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
			if(wp==VK_DOWN){
				SendMessage(hList, LB_SETCURSEL, nIndex+1, 0);
			}else if(wp==VK_UP){
				SendMessage(hList, LB_SETCURSEL, (nIndex==0?0:nIndex-1), 0);
			}
			return TRUE;
		
		case WM_CLOSE:	//終了
			EndDialog(hDlg, -1);
			return TRUE;

		default:
			return FALSE;
	}
}

LRESULT CALLBACK NewEditProc(HWND hEB, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg)
	{
		case WM_GETDLGCODE:
			return DLGC_WANTARROWS | DLGC_WANTCHARS;
		case WM_KEYDOWN:
			if(wp==VK_DOWN || wp==VK_UP){
				return SendMessage(GetParent(hEB), msg, wp, lp);
			}else if(wp=='F'){
				if(GetKeyState(VK_CONTROL) < 0)
					SendMessage(GetParent(hEB), WM_CLOSE, 0, 0);
			}

			break;

	}
	return CallWindowProc(s_hOldEditProc, hEB, msg, wp, lp);
}

int TraverseList2(HWND hLB, struct FILEINFO *ptr, char *szPart){
	int i=0;

	LockWindowUpdate(GetParent(hLB));
	ShowWindow(hLB, SW_HIDE);

	SendMessage(hLB, LB_RESETCONTENT, 0, 0);
	for(;ptr;ptr = ptr->pNext)
	{
		if(!szPart || StrStrI(ptr->szFilePath, szPart))
		{
			SendMessage(hLB, LB_ADDSTRING, 0, (LPARAM)GetFileName(ptr->szFilePath));
			SendMessage(hLB, LB_SETITEMDATA, i++, (LPARAM)ptr);
		}
	}
	SendMessage(hLB, LB_SETCURSEL, 0, 0);

	ShowWindow(hLB, SW_SHOW);
	LockWindowUpdate(NULL);
	
	return (int)SendMessage(hLB, LB_GETSELCOUNT, 0, 0);
}

void ToPlayList(struct FILEINFO **pTo, struct FILEINFO *pFrom, char *pszPart){
	struct FILEINFO **pTale = pTo;
	for(;pFrom;pFrom = pFrom->pNext)
	{
		if(!*pszPart || StrStrI(pFrom->szFilePath, pszPart))
		{
			pTale = AddList(pTale, pFrom->szFilePath, pFrom->szSize, pFrom->szTime);
		}
	}
	return ;
}

/*
 * ListTag.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	タブのPARAMに関連付けられたLISTTAB構造体を操作
//

#include "fittle.h"
#include "listtab.h"
#include "func.h"
#include "bass_tag.h"
#include "archive.h"

// 新しいタブもろもろを作成
struct LISTTAB *MakeNewTab(HWND hTab, LPTSTR szTabName, int nItem){
	struct LISTTAB *pNew = NULL;
	HWND hList = NULL;
	WNDPROC OldProc = NULL;
	TCITEM tci;

	pNew = (struct LISTTAB *)HAlloc(sizeof(struct LISTTAB));
	if(!pNew){
		MessageBox(GetParent(hTab), TEXT("タブの作成に失敗しました。メモリを確保してください。"), TEXT("Fittle"), MB_OK);
		return NULL;
	}
	// 構造体の初期化
	lstrcpyn(pNew->szTitle, szTabName, MAX_FITTLE_PATH);
	pNew->pRoot = NULL;
	pNew->nPlaying = -1;
	pNew->nStkPtr = -1;
	pNew->bFocusRect = FALSE;
	// タブアイテムの設定
	tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tci.iImage = -1;
	tci.pszText = szTabName;
	tci.cchTextMax = MAX_FITTLE_PATH;
	tci.lParam = (LPARAM)pNew;	// タブに構造体を関連付ける
	if(nItem==-1) nItem = TabCtrl_GetItemCount(hTab);
	TabCtrl_InsertItem(hTab, nItem, &tci);

	// リストビュー
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
		MessageBox(GetParent(hTab), TEXT("リストの作成に失敗しました。メモリを確保してください。"), TEXT("Fittle"), MB_OK);
		return NULL;
	}
	pNew->hList = hList;

	// INIファイルの位置を取得
	TCHAR szINIPath[MAX_FITTLE_PATH];
	GetModuleParentDir(szINIPath);
	lstrcat(szINIPath, TEXT("Fittle.ini"));

	// カラムの設定
	LVCOLUMN lvcol;
	lvcol.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	lvcol.cx = GetPrivateProfileInt(TEXT("Column"), TEXT("Width0"), 200, szINIPath);
	lvcol.iSubItem = 0;
	lvcol.pszText = TEXT("ファイル名");
	ListView_InsertColumn(hList, 0, &lvcol);
	lvcol.cx = GetPrivateProfileInt(TEXT("Column"), TEXT("Width1"), 70, szINIPath);
	lvcol.fmt = LVCFMT_RIGHT;
	lvcol.pszText = TEXT("サイズ");
	ListView_InsertColumn(hList, 1, &lvcol);
	lvcol.fmt = LVCFMT_LEFT;
	lvcol.cx = GetPrivateProfileInt(TEXT("Column"), TEXT("Width2"), 40, szINIPath);
	lvcol.pszText = TEXT("種類");
	ListView_InsertColumn(hList, 2, &lvcol);
	lvcol.cx = GetPrivateProfileInt(TEXT("Column"), TEXT("Width3"), 130, szINIPath);
	lvcol.pszText = TEXT("更新日時");
	ListView_InsertColumn(hList, 3, &lvcol);

	pNew->nSortState = GetPrivateProfileInt(TEXT("Column"), TEXT("Sort"), 0, szINIPath);;	// フルパスでソート

	ListView_SetExtendedListViewStyle(hList, (g_cfg.nGridLine?LVS_EX_GRIDLINES:0) | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | 0x00010000/*LVS_EX_DOUBLEBUFFER*/);

	//サブクラス化。前のプロシージャはWindowに関連付ける。
	OldProc = (WNDPROC)(LONG_PTR)SetWindowLongPtr(hList,
		GWLP_WNDPROC, (LONG_PTR)NewListProc);
	SetWindowLongPtr(hList, GWLP_USERDATA, (LONG_PTR)OldProc);
	// リストビューのドラッグ＆ドロップを許可
	DragAcceptFiles(hList, TRUE);

	// カラー
	ListView_SetTextColor(hList, (COLORREF)g_cfg.nTextColor);
	ListView_SetTextBkColor(hList, (COLORREF)g_cfg.nBkColor);
	ListView_SetBkColor(hList, (COLORREF)g_cfg.nBkColor);
	return pNew;
}

// インデックス -> リストタブ構造体へのポインタ
struct LISTTAB *GetListTab(HWND hTab, int nIndex){
	TCITEM tci;

	tci.mask = TCIF_PARAM;
	TabCtrl_GetItem(hTab, nIndex, &tci);
	return (struct LISTTAB *)tci.lParam;
}

// リスト構造を走査
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
		// 太字復元
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

/*List構造の順番を替える*/
int ChangeOrder(struct LISTTAB *pListTab, int nAfter){
	int nSelect;
	int nCount;
	struct FILEINFO *pSubRoot = NULL;
	struct FILEINFO *pSubTale = NULL;
	struct FILEINFO *pPlaying = NULL;

	//先頭選択ファイルのインデックス取得
	nSelect = ListView_GetNextItem(pListTab->hList, -1, LVNI_SELECTED);
	if(nSelect==-1)	return -1;	// 選択ファイルが無ければ抜ける

	// 調整
	nCount = ListView_GetItemCount(pListTab->hList);
	if(nCount==nAfter) nAfter--;
	if(nSelect<nAfter) nAfter++;

	//現在再生しているファイルのポインタを取得
	pPlaying = GetPtrFromIndex(pListTab->pRoot, pListTab->nPlaying);

	// 描画ロック
	LockWindowUpdate(GetParent(pListTab->hList));

	while((nSelect = ListView_GetNextItem(pListTab->hList, -1, LVNI_SELECTED))!=-1){
		// サブリストに追加
		if(!pSubRoot){
			pSubRoot = GetPtrFromIndex(pListTab->pRoot, nSelect);
			pSubTale = pSubRoot;
		}else{
			pSubTale->pNext = GetPtrFromIndex(pListTab->pRoot, nSelect);
			pSubTale = pSubTale->pNext;
		}
		// メインリストから分離
		if(nSelect==0){
			pListTab->pRoot = GetPtrFromIndex(pListTab->pRoot, 0)->pNext;
		}else{
			GetPtrFromIndex(pListTab->pRoot, nSelect-1)->pNext = GetPtrFromIndex(pListTab->pRoot, nSelect)->pNext;
		}
		// 位置調整
		if(nSelect<nAfter){
			nAfter--;
		}
		// 表示削除
		ListView_DeleteItem(pListTab->hList, nSelect);
	}
	pSubTale->pNext = NULL;	// サブルートを閉じる

	// 分離したリストを結合
	InsertList(pListTab, nAfter, pSubRoot);
	ListView_EnsureVisible(pListTab->hList, nAfter, FALSE);

	// 再生中ファイルのインデックスを変更
	pListTab->nPlaying = GetIndexFromPtr(pListTab->pRoot, pPlaying);

	// 描画ロック解除
	LockWindowUpdate(NULL);
	return 0;
}

/*リストとリストビューアイテムを同時に削除*/
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

// ファイルリスト -> ファイルリスト
void SendToPlaylist(struct LISTTAB *ltFrom, struct LISTTAB *ltTo){
	int nIndex = -1;
	FILEINFO *pTmp;

	while((nIndex = ListView_GetNextItem(ltFrom->hList, nIndex, LVNI_SELECTED))!=-1){
		pTmp = GetPtrFromIndex(ltFrom->pRoot, nIndex);
		AddList(&(ltTo->pRoot), pTmp->szFilePath, pTmp->szSize, pTmp->szTime);
	}
	TraverseList(ltTo);
}

// リストタブリネーム
int RenameListTab(HWND hTab, int nItem, LPTSTR szLabel){
	TCITEM tci;

	lstrcpyn(GetListTab(hTab, nItem)->szTitle, szLabel, MAX_FITTLE_PATH);
	tci.mask = TCIF_TEXT;
	tci.cchTextMax = MAX_FITTLE_PATH;
	tci.pszText = szLabel;
	TabCtrl_SetItem(hTab, nItem, &tci);
	return 0;
}

// リストタブ削除
int RemoveListTab(HWND hTab, int nItem){
	RECT rcTab;
	DeleteAllList(&(GetListTab(hTab, nItem)->pRoot));
	DestroyWindow(GetListTab(hTab, nItem)->hList);
	HFree(GetListTab(hTab, nItem));
	TabCtrl_DeleteItem(hTab, nItem);
	// タブの行数が変わるとリストビューがずれるためそれを修正
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

// タブ移動
int MoveTab(HWND hTab, int nTarget, int nDir){
	TCITEM tciFrom, tciTo;
	TCHAR szFrom[MAX_FITTLE_PATH], szTo[MAX_FITTLE_PATH];

	// 初期設定
	tciFrom.mask = tciTo.mask = TCIF_PARAM | TCIF_TEXT;
	tciFrom.cchTextMax = tciTo.cchTextMax = MAX_FITTLE_PATH;
	tciFrom.pszText = szFrom;
	tciTo.pszText = szTo;

	// 情報取得
	TabCtrl_GetItem(hTab, nTarget, &tciFrom);
	TabCtrl_GetItem(hTab, nTarget + nDir, &tciTo);

	// 情報スワップ
	TabCtrl_SetItem(hTab, nTarget, &tciTo);
	TabCtrl_SetItem(hTab, nTarget + nDir, &tciFrom);

	// フォーカスの移動
	if(TabCtrl_GetCurFocus(hTab)==nTarget){
		TabCtrl_SetCurFocus(hTab, nTarget + nDir);
	}else if(TabCtrl_GetCurFocus(hTab)==nTarget + nDir){
		TabCtrl_SetCurFocus(hTab, nTarget);
	}
	//InvalidateRect(hTab, NULL, FALSE);
	
	return 0;
}

// 前の曲履歴を取得
struct FILEINFO *PopPrevious(struct LISTTAB *pListTab){
	if(pListTab->nStkPtr<0) return NULL;
	return pListTab->pPrevStk[pListTab->nStkPtr--];
}

// スタックポインタを移動させずに履歴を取得
struct FILEINFO *GetPlaying(struct LISTTAB *pListTab){
	if(pListTab->nStkPtr<0) return NULL;
	return pListTab->pPrevStk[pListTab->nStkPtr];
}

// 前の曲履歴をプッシュ
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

// スタックポインタを取得
int GetStackPtr(struct LISTTAB *pListTab){
	return pListTab->nStkPtr;
}

// リストをソート
void SortListTab(struct LISTTAB *pLT, int nSortType){
	struct FILEINFO *pPlaying;

	pPlaying = GetPtrFromIndex(pLT->pRoot, pLT->nPlaying); 
	pLT->nSortState = nSortType;
	MergeSort(&(pLT->pRoot), nSortType);
	TraverseList(pLT);
	pLT->nPlaying = GetIndexFromPtr(pLT->pRoot, pPlaying);
	return;
}	

// 既存のリストに新たなリストを挿入
int InsertList(struct LISTTAB *pTo, int nStart, struct FILEINFO *pSub){
	int nMainCount;
	int nSubCount;
	int i;

	if(!pSub) return 0;

	// インデックスの処理
	nMainCount = ListView_GetItemCount(pTo->hList);
	nSubCount = GetListCount(pSub);
	if(nStart<0) nStart = nMainCount;

	// リストビューが可視なら描画をロック
	/*if(IsWindowVisible(pTo->hList)){
		LockWindowUpdate(GetParent(GetParent(pTo->hList)));
		ShowWindow(pTo->hList, SW_HIDE);
		bVisible = TRUE;
	}*/

	// リストを結合
	GetPtrFromIndex(pSub, nSubCount-1)->pNext = GetPtrFromIndex(pTo->pRoot, nStart);
	if(nStart!=0){
		GetPtrFromIndex(pTo->pRoot, nStart-1)->pNext = pSub;
	}else{
		pTo->pRoot = pSub;
	}
	ListView_SetItemCountEx(pTo->hList, nMainCount + nSubCount, LVSICF_NOINVALIDATEALL);

	// 選択状態の設定
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

	// ファイルをオープン＆リード
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
		MessageBox(GetParent(hTab), TEXT("メモリの確保に失敗しました。"), TEXT("Fittle"), MB_OK);
		return FALSE;
	}
	ReadFile(hFile, szBuff, dwSize, &dwAccBytes, NULL);
	CloseHandle(hFile);
	szBuff[dwAccBytes / sizeof(TCHAR)] = TEXT('\0');

	// 読み込んだバッファを処理
	for(p=szBuff;*p!=TEXT('\0');){
#ifdef UNICODE
		if (*p == (WCHAR)0xfeff) p++;
#endif
		// タイトル読み込み
		for(i=0;*p!=TEXT('\n') && *p!=TEXT('\r');p++){
			szTempPath[i++] = *p;
		}
		szTempPath[i] = TEXT('\0');
		pNew = MakeNewTab(hTab, szTempPath, -1);
		pTale = &(pNew->pRoot);
		while (*p == TEXT('\n') || *p == TEXT('\r'))
			p++;

		// ファイル読み込み
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

// mkimg.dllのバグ回避のため全てのリストを破棄
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
		nBuffSize += ListView_GetItemCount(GetListTab(hTab, i)->hList) + 2; //タイトル分で+1
	}
#ifdef UNICODE
	nBuffSize = sizeof(TCHAR) + nBuffSize * (MAX_FITTLE_PATH + 2) * sizeof(TCHAR);
#else
	nBuffSize *= (MAX_FITTLE_PATH + 1) * sizeof(TCHAR);
#endif

	szBuff = (LPTSTR)HZAlloc(nBuffSize);
	if(!szBuff){
		MessageBox(GetParent(hTab), TEXT("メモリの確保に失敗しました。"), TEXT("Fittle"), MB_OK);
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
			nBuffPos = WriteBuff(szBuff, nBuffSize, nBuffPos, TEXT("\t"));	// ファイル識別子(\t)を挿入
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

	// ファイルの書き込み
	hFile = CreateFile(szColPath, GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE){
		HFree(szBuff);
		return FALSE;
	}
	WriteFile(hFile, szBuff, (DWORD)nBuffPos * sizeof(TCHAR), &dwAccBytes, NULL);
	CloseHandle(hFile);

	// メモリの開放
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
				case IDOK:	// 設定保存
					GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), szTemp, MAX_FITTLE_PATH);
					if(lstrlen(szTemp)==0) return TRUE;
					lstrcpyn(pszText, szTemp, MAX_FITTLE_PATH);
					EndDialog(hDlg, TRUE);
					return TRUE;

				case IDCANCEL:	// 設定を保存せずに終了
					EndDialog(hDlg, FALSE);
					return TRUE;
			}
			return FALSE;

		// 終了
		case WM_CLOSE:
			EndDialog(hDlg, FALSE);
			return FALSE;

		default:
			return FALSE;
	}
}

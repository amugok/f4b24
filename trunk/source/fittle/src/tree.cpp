/*
 * Tree.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	ツリービュー＆コンボボックスを操作
//

#include "fittle.h"
#include "tree.h"
#include "func.h"
#include "bass_tag.h"
#include "archive.h"
#include <assert.h>
#include "f4b24.h"

// ローカル関数
static BOOL CheckHaveChild(LPTSTR);
static int DeleteAllChildNode(HWND, HTREEITEM);

// アイコンインデックス
static int m_iFolderIcon = -1;
static int m_iListIcon = -1;

// マクロ
#define OnEndDragTree()	ReleaseCapture()

// 指定したコンボボックスに使用可能なドライブを列挙*/
int SetDrivesToCombo(HWND hCB){
	DWORD dwSize;
	LPTSTR szBuff;
	COMBOBOXEXITEM citem = {0};
	int i;

	citem.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	citem.cchTextMax = MAX_FITTLE_PATH;
	SendMessage(hCB, CB_RESETCONTENT, 0, 0);

	// ドライブ列挙
	dwSize = GetLogicalDriveStrings(0, NULL); // サイズを取得
	szBuff = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (dwSize + 1) * sizeof(TCHAR));
	if(!szBuff){
		MessageBox(GetParent(hCB), TEXT("コンボボックスの初期化に失敗しました。メモリを確保してください。"), TEXT("Fittle"), MB_OK);
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

	SendMessage(GetParent(hCB), WM_F4B24_IPC, (WPARAM)WM_F4B24_HOOK_UPDATE_DRIVELISTE, (LPARAM)hCB);

	SendMessage(hCB, CB_SETCURSEL, 
		(WPARAM)SendMessage(hCB, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)(LPTSTR)TEXT("C:\\")), 0);
	// アイコン取得
	if(g_cfg.nTreeIcon) RefreshComboIcon(hCB);
	return i;
}

// コンボボックスのアイコンをリフレッシュ
void RefreshComboIcon(HWND hCB){
	int nCnt;
	int i;
	SHFILEINFO shfi;
	COMBOBOXEXITEM cbi = {0};
	TCHAR szPath[MAX_FITTLE_PATH];

	cbi.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	cbi.cchTextMax = MAX_FITTLE_PATH;
	cbi.pszText = szPath;
	nCnt = SendMessage(hCB, CB_GETCOUNT, 0, 0);
	for(i=0;i<nCnt;i++){
		cbi.iItem = i;
		SendMessage(hCB, CBEM_GETITEM, (WPARAM)0, (LPARAM)&cbi);
		if(cbi.pszText[3]) {
			if (IsArchive(cbi.pszText)) {
				cbi.lParam = cbi.iImage = cbi.iSelectedImage = GetArchiveIconIndex(cbi.pszText);
				SendMessage(hCB, CBEM_SETITEM, (WPARAM)0, (LPARAM)&cbi);
			}
		} else {
			SHGetFileInfo(cbi.pszText, 0, &shfi, sizeof(shfi), 
				SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
			DestroyIcon(shfi.hIcon);
			cbi.lParam = cbi.iImage = cbi.iSelectedImage = shfi.iIcon;
			SendMessage(hCB, CBEM_SETITEM, (WPARAM)0, (LPARAM)&cbi);
		}
	}
	return;
}

// bShowがTRUEならアイコン表示、FALSEならアイコン非表示
int InitTreeIconIndex(HWND hCB, HWND hTV, BOOL bShow){
	TCHAR szIconPath[MAX_FITTLE_PATH];
	SHFILEINFO shfi;
	HIMAGELIST hSysImglst;

	// イメージリストをセット
	if(bShow){
		hSysImglst = (HIMAGELIST)SHGetFileInfo(TEXT(""), 0, &shfi,
			sizeof(shfi), SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
			ImageList_SetBkColor(hSysImglst, (COLORREF)g_cfg.nBkColor);
	}else{
		hSysImglst = NULL;
	}
	SendMessage(hCB, CBEM_SETIMAGELIST, 0, (LPARAM)hSysImglst);
	TreeView_SetImageList(hTV, hSysImglst, TVSIL_NORMAL);

	// フォルダアイコンインデックス取得
	GetModuleParentDir(szIconPath);
	SHGetFileInfo(szIconPath, 0, &shfi, sizeof(SHFILEINFO),
		SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
	DestroyIcon(shfi.hIcon);
	m_iFolderIcon = shfi.iIcon;

	// リストアイコンインデックス取得
	m_iListIcon = GetArchiveIconIndex(TEXT("*.m3u"));

	return 0;
}

// ドライブノード作成
HTREEITEM MakeDriveNode(HWND hCB, HWND hTV){
	int nNowCBIndex;
	TCHAR szNowDrive[MAX_FITTLE_PATH] = {0};
	TV_INSERTSTRUCT tvi = {0};
	HTREEITEM hDriveNode;

	//コンボボックスの選択文字列取得
	nNowCBIndex = (int)SendMessage(hCB, CB_GETCURSEL, 0, 0);
	SendMessage(hCB, CB_GETLBTEXT, (WPARAM)nNowCBIndex, (LPARAM)szNowDrive);
	//ツリービューにドライブ追加
	TreeView_DeleteAllItems(hTV);
	tvi.hInsertAfter = TVI_LAST;
	tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.hParent = TVI_ROOT;
	tvi.item.pszText = szNowDrive;
	tvi.item.iImage = tvi.item.iSelectedImage = 
		(int)SendMessage(hCB, CB_GETITEMDATA, (int)nNowCBIndex, 0);
	hDriveNode = TreeView_InsertItem(hTV, &tvi);
	// ダミーノードを追加
	tvi.hParent = hDriveNode;
	TreeView_InsertItem(hTV, &tvi);
	// 展開
	TreeView_Expand(hTV, hDriveNode, TVE_EXPAND);
	return hDriveNode;
}

// 指定TreeViewの指定ノードにツリーを二階層作る
HTREEITEM MakeTwoTree(HWND hTV,	HTREEITEM hTarNode){
	HANDLE hFind;
	HTREEITEM hChildNode = NULL;
	TV_INSERTSTRUCT tvi = {0};
	WIN32_FIND_DATA wfd;
	TCHAR szParentPath[MAX_FITTLE_PATH];  //親ディレクトリ(フルパス)
	TCHAR szParentPathW[MAX_FITTLE_PATH]; //親ディレクトリ("\*.*"を追加)
	TCHAR szTargetPath[MAX_FITTLE_PATH];  //検索されたディレクトリ(フルパス)

	DeleteAllChildNode(hTV, hTarNode);
	GetPathFromNode(hTV, hTarNode, szParentPath);

	if(szParentPath[3]==TEXT('\0')) szParentPath[2] = TEXT('\0');	// ドライブ対策
	wsprintf(szParentPathW, TEXT("%s\\*.*"), szParentPath);
	tvi.hInsertAfter = TVI_SORT;
	tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.hParent = hTarNode;
	tvi.item.pszText = (LPTSTR)wfd.cFileName;
	hFind = FindFirstFile(szParentPathW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			// ゴミ以外のディレクトリだったら追加
			if(wfd.cFileName[0]!=TEXT('\0') && lstrcmp((LPTSTR)wfd.cFileName, TEXT(".")) && lstrcmp((LPTSTR)wfd.cFileName, TEXT(".."))){
				if(g_cfg.nHideShow || !(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)){
					// パスの完成
					wsprintf(szTargetPath,TEXT("%s\\%s"), szParentPath, wfd.cFileName);
					if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
						//----ディレクトリの処理----
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

// 指定ノード以下にディレクトリが存在すればTRUEを返す
BOOL CheckHaveChild(LPTSTR szTargetPath){
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	TCHAR szNowDir[MAX_FITTLE_PATH];
	TCHAR szTargetPathW[MAX_FITTLE_PATH];

	if(g_cfg.nAllSub) return TRUE;	// AvestaOption
	wsprintf(szTargetPathW, TEXT("%s\\*.*"), szTargetPath);
	hFind = FindFirstFile(szTargetPathW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			wsprintf(szNowDir, TEXT("%s\\%s"), szTargetPath, (LPTSTR)wfd.cFileName);
			//ゴミ以外のディレクトリだったらTRUE
			if(lstrcmp((LPTSTR)wfd.cFileName, TEXT(".")) && lstrcmp((LPTSTR)wfd.cFileName, TEXT(".."))){
				if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || IsPlayList(szNowDir) || IsArchive(szNowDir)){
					FindClose(hFind);
					return TRUE;
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	return FALSE; //子ディレクトリがなければFALSE
}

// パスを元にツリーを作る　長すぎ…
HTREEITEM MakeTreeFromPath(HWND hTV, HWND hCB, LPTSTR szSetPath){
	int nDriveIndex;
	TCHAR szDriveName[MAX_FITTLE_PATH];
	TCHAR szCompPath[MAX_FITTLE_PATH];
	TCHAR szLabel[MAX_FITTLE_PATH];
	TCHAR szErrorMsg[300];
	HTREEITEM hDriveNode, hCompNode, hFoundNode;
	TVITEM ti;
	int i, j;
	HWND hwndWork;

#ifdef _DEBUG
	DWORD dTime;
	dTime = GetTickCount();
#endif

	// \を削除
	PathRemoveBackslash(szSetPath);

	// パスが有効かチェック
	if((!FILE_EXIST(szSetPath)) || (lstrlen(szSetPath)==0)){
		wsprintf(szErrorMsg, TEXT("%s\nパスが見つかりません。"), szSetPath);
		MessageBox(hTV, szErrorMsg, TEXT("Fittle"), MB_OK | MB_ICONEXCLAMATION);
		return NULL;
	}

	lstrcpyn(szDriveName, szSetPath, MAX_FITTLE_PATH);
	PathStripToRoot(szDriveName);
	hwndWork = CreateWindow(TEXT("STATIC"),TEXT(""),0,0,0,0,0,NULL,NULL,GetModuleHandle(NULL),NULL);
	if (hwndWork){
		SendMessage(hwndWork, WM_SETTEXT, (WPARAM)0, (LPARAM)szSetPath);
		if (SendMessage(GetParent(hTV), WM_F4B24_IPC, (WPARAM)WM_F4B24_HOOK_GET_TREE_ROOT, (LPARAM)hwndWork)){
			// ルートになるしおりが存在する
			SendMessage(hwndWork, WM_GETTEXT, (WPARAM)MAX_FITTLE_PATH, (LPARAM)szDriveName);
		}
		DestroyWindow(hwndWork);
	}


	// 描画高速化のため再描画を阻止
	LockWindowUpdate(GetParent(hTV));

	// szDriveNameにドライブを設定
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
	// 渡されたパスがドライブだったらドライブノードを返して終わり
	if(!lstrcmp(szSetPath, szDriveName)/*szSetPath[lstrlen(szDriveName)]==TEXT('\0')*/){
		TreeView_SelectItem(hTV, hDriveNode);
		// 再描画を許可
		LockWindowUpdate(NULL);
		return hDriveNode;
	}

	// 初期化
	ti.mask = TVIF_TEXT ;
	ti.cchTextMax = MAX_FITTLE_PATH;
	ti.pszText = szLabel;
	hFoundNode = hDriveNode;
	// ドライブ以降
	do{
		j=0;
		// フォルダ名を取得
		do{
			szCompPath[j] = szSetPath[i];
#ifdef UNICODE
#else
			if(IsDBCSLeadByte(szSetPath[i])){
				// ２バイト文字の処理
				i++;
				j++;
				szCompPath[j] = szSetPath[i];
			}
#endif
			i++;
			j++;
		}while(!(szSetPath[i]==TEXT('\0') || szSetPath[i]==TEXT('\\')));
		szCompPath[j] = TEXT('\0');

		// フォルダ名と一致するノードを展開
		hCompNode = TreeView_GetChild(hTV, hFoundNode);
		while(hCompNode){
			ti.hItem = hCompNode;
			TreeView_GetItem(hTV, &ti);
			if(!lstrcmpi(szCompPath, ti.pszText)){
				// 発見
				TreeView_Expand(hTV, ti.hItem, TVE_EXPAND);
				hFoundNode = ti.hItem;
				break;
			}else{
				// 次のノード
				hCompNode = TreeView_GetNextSibling(hTV, hCompNode);
			}
		}
		// 一致するフォルダがなかった場合（隠しフォルダなど）は追加
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
			TreeView_InsertItem(hTV, &tvi);	// ダミーを追加
			TreeView_Expand(hTV, hFoundNode, TVE_EXPAND);
		}
	}while(szSetPath[i++]!=TEXT('\0'));
	TreeView_SelectItem(hTV, hFoundNode);

#ifdef _DEBUG
	TCHAR szBuff[100];
	wsprintf(szBuff, TEXT("Construct tree time: %d ms\n"), GetTickCount() - dTime);
	OutputDebugString(szBuff);
#endif

	// 再描画を許可
	LockWindowUpdate(NULL);

	return hFoundNode;
}

// 指定したツリービューの選択ノードの全子ノードを削除
int DeleteAllChildNode(HWND hTV, HTREEITEM hTarNode){
	HTREEITEM hDelNode;
	HTREEITEM hNextNode;

	for(hDelNode = TreeView_GetChild(hTV, hTarNode);hDelNode;hDelNode = hNextNode){
		hNextNode = TreeView_GetNextSibling(hTV, hDelNode);
		TreeView_DeleteItem(hTV, hDelNode);
	}
	return 0;
}

// 指定ツリービューの指定ノードからフルパスを取得
int GetPathFromNode(HWND hTV, HTREEITEM hTargetNode, LPTSTR pszBuff){
	TVITEM ti;
	HTREEITEM hParentNode;
	TCHAR szUnderPath[MAX_FITTLE_PATH];		// 選択ノード以下のパスを保存
	TCHAR szNowNodeLabel[MAX_FITTLE_PATH];	// 選択ノードのラベル

	if(!hTargetNode){
		lstrcpy(pszBuff, TEXT(""));
		return 0;
	}
	// 初期設定
	szUnderPath[0] = TEXT('\0');
	ti.mask = TVIF_TEXT;
	ti.hItem = hTargetNode;
	ti.pszText = szNowNodeLabel;
	ti.cchTextMax = MAX_FITTLE_PATH;

	// 現在のノード情報を取得
	if (!TreeView_GetItem(hTV, &ti)){
		return 0;
	}
	lstrcpyn(szNowNodeLabel, ti.pszText, MAX_FITTLE_PATH);

	while(!(ti.pszText[1]==TEXT(':') && ti.pszText[2]==TEXT('\\')) && !(ti.pszText[0]==TEXT('\\') && ti.pszText[1]==TEXT('\\'))){
		wsprintf(szNowNodeLabel, TEXT("%s\\"), ti.pszText);
		lstrcat(szNowNodeLabel, szUnderPath);
		lstrcpyn(szUnderPath, szNowNodeLabel, MAX_FITTLE_PATH);
		// 親ノードからラベルを取得
		hParentNode = TreeView_GetParent(hTV, ti.hItem);
		ti.hItem = hParentNode;
		if (!TreeView_GetItem(hTV, &ti)){
			return 0;
		}
	}

	lstrcat(szNowNodeLabel, szUnderPath);
	PathRemoveBackslash(szNowNodeLabel);
	lstrcpyn(pszBuff, szNowNodeLabel, MAX_FITTLE_PATH);
	return 0;
}

//ツリーの表示位置を調整
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

// コンボボックスのプロシージャ
LRESULT CALLBACK NewComboProc(HWND hCB, UINT msg, WPARAM wp, LPARAM lp){
	static HBRUSH hBrush = NULL;
	int crlBk;

	switch(msg){
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
			if(hBrush)	DeleteObject(hBrush);	// 前に使ってたものを削除

			SetTextColor((HDC)wp, (COLORREF)g_cfg.nTextColor);
			SetBkColor((HDC)wp, (COLORREF)g_cfg.nBkColor);
			crlBk = GetBkColor((HDC)wp);
			hBrush = (HBRUSH)CreateSolidBrush((COLORREF)crlBk);
			return (LRESULT)hBrush;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hCB, GWLP_USERDATA), hCB, msg, wp, lp);
}

// エディットボックスのプロシージャ
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

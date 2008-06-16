/*
 * bookmark
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "bookmark.rh"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

// ウィンドウをサブクラス化、プロシージャハンドルをウィンドウに関連付ける
#define SET_SUBCLASS(hWnd, Proc) \
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Proc))

static void LoadConfig();
static BOOL OnInit();
static void OnQuit();
static void OnTrackChange();
static void OnStatusChange();
static void OnConfig();

static BOOL CALLBACK BookMarkDlgProc(HWND, UINT, WPARAM, LPARAM);
static void DrawBookMark(HMENU);
static int AddBookMark(HMENU, HWND);

static HMODULE m_hinstDLL = 0;
static WNDPROC m_hOldProc = 0;

static FITTLE_PLUGIN_INFO fpi = {
	PDK_VER,
	OnInit,
	OnQuit,
	OnTrackChange,
	OnStatusChange,
	OnConfig,
	NULL,
	NULL
};

void GetModuleParentDir(LPTSTR szParPath){
	TCHAR szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98以降
	*PathFindFileName(szParPath) = '\0';
	return;
}

// 設定を読込
static void LoadConfig(){
	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("fittle.ini"));

//	nTagReverse = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("TagReverse"), 0, m_szINIPath);
}

//Int型で設定ファイル書き込み
static int WritePrivateProfileInt(LPTSTR szAppName, LPTSTR szKeyName, int nData, LPTSTR szINIPath){
	TCHAR szTemp[100];

	wsprintf(szTemp, TEXT("%d"), nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}

// 終了状態を保存
static void SaveState(){
	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("fittle.ini"));


//	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("x"), nMiniPanel_x, m_szINIPath);
	WritePrivateProfileString(NULL, NULL, NULL, m_szINIPath);

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hinstDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static LRESULT FittlePluginInterface(int nCommand){
	return SendMessage(fpi.hParent, WM_FITTLE, nCommand, 0);
}

static int IsCheckedMainMenu(int nCommand){
	HMENU hMainMenu = (HMENU)FittlePluginInterface(GET_MENU);
	return (GetMenuState(hMainMenu, nCommand, MF_BYCOMMAND) & MF_CHECKED) != 0;
}

static void SendCommandMessage(HWND hwnd, int nCommand){
	if (hwnd && IsWindow(hwnd))
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(nCommand, 0), 0);
}

static void SendCommand(int nCommand){
	SendCommandMessage(fpi.hParent, nCommand);
}

// サブクラス化したウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_SYSCOMMAND:
	case WM_COMMAND:
		switch (LOWORD(wp)){
		case IDM_BM_ADD: //しおりに追加
			if(AddBookMark(GetSubMenu(m_hMainMenu, GetMenuPosFromString(m_hMainMenu, TEXT("しおり(&B)"))), hWnd)!=-1){
//				SetDrivesToCombo(m_hCombo);
				SendMessage(hWnd, WM_DEVICECHANGE, 0x8000 ,0);
			}
			break;

		case IDM_BM_ORG:
			DialogBox(m_hInst, TEXT("BOOKMARK_DIALOG"), hWnd, (DLGPROC)BookMarkDlgProc);
			DrawBookMark(GetSubMenu(m_hMainMenu, GetMenuPosFromString(m_hMainMenu, TEXT("しおり(&B)"))));
			break;
		}
	case WM_FITTLE:
		if (wp == GET_MINIPANEL)		{
			return (m_hMiniPanel && IsWindow(m_hMiniPanel)) ? (LRESULT)m_hMiniPanel : (LRESULT)0;
		}
		break;
	}
	return CallWindowProc(m_hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	m_hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);

	LoadState();

	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
static void OnQuit(){
	SaveState();
}

/* 曲が変わる時に呼ばれます */
static void OnTrackChange(){
}

/* 再生状態が変わる時に呼ばれます */
static void OnStatusChange(){
}

/* 設定画面を呼び出します（未実装）*/
static void OnConfig(){
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

// しおりをメニューに描画
static void DrawBookMark(HMENU hBMMenu){
	int i;
	TCHAR szMenuBuff[MAX_FITTLE_PATH+4];

	// 今あるメニューを削除
	for(i=0;i<MAX_BM_SIZE;i++){
		if(!DeleteMenu(hBMMenu, IDM_BM_FIRST+i, MF_BYCOMMAND)) break;
	}

	// 新しくメニューを追加
	for(i=0;i<MAX_BM_SIZE;i++){
		TCHAR szBMPath[MAX_FITTLE_PATH];
		lstrcpyn(szBMPath, GetBookmark(i), MAX_FITTLE_PATH);
		if(g_cfg.nBMFullPath){
			wsprintf(szMenuBuff, TEXT("&%d: %s"), i, szBMPath);
		}else{
			wsprintf(szMenuBuff, TEXT("&%d: %s"), i, PathFindFileName(szBMPath));
		}
		if(!szBMPath[0]) break;
		AppendMenu(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuff);
	}
	return;
}

// しおりに追加（あとで修正）
static int AddBookMark(HMENU hBMMenu, HWND hWnd){
	//HTREEITEM hNode;
	TCHAR szMenuBuff[MAX_FITTLE_PATH+4];
	
	/*
	hNode = TreeView_GetSelection(m_hTree);
	if(hNode==NULL || hNode==TVI_ROOT) return -1;
	GetPathFromNode(m_hTree, hNode, szMenuBuff);
	*/

	if (*GetBookmark(MAX_BM_SIZE-1)){
		MessageBox(hWnd, TEXT("しおりの個数制限を越えました。"), TEXT("Fittle"), MB_OK | MB_ICONEXCLAMATION);
		return -1;
	}
	int i = AppendBookmark(m_szTreePath);
	if (i < 0){
		return -1;
	}
	
	if(g_cfg.nBMFullPath){
		wsprintf(szMenuBuff, TEXT("&%d: %s"), i, m_szTreePath);
	}else{
		wsprintf(szMenuBuff, TEXT("&%d: %s"), i, PathFindFileName(m_szTreePath));
	}
	AppendMenu(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuff);
	return i;
}


/*  既に実行中のFittleにパラメータを送信する */
static void SendCopyData(HWND hFittle, int iType, LPTSTR lpszString){
#ifdef UNICODE
	CHAR nameA[MAX_FITTLE_PATH];
	LPBYTE lpWork;
	COPYDATASTRUCT cds;
	DWORD la, lw, cbData;
	WideCharToMultiByte(CP_ACP, 0, lpszString, -1, nameA, MAX_FITTLE_PATH, NULL, NULL);
	la = lstrlenA(nameA) + 1;
	if (la & 1) la++; /* WORD align */
	lw = lstrlenW(lpszString) + 1;
	cbData = la * sizeof(CHAR) + lw * sizeof(WCHAR);
	lpWork = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbData);
	if (!lpWork) return;
	lstrcpyA((LPSTR)(lpWork), nameA);
	lstrcpyW((LPWSTR)(lpWork + la), lpszString);
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpWork;
	cds.cbData = cbData;
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
	HeapFree(GetProcessHeap(), 0, lpWork);
#else
	COPYDATASTRUCT cds;
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpszString;
	cds.cbData = (lstrlenA(lpszString) + 1) * sizeof(CHAR);
	// 文字列送信
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
#endif
}

static BOOL CALLBACK BookMarkDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM /*lp*/){
	static HWND hList = NULL;
	int i;
	LVITEM item;

	switch (msg)
	{
		case WM_INITDIALOG:	// 初期化
			hList = GetDlgItem(hDlg, IDC_LIST1);

			LVCOLUMN lvcol;
			ZeroMemory(&lvcol, sizeof(LVCOLUMN));
			lvcol.mask = LVCF_FMT;
			lvcol.fmt = LVCFMT_LEFT;
			ListView_InsertColumn(hList, 0, &lvcol);

			ZeroMemory(&item, sizeof(LVITEM));
			item.mask = LVIF_TEXT;
			item.iSubItem = 0;
			int nMax;
			nMax = 300;

			for(i=0;i<MAX_BM_SIZE;i++){
				TCHAR szBMPath[MAX_FITTLE_PATH];
				lstrcpyn(szBMPath, GetBookmark(i), MAX_FITTLE_PATH);
				if(!szBMPath[0]) break;
				item.pszText = szBMPath;
				nMax = ListView_GetStringWidth(hList, item.pszText)+10>nMax?ListView_GetStringWidth(hList, item.pszText)+10:nMax;
				item.iItem = i;
				ListView_InsertItem(hList, &item);
			}
			ListView_SetColumnWidth(hList, 0, nMax);
			ListView_SetItemState(hList, 0, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
			SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)(g_cfg.nBMRoot?BST_CHECKED:BST_UNCHECKED), 0);
			SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, (WPARAM)(g_cfg.nBMFullPath?BST_CHECKED:BST_UNCHECKED), 0);
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDOK:
					int nCount;

					ClearBookmark();
					nCount = ListView_GetItemCount(hList);
					for(i=0;i<nCount;i++){
						TCHAR szBuf[MAX_FITTLE_PATH];
						ListView_GetItemText(hList, i, 0, szBuf, MAX_FITTLE_PATH);
						AppendBookmark(szBuf);
					}
					g_cfg.nBMRoot = SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
					g_cfg.nBMFullPath = SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);
					EndDialog(hDlg, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, -1);
					return TRUE;

				case IDC_BUTTON1:	// 上へ
					int nSel;
					TCHAR szPath[MAX_FITTLE_PATH], szSub[MAX_FITTLE_PATH];

					nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
					if(nSel<=0) return FALSE;
					ListView_GetItemText(hList, nSel-1, 0, szSub, MAX_FITTLE_PATH);
					ListView_GetItemText(hList, nSel, 0, szPath, MAX_FITTLE_PATH);
					ListView_SetItemText(hList, nSel-1, 0, szPath);
					ListView_SetItemText(hList, nSel, 0, szSub);
					ListView_SetItemState(hList, nSel-1, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
					return TRUE;

				case IDC_BUTTON2:	// 下へ
					nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
					if(nSel+1==ListView_GetItemCount(hList)) return FALSE;
					ListView_GetItemText(hList, nSel+1, 0, szSub, MAX_FITTLE_PATH);
					ListView_GetItemText(hList, nSel, 0, szPath, MAX_FITTLE_PATH);
					ListView_SetItemText(hList, nSel+1, 0, szPath);
					ListView_SetItemText(hList, nSel, 0, szSub);
					ListView_SetItemState(hList, nSel+1, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
					return TRUE;

				case IDC_BUTTON3:
					nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
					if(nSel<0) return FALSE;
					ListView_DeleteItem(hList, nSel);
					return TRUE;

			}
			return FALSE;

		
		case WM_CLOSE:	//終了
			EndDialog(hDlg, -1);
			return TRUE;

		default:
			return FALSE;
	}
}

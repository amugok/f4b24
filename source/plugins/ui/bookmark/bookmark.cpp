/*
 * bookmark
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/gplugin.h"
#include "../../../fittle/src/f4b24.h"
#include "bookmark.rh"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(linker, "/EXPORT:GetGPluginInfo=_GetGPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

typedef struct StringList {
	struct StringList *pNext;
	WASTR szString;
} STRING_LIST, *LPSTRING_LIST;


static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg);
static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode);

#include "../../../fittle/src/wastr.cpp"

static GENERAL_PLUGIN_INFO gpinfo = {
	GPDK_VER,
	HookWndProc,
	OnEvent
};

#ifdef __cplusplus
extern "C"
#endif
GENERAL_PLUGIN_INFO * CALLBACK GetGPluginInfo(void){
	return &gpinfo;
}

static BOOL CALLBACK BookMarkDlgProc(HWND, UINT, WPARAM, LPARAM);
static void DrawBookMark(HMENU);
static int AddBookMark(HMENU, HWND);
static void LoadBookMark(HMENU);
static void SaveBookMark();

static void HookComboUpdate(HWND);
static BOOL HookTreeRoot(HWND);

static HMODULE m_hinstDLL = 0;
static BOOL m_bIsUnicode = FALSE;

#define MAX_BM_SIZE 100			// しおりの数

static struct {
	LPSTRING_LIST lpBookmark;
	int nBMRoot;				// しおりをルートとして扱うか
	int nBMFullPath;			// しおりをフルパスで表示
} g_cfgbm = {
	NULL
};

static void StringListFree(LPSTRING_LIST *pList){
	LPSTRING_LIST pCur = *pList;
	while (pCur){
		LPSTRING_LIST pNext = pCur->pNext;
		HeapFree(GetProcessHeap(), 0, pCur);
		pCur = pNext;
	}
	*pList = NULL;
}

static  LPSTRING_LIST StringListWalk(LPSTRING_LIST *pList, int nIndex){
	LPSTRING_LIST pCur = *pList;
	int i = 0;
	while (pCur){
		if (i++ == nIndex) return pCur;
		pCur = pCur->pNext;
	}
	return NULL;
}

static int StringListAdd(LPSTRING_LIST *pList, LPCWASTR pszValue){
	int i = 0;
	LPSTRING_LIST pCur = *pList;
	LPSTRING_LIST pNew = (LPSTRING_LIST)HeapAlloc(GetProcessHeap(), 0, sizeof(STRING_LIST));
	if (!pNew) return -1;
	pNew->pNext = NULL;
	WAstrcpy(&pNew->szString, pszValue);
	if (pCur){
		i++;
		/* 末尾に追加 */
		while (pCur->pNext){
			pCur = pCur->pNext;
			i++;
		}
		pCur->pNext = pNew;
	} else {
		/* 先頭 */
		*pList = pNew;
	}
	return i;
}

static void ClearBookmark(void){
	StringListFree(&g_cfgbm.lpBookmark);
}

LPCWASTR GetBookmark(int nIndex){
	LPSTRING_LIST lpbm = StringListWalk(&g_cfgbm.lpBookmark, nIndex);
	return lpbm ? &lpbm->szString : 0;
}

static int AppendBookmark(LPCWASTR pszPath){
	if (!pszPath || !WAstrlen(pszPath)) return TRUE;
	return StringListAdd(&g_cfgbm.lpBookmark, pszPath);
}

static void UpdateDriveList(HWND hWnd){
	SendMessage(hWnd, WM_DEVICECHANGE, 0x8000 ,0);
}

// 指定した文字列を持つメニュー項目を探す
static int GetMenuPosFromString(HMENU hMenu, LPTSTR lpszText){
	int i;
	TCHAR szBuf[64];
	int c = GetMenuItemCount(hMenu);
	for (i = 0; i < c; i++){
		GetMenuString(hMenu, i, szBuf, 64, MF_BYPOSITION);
		if (lstrcmp(lpszText, szBuf) == 0)
			return i;
	}
	return 0;
}

// 設定を読込
static void LoadState(HWND hWnd){
	WASetIniFile(NULL, "Fittle.ini");

	// しおりをルートとして扱う
	g_cfgbm.nBMRoot = WAGetIniInt("BookMark", "BMRoot", 1);
	// しおりをフルパスで表示
	g_cfgbm.nBMFullPath = WAGetIniInt("BookMark", "BMFullPath", 1);

	// しおりの読み込み
	LoadBookMark(GetSubMenu(GetMenu(hWnd), GetMenuPosFromString(GetMenu(hWnd), TEXT("しおり(&B)"))));
}

// 終了状態を保存
static void SaveState(){
	WASetIniFile(NULL, "Fittle.ini");

	WASetIniInt("BookMark", "BMRoot", g_cfgbm.nBMRoot);
	WASetIniInt("BookMark", "BMFullPath", g_cfgbm.nBMFullPath);

	SaveBookMark();	// しおりの保存

	WAFlushIni();

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

static HWND CreateWorkWindow(void){
	return CreateWindow(TEXT("STATIC"),TEXT(""),0,0,0,0,0,NULL,NULL,m_hinstDLL,NULL);
}

static void GetCurPath(HWND hWnd, LPWASTR lpszBuf) {
	HWND hwndWork = CreateWorkWindow();
	if (hwndWork){
		SendMessage(hWnd, WM_F4B24_IPC, (WPARAM)WM_F4B24_IPC_GET_CURPATH, (LPARAM)hwndWork);
		WAGetWindowText(hwndWork, lpszBuf);
		DestroyWindow(hwndWork);
	}
}

static void SetCurPath(HWND hWnd, LPCWASTR lpszPath) {
	HWND hwndWork = CreateWorkWindow();
	if (hwndWork){
		WASetWindowText(hwndWork, lpszPath);
		SendMessage(hWnd, WM_F4B24_IPC, (WPARAM)WM_F4B24_IPC_SET_CURPATH, (LPARAM)hwndWork);
		DestroyWindow(hwndWork);
	}
}

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg) {
	switch(pMsg->msg){
	case WM_SYSCOMMAND:
	case WM_COMMAND:
		if(IDM_BM_FIRST<=LOWORD(pMsg->wp) && LOWORD(pMsg->wp)<IDM_BM_FIRST+MAX_BM_SIZE){
			LPCWASTR lpszBMPath = GetBookmark(LOWORD(pMsg->wp)-IDM_BM_FIRST);
			if(!lpszBMPath || !WAstrlen(lpszBMPath)) break;
			SetCurPath(pMsg->hWnd, lpszBMPath);
			break;
		}
		switch (LOWORD(pMsg->wp)){
		case IDM_BM_ADD: //しおりに追加
			if(AddBookMark(GetSubMenu(GetMenu(pMsg->hWnd), GetMenuPosFromString(GetMenu(pMsg->hWnd), TEXT("しおり(&B)"))), pMsg->hWnd)!=-1){
				UpdateDriveList(pMsg->hWnd);
			}
			break;

		case IDM_BM_ORG:
			DialogBox(m_hinstDLL, TEXT("BOOKMARK_DIALOG"), pMsg->hWnd, (DLGPROC)BookMarkDlgProc);
			DrawBookMark(GetSubMenu(GetMenu(pMsg->hWnd), GetMenuPosFromString(GetMenu(pMsg->hWnd), TEXT("しおり(&B)"))));
			UpdateDriveList(pMsg->hWnd);
			break;
		}
		break;
	case WM_F4B24_IPC:
		switch (pMsg->wp){
			case WM_F4B24_HOOK_UPDATE_DRIVELISTE:
				HookComboUpdate((HWND)pMsg->lp);
				break;
			case WM_F4B24_HOOK_GET_TREE_ROOT:
			if (HookTreeRoot((HWND)pMsg->lp)) {
				pMsg->lMsgResult = TRUE;
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}


static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode) {
	if (eCode == GENERAL_PLUGIN_EVENT_INIT) {
		m_bIsUnicode = WAIsUnicode();
		LoadState(hWnd);

		EnableMenuItem(GetMenu(hWnd), IDM_BM_ADD, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(GetMenu(hWnd), IDM_BM_ORG, MF_BYCOMMAND | MF_ENABLED);

	} else if (eCode == GENERAL_PLUGIN_EVENT_INIT2) {
		UpdateDriveList(hWnd);
	} else if (eCode == GENERAL_PLUGIN_EVENT_QUIT) {
		SaveState();
		ClearBookmark();
	}
	return TRUE;
}

static void AddBMMenu(HMENU hBMMenu, int i, LPCWASTR lpszBMPath){
	WASTR szMenuBuf;
	CHAR buf[16];
	wsprintfA(buf, "&%d: ", i);
	WAstrcpyA(&szMenuBuf, buf);
	if (g_cfgbm.nBMFullPath) {
		WAstrcat(&szMenuBuf, lpszBMPath);
	} else {
		WASTR szFileName;
		WAGetFileName(&szFileName, lpszBMPath);
		WAstrcat(&szMenuBuf, &szFileName);
	}
	if (m_bIsUnicode)
		AppendMenuW(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuf.W);
	else
		AppendMenuA(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuf.A);
}

// しおりをメニューに描画
static void DrawBookMark(HMENU hBMMenu){
	int i;

	// 今あるメニューを削除
	for(i=0;i<MAX_BM_SIZE;i++){
		if(!DeleteMenu(hBMMenu, IDM_BM_FIRST+i, MF_BYCOMMAND)) break;
	}

	// 新しくメニューを追加
	for(i=0;i<MAX_BM_SIZE;i++){
		LPCWASTR lpszBMPath = GetBookmark(i);
		if(!lpszBMPath || !WAstrlen(lpszBMPath)) break;
		AddBMMenu(hBMMenu, i, lpszBMPath);
	}
	return;
}

// しおりに追加（あとで修正）
static int AddBookMark(HMENU hBMMenu, HWND hWnd){
	WASTR szBuf;
	int i;

	if (GetBookmark(MAX_BM_SIZE - 1)){
		MessageBox(hWnd, TEXT("しおりの個数制限を越えました。"), TEXT("Fittle"), MB_OK | MB_ICONEXCLAMATION);
		return -1;
	}

	WAstrcpyA(&szBuf, "");
	GetCurPath(hWnd, &szBuf);
	if (!WAstrlen(&szBuf)) return -1;

	i = AppendBookmark(&szBuf);
	if (i < 0){
		return -1;
	}
	AddBMMenu(hBMMenu, i, &szBuf);

	return i;
}

static int LV_GetWidth(HWND hList, LPCWASTR lpszString){
	return (m_bIsUnicode) ?
		SendMessageW(hList, LVM_GETSTRINGWIDTHW, 0, (LPARAM)lpszString->W) :
		SendMessageA(hList, LVM_GETSTRINGWIDTHA, 0, (LPARAM)lpszString->A);
}

static LV_InsertItem(HWND hList, int nPos, LPCWASTR lpszString){
	union {
		LVITEMA A;
		LVITEMW W;
	} item;
	if (m_bIsUnicode){
		item.W.mask = LVIF_TEXT;
		item.W.iSubItem = 0;
		item.W.pszText = (LPWSTR)lpszString->W;
		item.W.iItem = nPos;
		SendMessageW(hList, LVM_INSERTITEMW, 0, (LPARAM)&item.W);
	}else{
		item.A.mask = LVIF_TEXT;
		item.A.iSubItem = 0;
		item.A.pszText = (LPSTR)lpszString->A;
		item.A.iItem = nPos;
		SendMessageA(hList, LVM_INSERTITEMA, 0, (LPARAM)&item.A);
	}
}

static LV_GetItemText(HWND hList, int nPos, LPWASTR lpszBuf){
	union {
		LVITEMA A;
		LVITEMW W;
	} item;
	if (m_bIsUnicode){
		item.W.iSubItem = 0;
		item.W.pszText = lpszBuf->W;
		item.W.cchTextMax = WA_MAX_SIZE;
		SendMessageW(hList, LVM_GETITEMTEXTW, nPos, (LPARAM)&item.W);
	}else{
		item.A.iSubItem = 0;
		item.A.pszText = lpszBuf->A;
		item.A.cchTextMax = WA_MAX_SIZE;
		SendMessageA(hList, LVM_GETITEMTEXTA, nPos, (LPARAM)&item.A);
	}
}

static LV_SetItemText(HWND hList, int nPos, LPCWASTR lpszBuf){
	union {
		LVITEMA A;
		LVITEMW W;
	} item;
	if (m_bIsUnicode){
		item.W.iSubItem = 0;
		item.W.pszText = (LPWSTR)lpszBuf->W;
		SendMessageW(hList, LVM_SETITEMTEXTW, nPos, (LPARAM)&item.W);
	}else{
		item.A.iSubItem = 0;
		item.A.pszText = (LPSTR)lpszBuf->A;
		SendMessageA(hList, LVM_SETITEMTEXTA, nPos, (LPARAM)&item.A);
	}
}

static BOOL CALLBACK BookMarkDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM /*lp*/){
	static HWND hList = NULL;

	switch (msg)
	{
		case WM_INITDIALOG:	// 初期化
			{
				int i;
				int nMax;
				LVCOLUMN lvcol;
				hList = GetDlgItem(hDlg, IDC_LIST1);

				ZeroMemory(&lvcol, sizeof(LVCOLUMN));
				lvcol.mask = LVCF_FMT;
				lvcol.fmt = LVCFMT_LEFT;
				ListView_InsertColumn(hList, 0, &lvcol);

				nMax = 300;

				for(i=0;i<MAX_BM_SIZE;i++){
					LPCWASTR lpszBMPath = GetBookmark(i);
					if(!lpszBMPath || !WAstrlen(lpszBMPath)) break;

					nMax = LV_GetWidth(hList, lpszBMPath) + 10 > nMax ? LV_GetWidth(hList, lpszBMPath) + 10 : nMax;
					LV_InsertItem(hList, i, lpszBMPath);
				}
				ListView_SetColumnWidth(hList, 0, nMax);
				ListView_SetItemState(hList, 0, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
				SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)(g_cfgbm.nBMRoot?BST_CHECKED:BST_UNCHECKED), 0);
				SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, (WPARAM)(g_cfgbm.nBMFullPath?BST_CHECKED:BST_UNCHECKED), 0);
			}
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDOK:
					{
						int i;
						int nCount;

						ClearBookmark();
						nCount = ListView_GetItemCount(hList);
						for(i=0;i<nCount;i++){
							WASTR szBuf;
							LV_GetItemText(hList, i, &szBuf);
							AppendBookmark(&szBuf);
						}
						g_cfgbm.nBMRoot = SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
						g_cfgbm.nBMFullPath = SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);
						EndDialog(hDlg, 1);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, -1);
					return TRUE;

				case IDC_BUTTON1:	// 上へ
					{
						WASTR szPath, szSub;
						int nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
						if(nSel<=0) return FALSE;
						LV_GetItemText(hList, nSel-1, &szSub);
						LV_GetItemText(hList, nSel, &szPath);
						LV_SetItemText(hList, nSel-1, &szPath);
						LV_SetItemText(hList, nSel, &szSub);
						ListView_SetItemState(hList, nSel-1, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
					}
					return TRUE;

				case IDC_BUTTON2:	// 下へ
					{
						WASTR szPath, szSub;
						int nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
						if(nSel+1==ListView_GetItemCount(hList)) return FALSE;
						LV_GetItemText(hList, nSel+1, &szSub);
						LV_GetItemText(hList, nSel, &szPath);
						LV_SetItemText(hList, nSel+1, &szPath);
						LV_SetItemText(hList, nSel, &szSub);
						ListView_SetItemState(hList, nSel+1, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
					}
					return TRUE;

				case IDC_BUTTON3:
					{
						int nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
						if(nSel<0) return FALSE;
						ListView_DeleteItem(hList, nSel);
					}
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

// しおりを読み込む
static void LoadBookMark(HMENU hBMMenu){
	int i;
	CHAR szSec[10];
	WASTR szItem;

	ClearBookmark();
	for(i=0;i<MAX_BM_SIZE;i++){
		int j;
		wsprintfA(szSec, "Path%d", i);
		WAGetIniStr("BookMark", szSec, &szItem);
		if(!WAstrlen(&szItem)) break;
		j =  AppendBookmark(&szItem);
		if (j >= 0){
			AddBMMenu(hBMMenu, i, &szItem);
		}
	}
	return;
}

// しおりを保存
static void SaveBookMark(){
	int i;
	CHAR szSec[10];

	for(i=0;i<MAX_BM_SIZE;i++){
		LPCWASTR lpszBMPath = GetBookmark(i);
		wsprintfA(szSec, "Path%d", i);
		if (!lpszBMPath || !WAstrlen(lpszBMPath)){
			WASetIniStr("BookMark", szSec, NULL);
			break;
		}
		WASetIniStr("BookMark", szSec, lpszBMPath);
	}
	return;
}

static void HookComboUpdate(HWND hCB){
	int i,j;
	WASTR szDrawBuff;
	union {
		COMBOBOXEXITEMA A;
		COMBOBOXEXITEMW W;
	} citem;

	// しおりフォルダ列挙
	i = SendMessage(hCB, CB_GETCOUNT, 0, 0);
	for(j=0;j<MAX_BM_SIZE;j++){
		LPCWASTR lpszBMPath = GetBookmark(j);
		if(!lpszBMPath || !WAstrlen(lpszBMPath)) break;
		if(WAIsUNCPath(lpszBMPath) || WAIsDirectory(lpszBMPath)){
			WAstrcpy(&szDrawBuff, lpszBMPath);
			WAAddBackslash(&szDrawBuff);
		} else {
			WAstrcpy(&szDrawBuff, lpszBMPath);
		}
		if (m_bIsUnicode){
			citem.W.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
			citem.W.cchTextMax = WA_MAX_SIZE;
			citem.W.iItem = i;
			citem.W.lParam = citem.W.iImage = citem.W.iSelectedImage = -1;
			citem.W.pszText = szDrawBuff.W;
			SendMessageW(hCB, CBEM_INSERTITEMW, 0, (LPARAM)&citem.W);
		} else {
			citem.A.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
			citem.A.cchTextMax = WA_MAX_SIZE;
			citem.A.iItem = i;
			citem.A.lParam = citem.A.iImage = citem.A.iSelectedImage = -1;
			citem.A.pszText = szDrawBuff.A;
			SendMessageA(hCB, CBEM_INSERTITEMW, 0, (LPARAM)&citem.A);
		}
		i++;
	}
}


static BOOL IsSeparator(LPCWASTR lpszpath, int nPos){
	if (m_bIsUnicode)
		return lpszpath->W[nPos] == L'\\' || lpszpath->W[nPos] == L'\0';
	else
		return lpszpath->A[nPos] == '\\' || lpszpath->A[nPos] == '\0';
}

static BOOL HookTreeRoot(HWND hwndWork){
	int i;
	WASTR szSetPath;
	WASTR szTempRoot;
	
	WAstrcpyA(&szTempRoot, "");
	WAGetWindowText(hwndWork, &szSetPath);

	// ドライブボックスの変更
	if(g_cfgbm.nBMRoot){
		for(i=0;i<MAX_BM_SIZE;i++){
			LPCWASTR lpszBMPath = GetBookmark(i);
			if(!lpszBMPath || !WAstrlen(lpszBMPath)) break;

			if(WAStrStrI(&szSetPath, lpszBMPath) == 1){
				int len = WAstrlen(lpszBMPath);
				if(len > WAstrlen(&szTempRoot) && IsSeparator(&szSetPath, len)){
					WAstrcpy(&szTempRoot, lpszBMPath);
				}
			}
		}
	}
	if (!WAstrlen(&szTempRoot)) return FALSE;

	WASetWindowText(hwndWork, &szTempRoot);	
	return TRUE;
}

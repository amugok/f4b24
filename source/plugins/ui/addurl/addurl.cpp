/*
 * addurl
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include <windows.h>
#include <shlwapi.h>
#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/gplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(linker, "/EXPORT:GetGPluginInfo=_GetGPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#ifndef MAX_FITTLE_PATH
#define MAX_FITTLE_PATH 260*2
#endif

#define FILE_EXISTA(X) (GetFileAttributesA(X)==0xFFFFFFFF ? FALSE : TRUE)
#define FILE_EXISTW(X) (GetFileAttributesW(X)==0xFFFFFFFF ? FALSE : TRUE)
#define IsURLPathA(X) StrStrA(X, "://")
#define IsURLPathW(X) StrStrW(X, L"://")

static BOOL CALLBACK AddURLDlgProc(HWND, UINT, WPARAM, LPARAM);
static void AddURL(HWND hWnd);

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg);
static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode);

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

static BOOL m_fIsUnicode = FALSE;
static HMODULE m_hDLL = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static void UpdateMenuItems(HMENU hMenu){
	UINT uState = GetMenuState(hMenu, IDM_LIST_MOVETOP, MF_BYCOMMAND);
	if (uState == (UINT)-1) return;
	uState = GetMenuState(hMenu, IDM_LIST_URL, MF_BYCOMMAND);
	if (uState == (UINT)-1){
		int c = GetMenuItemCount(hMenu);
		int i, p = -1;
		for (i = 0; i < c; i++){
			if (GetMenuItemID(hMenu, i) == IDM_LIST_MOVETOP){
				p = i;
				break;
			}
		}
		if (p >= 0){
			InsertMenu(hMenu, p, MF_STRING | MF_BYPOSITION, IDM_LIST_URL, TEXT("アドレスを追加(&U)\tCtrl+U"));
		}else{
			AppendMenu(hMenu, MF_STRING, IDM_LIST_URL, TEXT("アドレスを追加(&U)\tCtrl+U"));
		}
	}else{
		EnableMenuItem(hMenu, IDM_LIST_URL, MF_BYCOMMAND | MF_ENABLED);
	}
}

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg) {
	switch(pMsg->msg){
	case WM_COMMAND:
		if(LOWORD(pMsg->wp)==IDM_LIST_URL){
			AddURL(pMsg->hWnd);
			//pMsg->lMsgResult = 0;
			return TRUE;
		}
		break;
	case WM_INITMENUPOPUP:
		UpdateMenuItems((HMENU)pMsg->wp);
		break;
	}
	return FALSE;
}

static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode) {
	if (eCode == GENERAL_PLUGIN_EVENT_INIT) {
		m_fIsUnicode = ((GetVersion() & 0x80000000) == 0) || IsWindowUnicode(hWnd);
	} else if (eCode == GENERAL_PLUGIN_EVENT_QUIT) {
	}
	return TRUE;
}

static BOOL CALLBACK AddURLDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	static LPVOID pszBuf;
	union {
		CHAR A[MAX_FITTLE_PATH];
		WCHAR W[MAX_FITTLE_PATH];
	} szTemp;

	switch(msg)
	{
		case WM_INITDIALOG:
			pszBuf = (LPVOID)lp;

			// クリップボードに有効なURIが入っていればエディットボックスにコピー
			if(IsClipboardFormatAvailable(m_fIsUnicode ? CF_UNICODETEXT : CF_TEXT)){
				if(OpenClipboard(NULL)){
					HANDLE hMem = GetClipboardData(m_fIsUnicode ? CF_UNICODETEXT : CF_TEXT);
					if (hMem){
						LPVOID pMem = (LPVOID)GlobalLock(hMem);
						if (pMem){
							if (m_fIsUnicode)
								lstrcpynW(szTemp.W, (LPCWSTR)pMem, MAX_FITTLE_PATH);
							else
								lstrcpynA(szTemp.A, (LPCSTR)pMem, MAX_FITTLE_PATH);
							GlobalUnlock(hMem);
							CloseClipboard();
							if (m_fIsUnicode){
								if(IsURLPathW(szTemp.W) || FILE_EXISTW(szTemp.W)){
									SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT1), szTemp.W);
									SendMessageW(GetDlgItem(hDlg, IDC_EDIT1), EM_SETSEL, (WPARAM)0, (LPARAM)lstrlenW(szTemp.W));
								}
							}else{
								if(IsURLPathA(szTemp.A) || FILE_EXISTA(szTemp.A)){
									SetWindowTextA(GetDlgItem(hDlg, IDC_EDIT1), szTemp.A);
									SendMessageA(GetDlgItem(hDlg, IDC_EDIT1), EM_SETSEL, (WPARAM)0, (LPARAM)lstrlenA(szTemp.A));
								}
							}
						}
					}
				}
			}
			SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)TEXT("アドレスを追加"));
			SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp)) {
				case IDOK:	// 設定保存
					BOOL fRet;
					fRet = FALSE;
					if (m_fIsUnicode){
						GetWindowTextW(GetDlgItem(hDlg, IDC_EDIT1), szTemp.W, MAX_FITTLE_PATH);
						if(lstrlenW(szTemp.W)){
							lstrcpynW((LPWSTR)pszBuf, szTemp.W, MAX_FITTLE_PATH);
							fRet = TRUE;
						}
					}else{
						GetWindowTextA(GetDlgItem(hDlg, IDC_EDIT1), szTemp.A, MAX_FITTLE_PATH);
						if(lstrlenA(szTemp.A)){
							lstrcpynA((LPSTR)pszBuf, szTemp.A, MAX_FITTLE_PATH);
							fRet = TRUE;
						}
					}
					EndDialog(hDlg, fRet);
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

static void SendCopyDataW(HWND hFittle, int iType, LPWSTR lpszString){
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
}

static void SendCopyDataA(HWND hFittle, int iType, LPSTR lpszString){
	COPYDATASTRUCT cds;
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpszString;
	cds.cbData = (lstrlenA(lpszString) + 1) * sizeof(CHAR);
	// 文字列送信
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}

static void AddURL(HWND hWnd){
	union {
		WCHAR W[MAX_FITTLE_PATH];
		CHAR A[MAX_FITTLE_PATH];
	} szLabel;
	if (m_fIsUnicode){
		szLabel.W[0] = L'\0';
		if(DialogBoxParamW(m_hDLL, L"TAB_NAME_DIALOG", hWnd, AddURLDlgProc, (LPARAM)szLabel.W)){
			SendCopyDataW(hWnd, 1, szLabel.W);
		}
	}else{
		szLabel.A[0] = '\0';
		if(DialogBoxParamA(m_hDLL, "TAB_NAME_DIALOG", hWnd, AddURLDlgProc, (LPARAM)szLabel.A)){
			SendCopyDataA(hWnd, 1, szLabel.A);
		}
	}
}

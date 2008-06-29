/*
 * addurl
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/plugin.h"
#include <shlwapi.h>

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#define MAX_FITTLE_PATH 260*2
#define FILE_EXISTA(X) (GetFileAttributesA(X)==0xFFFFFFFF ? FALSE : TRUE)
#define FILE_EXISTW(X) (GetFileAttributesW(X)==0xFFFFFFFF ? FALSE : TRUE)
#define IsURLPathA(X) StrStrA(X, "://")
#define IsURLPathW(X) StrStrW(X, L"://")

static BOOL OnInit();
static void OnQuit();
static void OnTrackChange();
static void OnStatusChange();
static void OnConfig();

static BOOL CALLBACK AddURLDlgProc(HWND, UINT, WPARAM, LPARAM);
static void AddURL(HWND hWnd);

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

static BOOL fIsUnicode = FALSE;
static WNDPROC hOldProc = 0;
static HMODULE hDLL = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
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

// サブクラス化したウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_COMMAND:
		if(LOWORD(wp)==IDM_LIST_URL){
			AddURL(hWnd);
			return 0;
		}
		break;
	case WM_INITMENUPOPUP:
		UpdateMenuItems((HMENU)wp);
		break;
	}
	return (fIsUnicode ? CallWindowProcW : CallWindowProcA)(hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	fIsUnicode = ((GetVersion() & 0x80000000) == 0) || IsWindowUnicode(fpi.hParent);
	hOldProc = (WNDPROC)(fIsUnicode ? SetWindowLongW : SetWindowLongA)(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);
	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
static void OnQuit(){
	return;
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
			if(IsClipboardFormatAvailable(fIsUnicode ? CF_UNICODETEXT : CF_TEXT)){
				if(OpenClipboard(NULL)){
					HANDLE hMem = GetClipboardData(fIsUnicode ? CF_UNICODETEXT : CF_TEXT);
					if (hMem){
						LPVOID pMem = (LPVOID)GlobalLock(hMem);
						if (pMem){
							if (fIsUnicode)
								lstrcpynW(szTemp.W, (LPCWSTR)pMem, MAX_FITTLE_PATH);
							else
								lstrcpynA(szTemp.A, (LPCSTR)pMem, MAX_FITTLE_PATH);
							GlobalUnlock(hMem);
							CloseClipboard();
							if (fIsUnicode){
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
					if (fIsUnicode){
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
	if (fIsUnicode){
		szLabel.W[0] = L'\0';
		if(DialogBoxParamW(hDLL, L"TAB_NAME_DIALOG", hWnd, AddURLDlgProc, (LPARAM)szLabel.W)){
			SendCopyDataW(fpi.hParent, 1, szLabel.W);
		}
	}else{
		szLabel.A[0] = '\0';
		if(DialogBoxParamA(hDLL, "TAB_NAME_DIALOG", hWnd, AddURLDlgProc, (LPARAM)szLabel.A)){
			SendCopyDataA(fpi.hParent, 1, szLabel.A);
		}
	}
}

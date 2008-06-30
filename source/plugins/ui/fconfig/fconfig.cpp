#define WINVER		0x0400	// 98以降
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4以降

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <prsht.h>

#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/resource/resource.h"
#include "../../config/cplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#define MAX_FITTLE_PATH 260*2

#define FCONFIG_MAPPING_NAME "fconfig.exe - <shared memory>"

typedef enum {
	SM_SUCCESS,
	SM_ALREADY_EXISTS,
	SM_ERROR
} SHARED_MEMORY_STATUS;

typedef struct {
	HWND hwnd;
} SHARED_MEMORY;

static BOOL OnInit();
static void OnQuit();
static void OnTrackChange();
static void OnStatusChange();
static void OnConfig();

/* 共有メモリ */
static HANDLE hsm = NULL;
static SHARED_MEMORY *psm = NULL;

/* プラグインリスト */
static CONFIG_PLUGIN_INFO *pTop = NULL;

static BOOL fIsUnicode = FALSE;
static WNDPROC hOldProc = 0;

static HMODULE hDLL = 0;

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


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static void CloseSharedMemory(){
	if (psm){
		UnmapViewOfFile(psm);
		psm = NULL;
	}
	if (hsm){
		CloseHandle(hsm);
		hsm = NULL;
	}
}

static SHARED_MEMORY_STATUS OpenSharedMemory(){
	SHARED_MEMORY_STATUS ret = SM_SUCCESS;
	CloseSharedMemory();
	hsm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SHARED_MEMORY), TEXT(FCONFIG_MAPPING_NAME));
	if (!hsm) return SM_ERROR;
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		ret = SM_ALREADY_EXISTS;
	psm = (SHARED_MEMORY *)MapViewOfFile(hsm, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SHARED_MEMORY));
	if (!psm){
		CloseSharedMemory();
		return SM_ERROR;
	}
	return ret;
}

/* プラグインを登録 */
static BOOL RegisterPlugin(FARPROC lpfnProc){
	GetCPluginInfoFunc GetCPluginInfo = (GetCPluginInfoFunc)lpfnProc;
	if (GetCPluginInfo){
		CONFIG_PLUGIN_INFO *pNewPlugins = GetCPluginInfo();
		if (pNewPlugins){
			CONFIG_PLUGIN_INFO *pNext = pNewPlugins;
			do{
				pNext = pNext->pNext;
			} while (pNext);
			for (pNext = pNewPlugins; pNext->pNext; pNext = pNext->pNext);
			pNext->pNext = pTop;
			pTop = pNewPlugins;
			return TRUE;
		}
	}
	return FALSE;
}

static void InitPlugins(){
	HANDLE hFind;
	union {
		CHAR A[MAX_PATH];
		WCHAR W[MAX_PATH];
	} szDir, szPath;
	union {
		WIN32_FIND_DATAA A;
		WIN32_FIND_DATAW W;
	} wfd;

	if (fIsUnicode) {
		GetModuleFileNameW(NULL, szDir.W, MAX_FITTLE_PATH);
		*PathFindFileNameW(szDir.W) = L'\0';
		PathCombineW(szPath.W, szDir.W, L"*.fcp");
		hFind = FindFirstFileW(szPath.W, &wfd.W);
	} else {
		GetModuleFileNameA(NULL, szDir.A, MAX_FITTLE_PATH);
		*PathFindFileNameA(szDir.A) = '\0';
		PathCombineA(szPath.A, szDir.A, "*.fcp");
		hFind = FindFirstFileA(szPath.A, &wfd.A);
	}

	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			HINSTANCE hDll;
			if (fIsUnicode){
				PathCombineW(szPath.W, szDir.W, wfd.W.cFileName);
				hDll = LoadLibraryW(szPath.W);
			}else{
				PathCombineA(szPath.A, szDir.A, wfd.A.cFileName);
				hDll = LoadLibraryA(szPath.A);
			}
			if(hDll){
				FARPROC pfnCPlugin = GetProcAddress(hDll, "GetCPluginInfo");
				if (!pfnCPlugin || !RegisterPlugin(pfnCPlugin))
				{
					FreeLibrary(hDll);
				}
			}
		}while(fIsUnicode ? FindNextFileW(hFind, &wfd.W) : FindNextFileA(hFind, &wfd.A));
		FindClose(hFind);
	}
}

static void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}

static void HFree(void *pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}

static void SheetActivate(LPPROPSHEETHEADER ppsh){
	if (psm){
		HWND hwnd = psm->hwnd;
		if (hwnd && IsWindow(hwnd)){
			PropSheet_SetCurSel(hwnd, ppsh->nStartPage, ppsh->nStartPage);
			ShowWindow(hwnd, SW_SHOW);
			SetForegroundWindow(hwnd);
		}
	}
}

static void SheetInvoke(LPPROPSHEETHEADER ppsh){
	if (ppsh->phpage && ppsh->nPages > 0){
		int i;
		PropertySheet(ppsh);
		/* ページはPropertySheet内で開放されるため二重開放を防止する */
		for (i = 0; i < ppsh->nPages; i++){
			if (ppsh->phpage[i]) ppsh->phpage[i] = 0;
		}
	}
}
static void SheetFree(LPPROPSHEETHEADER ppsh){
	if (ppsh->phpage){
		int i;
		for (i = 0; i < ppsh->nPages; i++){
			if (ppsh->phpage[i]) DestroyPropertySheetPage(ppsh->phpage[i]);
		}
		HFree(ppsh->phpage);
		ppsh->phpage = 0;
	}
}

static int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam){
	if (uMsg == PSCB_INITIALIZED){
		if (psm) psm->hwnd = hwndDlg;
	}
	return 0;
}

static int SheetSetup(PROPSHEETHEADER *ppsh, HWND hwndParent, LPCSTR lpszStartPath){
	int nMaxPage = 0;
	int nNumPage = 0;
	int nStartPage = 0;
	CONFIG_PLUGIN_INFO *pCur;

	ppsh->phpage = 0;
	ppsh->nPages = 0;

	for (pCur = pTop; pCur; pCur = pCur->pNext)
		nMaxPage += pCur->GetConfigPageCount();

	if (nMaxPage == 0) return 0;
	
	ppsh->phpage = (HPROPSHEETPAGE FAR *)HAlloc(sizeof(HPROPSHEETPAGE) * nMaxPage);
	if (ppsh->phpage) {
		int nLevel;
		for (nLevel = 0; nLevel < 4; nLevel++){
			for (pCur = pTop; pCur; pCur = pCur->pNext){
				int nIndex = 0;
				HPROPSHEETPAGE hpspAdd;
				do{
					CHAR szPath[64];
					hpspAdd = pCur->GetConfigPage(nIndex++, nLevel, szPath, 64);
					if (hpspAdd && nNumPage < nMaxPage){
						if (lpszStartPath && lstrcmpA(szPath, lpszStartPath) == 0)
							nStartPage = nNumPage;
						ppsh->phpage[nNumPage++] = hpspAdd;
					}
				}while(hpspAdd);
			}
		}
		if (nNumPage > 0){
			ppsh->dwSize = sizeof (PROPSHEETHEADER);
			ppsh->dwFlags = PSH_USEICONID | PSH_USECALLBACK;
			ppsh->hwndParent = hwndParent;
			ppsh->hInstance = GetModuleHandle(NULL);
			ppsh->pszIcon = TEXT("MYICON");
			ppsh->pszCaption = TEXT("設定");
			ppsh->nPages = nNumPage;
			ppsh->nStartPage = nStartPage;
			ppsh->pfnCallback = PropSheetProc; 
		}
	}
	return nNumPage;
}

static void ShowSettingDialog(HWND hWnd, int nPage){
	static const LPCSTR table[] = {
		"fittle/general",
		"fittle/path",
		"fittle/control",
		"fittle/tasktray",
		"fittle/hotkey",
		"fittle/about"
	};
	SHARED_MEMORY_STATUS sms = OpenSharedMemory();

	if (nPage < 0 || nPage > WM_F4B24_IPC_SETTING_LP_MAX) nPage = 0;
	if (SM_ERROR != sms){
		PROPSHEETHEADER psh;
		int nNumPage = SheetSetup(&psh, hWnd, table[nPage]);
		if (sms == SM_SUCCESS)
			SheetInvoke(&psh);
		else if (sms == SM_ALREADY_EXISTS)
			SheetActivate(&psh);
		SheetFree(&psh);
		CloseSharedMemory();
	}
}


// サブクラス化したウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	if (msg == WM_COMMAND){
		if (LOWORD(wp) == IDM_SETTING){
			ShowSettingDialog(hWnd, 0);
			return 0;
		} else if (LOWORD(wp) == IDM_VER){
			ShowSettingDialog(hWnd, 5);
			return 0;
		}
	} else if ((msg == WM_F4B24_IPC) && (wp == WM_F4B24_IPC_SETTING)){
		ShowSettingDialog(hWnd, lp);
		return 0;
	}
	return (fIsUnicode ? CallWindowProcW : CallWindowProcA)(hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	fIsUnicode = ((GetVersion() & 0x80000000) == 0) || IsWindowUnicode(fpi.hParent);
	InitPlugins();
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
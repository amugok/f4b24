#define WINVER		0x0400	// 98以降
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4以降

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <prsht.h>

#include "../../../fittle/src/gplugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/resource/resource.h"
#include "../../config/cplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
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

#define FCONFIG_MAPPING_NAME "fconfig.exe - <shared memory>"

typedef enum {
	SM_SUCCESS,
	SM_ALREADY_EXISTS,
	SM_ERROR
} SHARED_MEMORY_STATUS;

typedef struct {
	HWND hwnd;
} SHARED_MEMORY;

/* 共有メモリ */
static HANDLE m_hsm = NULL;
static SHARED_MEMORY *m_psm = NULL;

/* プラグインリスト */
static CONFIG_PLUGIN_INFO *m_pTop = NULL;

static BOOL m_fInitialized = FALSE;

#include "../../../fittle/src/wastr.h"
#include "../../../fittle/src/wastr.cpp"

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


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static void CloseSharedMemory(){
	if (m_psm){
		UnmapViewOfFile(m_psm);
		m_psm = NULL;
	}
	if (m_hsm){
		CloseHandle(m_hsm);
		m_hsm = NULL;
	}
}

static SHARED_MEMORY_STATUS OpenSharedMemory(){
	SHARED_MEMORY_STATUS ret = SM_SUCCESS;
	CloseSharedMemory();
	m_hsm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SHARED_MEMORY), TEXT(FCONFIG_MAPPING_NAME));
	if (!m_hsm) return SM_ERROR;
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		ret = SM_ALREADY_EXISTS;
	m_psm = (SHARED_MEMORY *)MapViewOfFile(m_hsm, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SHARED_MEMORY));
	if (!m_psm){
		CloseSharedMemory();
		return SM_ERROR;
	}
	return ret;
}

/* プラグインを登録 */
static BOOL CALLBACK RegisterPlugin(HMODULE hPlugin, LPVOID user){
	FARPROC lpfnProc = GetProcAddress(hPlugin, "GetCPluginInfo");
	if (lpfnProc){
		CONFIG_PLUGIN_INFO *pNewPlugins = ((GetCPluginInfoFunc)lpfnProc)();
		if (pNewPlugins){
			CONFIG_PLUGIN_INFO *pNext = pNewPlugins;
			for (pNext = pNewPlugins; pNext->pNext; pNext = pNext->pNext);
			pNext->pNext = m_pTop;
			m_pTop = pNewPlugins;
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL InitPlugins(){
	if (!m_fInitialized) {
		WAEnumPlugins(NULL, "Plugins\\fcp\\", "*.fcp", RegisterPlugin, 0);
		m_fInitialized = TRUE;
	}
	return TRUE;
}

static void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}

static void HFree(void *pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}

static void SheetActivate(LPPROPSHEETHEADER ppsh){
	if (m_psm){
		HWND hwnd = m_psm->hwnd;
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
		if (m_psm) m_psm->hwnd = hwndDlg;
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

	for (pCur = m_pTop; pCur; pCur = pCur->pNext)
		nMaxPage += pCur->GetConfigPageCount();

	if (nMaxPage == 0) return 0;
	
	ppsh->phpage = (HPROPSHEETPAGE FAR *)HAlloc(sizeof(HPROPSHEETPAGE) * nMaxPage);
	if (ppsh->phpage) {
		int nLevel;
		for (nLevel = 0; nLevel < 4; nLevel++){
			for (pCur = m_pTop; pCur; pCur = pCur->pNext){
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

	if (nPage < 0 || nPage > 5) nPage = 0;
	if (SM_ERROR != sms){
		if (InitPlugins()) {
			PROPSHEETHEADER psh;
			int nNumPage = SheetSetup(&psh, hWnd, table[nPage]);
			if (sms == SM_SUCCESS)
				SheetInvoke(&psh);
			else if (sms == SM_ALREADY_EXISTS)
				SheetActivate(&psh);
			SheetFree(&psh);
		}
		CloseSharedMemory();
	}
}


static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg) {
	if (pMsg->msg == WM_COMMAND){
		if (LOWORD(pMsg->wp) == IDM_SETTING){
			ShowSettingDialog(pMsg->hWnd, 0);
			return TRUE;
		} else if (LOWORD(pMsg->wp) == IDM_VER){
			ShowSettingDialog(pMsg->hWnd, 5);
			return TRUE;
		}
	} else if ((pMsg->msg == WM_F4B24_IPC) && (pMsg->wp == WM_F4B24_IPC_SETTING)){
		ShowSettingDialog(pMsg->hWnd, pMsg->lp);
		return TRUE;
	}
	return FALSE;
}

static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode) {
	if (eCode == GENERAL_PLUGIN_EVENT_INIT) {
		WAIsUnicode();
	} else if (eCode == GENERAL_PLUGIN_EVENT_QUIT) {
	}
	return TRUE;
}


/*
 * wadsp.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include <windows.h>
#include <shlwapi.h>
#include "../../../fittle/src/gplugin.h"
#include "../../../fittle/src/f4b24.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(linker, "/EXPORT:GetGPluginInfo=_GetGPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

typedef DWORD HWADSP;
typedef DWORD HDSP;

typedef LRESULT (CALLBACK WINAMPWINPROC)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef void (WINAPI *LPBASS_WADSP_Init)(HWND hwndMain);
typedef HWADSP (WINAPI *LPBASS_WADSP_Load)(const void *dspfile, int x, int y, int width, int height, WINAMPWINPROC *proc);
typedef void (WINAPI *LPBASS_WADSP_Start)(HWADSP plugin, DWORD module, DWORD hchan);
typedef HDSP (WINAPI *LPBASS_WADSP_ChannelSetDSP)(HWADSP plugin, DWORD hchan, int priority);
typedef int (WINAPI *LPBASS_WADSP_ChannelRemoveDSP)(HWADSP plugin);
typedef void (WINAPI *LPBASS_WADSP_Stop)(HWADSP plugin);
typedef void (WINAPI *LPBASS_WADSP_Free)(void);
typedef void (WINAPI *LPBASS_WADSP_Config)(HWADSP plugin);
typedef LPCSTR (WINAPI *LPBASS_WADSP_GetName)(HWADSP plugin);

typedef struct WADSPNODE {
	struct WADSPNODE *pNext;
	HWADSP h;
	HDSP hdsp;
} WADSPNODE;

static LPBASS_WADSP_Init BASS_WADSP_Init = NULL;
static LPBASS_WADSP_Load BASS_WADSP_Load = NULL;
static LPBASS_WADSP_Start BASS_WADSP_Start = NULL;
static LPBASS_WADSP_ChannelSetDSP BASS_WADSP_ChannelSetDSP = NULL;
static LPBASS_WADSP_ChannelRemoveDSP BASS_WADSP_ChannelRemoveDSP = NULL;
static LPBASS_WADSP_Stop BASS_WADSP_Stop = NULL;
static LPBASS_WADSP_Free BASS_WADSP_Free = NULL;
static LPBASS_WADSP_Config BASS_WADSP_Config = NULL;
static LPBASS_WADSP_GetName BASS_WADSP_GetName = NULL;

#define FUNC_PREFIXA "BASS_WADSP_"
static /*const*/ CHAR szDllNameA[] = "bass_wadsp.dll";
static /*const*/ struct IMPORT_FUNC_TABLE {
	LPSTR lpszFuncName;
	FARPROC * ppFunc;
} functbl[] = {
	{ "Init", (FARPROC *)&BASS_WADSP_Init },
	{ "Load", (FARPROC *)&BASS_WADSP_Load },
	{ "Start", (FARPROC *)&BASS_WADSP_Start },
	{ "ChannelSetDSP", (FARPROC *)&BASS_WADSP_ChannelSetDSP },
	{ "ChannelRemoveDSP", (FARPROC *)&BASS_WADSP_ChannelRemoveDSP },
	{ "Stop", (FARPROC *)&BASS_WADSP_Stop },
	{ "Free", (FARPROC *)&BASS_WADSP_Free },
	{ "Config", (FARPROC *)&BASS_WADSP_Config },
	{ "GetName", (FARPROC *)&BASS_WADSP_GetName },
	{ 0, (FARPROC *)0 }
};

static HMODULE hWaDsp = NULL;
static WADSPNODE *pDspListTop = NULL;
static DWORD hTarget = 0;

static enum {
	NOT_INITIALIZED,
	INITIALIZED,
	TERMINATED
} m_fInitialized = NOT_INITIALIZED;

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

static LPVOID HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
}

static void HFree(LPVOID lp){
	HeapFree(GetProcessHeap(), 0, lp);
}

static BOOL LoadBASSWADSP(void){
	const struct IMPORT_FUNC_TABLE *pft;
	CHAR funcname[32];
	WASTR szPath;
	int l;
	if (hWaDsp) return TRUE;

	WAGetModuleParentDir(NULL, &szPath);
	WAstrcatA(&szPath, "Plugins\\BASS\\");
	WAstrcatA(&szPath, szDllNameA);
	hWaDsp = WALoadLibrary(&szPath);
	if(!hWaDsp) return FALSE;

	lstrcpynA(funcname, FUNC_PREFIXA, 32);
	l = lstrlenA(funcname);

	for (pft = functbl; pft->lpszFuncName; pft++){
		lstrcpynA(funcname + l, pft->lpszFuncName, 32 - l);
		FARPROC fp = GetProcAddress(hWaDsp, funcname);
		if (!fp){
			FreeLibrary(hWaDsp);
			hWaDsp = NULL;
			return FALSE;
		}
		*pft->ppFunc = fp;
	}
	return TRUE;
}

static BOOL CALLBACK LoadDSPPluginProc(LPWASTR lpPath, LPVOID user){
	WADSPNODE *pNew = (WADSPNODE *)HAlloc(sizeof(WADSPNODE));
	if (pNew) {
		pNew->h = BASS_WADSP_Load(lpPath, 0, 0, 5, 5, NULL);
		pNew->hdsp = 0;
		if(pNew->h){
			BASS_WADSP_Start(pNew->h, 0, 0);
			pNew->pNext = pDspListTop;
			pDspListTop = pNew;
			return TRUE;
		}
		HFree(pNew);
	}
	return FALSE;
}

static BOOL InitBassWaDsp(HWND hWnd){
	if (m_fInitialized != NOT_INITIALIZED) return m_fInitialized == INITIALIZED;
	m_fInitialized = INITIALIZED;
	BASS_WADSP_Init(hWnd);
	WAEnumFiles(NULL, "Plugins\\wadsp\\", "dsp_*.dll", LoadDSPPluginProc, 0);
	return TRUE;
}

static void DetachDSP(WADSPNODE *pDsp) {
	if (pDsp->hdsp) {
		BASS_WADSP_ChannelRemoveDSP(pDsp->h);
		//BASS_WADSP_Stop(pDsp->h);
		pDsp->hdsp = 0;
	}
}

static BOOL SetBassWaDsp(HWND hWnd, DWORD hChan, BOOL fSw){
	if (hChan && InitBassWaDsp(hWnd)) {
		WADSPNODE *pDsp = pDspListTop;
		while (pDsp){
			WADSPNODE *pNext = pDsp->pNext;
			DetachDSP(pDsp);
			if (fSw) {
				//BASS_WADSP_Start(pDsp->h, 0, 0);
				pDsp->hdsp = BASS_WADSP_ChannelSetDSP(pDsp->h, hChan, 0);
				pDsp = pNext;
			}
		}
	}
	return TRUE;
}

static void FreeBassWaDsp(){
	hTarget = 0;
	if (m_fInitialized == INITIALIZED) {
		WADSPNODE *pDsp = pDspListTop;
		m_fInitialized = TERMINATED;
		while (pDsp){
			WADSPNODE *pNext = pDsp->pNext;
			DetachDSP(pDsp);
			BASS_WADSP_Stop(pDsp->h);
			HFree(pDsp);
			pDsp = pNext;
		}
		pDspListTop = NULL;
		if(BASS_WADSP_Free) BASS_WADSP_Free();
	}
/*
	if (hWaDsp) {
		FreeLibrary(hWaDsp);
		hWaDsp = NULL;
	}
*/
}

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg){
	switch(pMsg->msg){
	case WM_F4B24_IPC:
		if (pMsg->wp == WM_F4B24_HOOK_START_DECODE_STREAM) {
			DWORD hChan = (DWORD)pMsg->lp;
			SetBassWaDsp(pMsg->hWnd, hChan, TRUE);
//		} else if (pMsg->wp == WM_F4B24_HOOK_FREE_STREAM) {
//			SetBassWaDsp(pMsg->hWnd, 0, FALSE);
		} else if (pMsg->wp == WM_F4B24_IPC_INVOKE_WADSP_SETUP && InitBassWaDsp(pMsg->hWnd)){
			int i;
			WADSPNODE *pDsp = pDspListTop;
			for (i = 0; pDsp; i++){
				WADSPNODE *pNext = pDsp->pNext;
				if (i == pMsg->lp){
					BASS_WADSP_Config(pDsp->h);
					break;
				}
				pDsp = pNext;
			}
		} else if (pMsg->wp == WM_F4B24_IPC_GET_WADSP_LIST && InitBassWaDsp(pMsg->hWnd)){
			int i, l = 1;
			WADSPNODE *pDsp = pDspListTop;
			char *p;
			for (i = 0; pDsp; i++){
				WADSPNODE *pNext = pDsp->pNext;
				LPCSTR n = BASS_WADSP_GetName(pDsp->h);
				l += lstrlenA(n) + 1;
				pDsp = pNext;
			}
			p = (char *)HAlloc(l);
			if (p){
				int s = 0;
				pDsp = pDspListTop;
				for (i = 0; pDsp; i++){
					WADSPNODE *pNext = pDsp->pNext;
					LPCSTR n = BASS_WADSP_GetName(pDsp->h);
					lstrcpyA(p + s, n);
					s += lstrlenA(n);
					p[s++] = '\n';
					p[s] = '\0';
					pDsp = pNext;
				}
				SendMessageA((HWND)pMsg->lp, WM_SETTEXT, (WPARAM)0, (LPARAM)p);
				HFree(p);
			}
		}
		break;
	}
	return FALSE;
}

static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode) {
	if (eCode == GENERAL_PLUGIN_EVENT_INIT0) {
		WAIsUnicode();
		if (!LoadBASSWADSP()) return FALSE;
	} else if (eCode == GENERAL_PLUGIN_EVENT_QUIT) {
		FreeBassWaDsp();
	}
	return TRUE;
}

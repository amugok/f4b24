/*
 * wadsp.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include <shlwapi.h>

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"gdi32.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

typedef DWORD HWADSP;

typedef LRESULT (CALLBACK WINAMPWINPROC)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef void (WINAPI *LPBASS_WADSP_Init)(HWND hwndMain);
typedef HWADSP (WINAPI *LPBASS_WADSP_Load)(const void *dspfile, int x, int y, int width, int height, WINAMPWINPROC *proc);
typedef void (WINAPI *LPBASS_WADSP_Start)(HWADSP plugin, DWORD module, DWORD hchan);
typedef int (WINAPI *LPBASS_WADSP_ChannelSetDSP)(HWADSP plugin, DWORD hchan, int priority);
typedef int (WINAPI *LPBASS_WADSP_ChannelRemoveDSP)(HWADSP plugin);
typedef void (WINAPI *LPBASS_WADSP_Stop)(HWADSP plugin);
typedef void (WINAPI *LPBASS_WADSP_Free)(void);
typedef void (WINAPI *LPBASS_WADSP_Config)(HWADSP plugin);
typedef LPCSTR (WINAPI *LPBASS_WADSP_GetName)(HWADSP plugin);

typedef struct WADSPNODE {
	struct WADSPNODE *pNext;
	HWADSP h;
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

static HMODULE m_hDLL = 0;
static BOOL m_fIsUnicode = FALSE;
static WNDPROC m_hOldProc = 0;
static BOOL m_fInitialized = FALSE;

#include "../../../fittle/src/wastr.h"
#include "../../../fittle/src/wastr.cpp"

static BOOL OnInit();
static void OnQuit();
static void OnTrackChange();
static void OnStatusChange();
static void OnConfig();

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
		m_hDLL = hinstDLL;
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

static BOOL InitBassWaDsp(){
	if (!LoadBASSWADSP()) return FALSE;
	if (m_fInitialized) return TRUE;
	m_fInitialized = TRUE;
	BASS_WADSP_Init(fpi.hParent);
	WAEnumFiles(NULL, "Plugins\\wadsp\\", "dsp_*.dll", LoadDSPPluginProc, 0);
	return TRUE;
}

static BOOL SetBassWaDsp(DWORD hChan, BOOL fSw){
	if (InitBassWaDsp()) {
		WADSPNODE *pDsp = pDspListTop;
		while (pDsp){
			WADSPNODE *pNext = pDsp->pNext;
			if (fSw) {
				BASS_WADSP_ChannelSetDSP(pDsp->h, hChan, 0);
			} else {
				BASS_WADSP_ChannelRemoveDSP(pDsp->h);
			}
			pDsp = pNext;
		}
	}
	return TRUE;
}

static void FreeBassWaDsp(){
	if (m_fInitialized) {
		WADSPNODE *pDsp = pDspListTop;
		while (pDsp){
			WADSPNODE *pNext = pDsp->pNext;
			BASS_WADSP_Stop(pDsp->h);
			HFree(pDsp);
			pDsp = pNext;
		}
		pDspListTop = NULL;
		if(BASS_WADSP_Free) BASS_WADSP_Free();
	}
}

// サブクラス化したウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_F4B24_IPC:
		if (wp == WM_F4B24_HOOK_CREATE_DECODE_STREAM) {
			SetBassWaDsp((DWORD)lp, TRUE);
		} else if (wp == WM_F4B24_HOOK_FREE_DECODE_STREAM) {
			SetBassWaDsp((DWORD)lp, FALSE);
		} else if (wp == WM_F4B24_IPC_INVOKE_WADSP_SETUP){
			int i;
			WADSPNODE *pDsp = pDspListTop;
			for (i = 0; pDsp; i++){
				WADSPNODE *pNext = pDsp->pNext;
				if (i == lp){
					BASS_WADSP_Config(pDsp->h);
					break;
				}
				pDsp = pNext;
			}
		} else if (wp == WM_F4B24_IPC_GET_WADSP_LIST && InitBassWaDsp()){
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
				SendMessageA((HWND)lp, WM_SETTEXT, (WPARAM)0, (LPARAM)p);
				HFree(p);
			}
		}
		break;
	}
	return (m_fIsUnicode ? CallWindowProcW : CallWindowProcA)(m_hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	m_fIsUnicode = WAIsUnicode() || IsWindowUnicode(fpi.hParent);
	m_hOldProc = (WNDPROC)(m_fIsUnicode ? SetWindowLongW : SetWindowLongA)(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);
	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
static void OnQuit(){
	FreeBassWaDsp();
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

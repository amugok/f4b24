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
#pragma comment(lib,"shlwapi.lib")
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
static /*const*/ WCHAR szDllNameW[] =L"bass_wadsp.dll";
static /*const*/ struct IMPORT_FUNC_TABLE {
	LPSTR lpszFuncName;
	FARPROC * ppFunc;
} functbl[] = {
	{ FUNC_PREFIXA "Init", (FARPROC *)&BASS_WADSP_Init },
	{ FUNC_PREFIXA "Load", (FARPROC *)&BASS_WADSP_Load },
	{ FUNC_PREFIXA "Start", (FARPROC *)&BASS_WADSP_Start },
	{ FUNC_PREFIXA "ChannelSetDSP", (FARPROC *)&BASS_WADSP_ChannelSetDSP },
	{ FUNC_PREFIXA "ChannelRemoveDSP", (FARPROC *)&BASS_WADSP_ChannelRemoveDSP },
	{ FUNC_PREFIXA "Stop", (FARPROC *)&BASS_WADSP_Stop },
	{ FUNC_PREFIXA "Free", (FARPROC *)&BASS_WADSP_Free },
	{ FUNC_PREFIXA "Config", (FARPROC *)&BASS_WADSP_Config },
	{ FUNC_PREFIXA "GetName", (FARPROC *)&BASS_WADSP_GetName },
	{ 0, (FARPROC *)0 }
};

static HMODULE hWaDsp = NULL;
static WADSPNODE *pDspListTop = NULL;
static int nCount = 0;

static ATOM F4B24_WADSP_INVOKE_CONFIG = 0;
static ATOM F4B24_WADSP_GET_LIST = 0;

static HMODULE hDLL = 0;
static BOOL fIsUnicode = FALSE;
static WNDPROC hOldProc = 0;

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
		hDLL = hinstDLL;
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

static BOOL InitBassWaDsp(HWND hWnd){
	const struct IMPORT_FUNC_TABLE *pft;
	HANDLE hFind;
	union{
		CHAR A[MAX_PATH];
		WCHAR W[MAX_PATH];
	} szDir, szPath;
	union{
		WIN32_FIND_DATAA A;
		WIN32_FIND_DATAW W;
	} wfd;
	int i = 0;
	hWaDsp = fIsUnicode ? LoadLibraryW(szDllNameW) : LoadLibraryA(szDllNameA);
	if(!hWaDsp){
		return FALSE;
	}
	for (pft = functbl; pft->lpszFuncName; pft++){
		FARPROC fp = GetProcAddress(hWaDsp, pft->lpszFuncName);
		if (!fp){
			FreeLibrary(hWaDsp);
			hWaDsp = NULL;
			return FALSE;
		}
		*pft->ppFunc = fp;
	}

	BASS_WADSP_Init(hWnd);

	if (fIsUnicode){
		GetModuleFileNameW(NULL, szDir.W, MAX_PATH);
		PathFindFileNameW(szDir.W)[0] = L'\0';
		PathCombineW(szPath.W, szDir.W, L"dsp_*.dll");
		hFind = FindFirstFileW(szPath.W, &wfd.W);
	}else{
		GetModuleFileNameA(NULL, szDir.A, MAX_PATH);
		PathFindFileNameA(szDir.A)[0] = '\0';
		PathCombineA(szPath.A, szDir.A, "dsp_*.dll");
		hFind = FindFirstFileA(szPath.A, &wfd.A);
	}

	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			WADSPNODE *pNew = (WADSPNODE *)HAlloc(sizeof(WADSPNODE));
			if (pNew) {
				if (fIsUnicode){
					PathCombineW(szPath.W, szDir.W, wfd.W.cFileName);
				}else{
					PathCombineA(szPath.A, szDir.A, wfd.A.cFileName);
				}
				pNew->h = BASS_WADSP_Load(fIsUnicode ? (void *)szPath.W : (void *)szPath.A, 0, 0, 5, 5, NULL);
				if(pNew->h){
					BASS_WADSP_Start(pNew->h, 0, 0);
					i++;
					pNew->pNext = pDspListTop;
					pDspListTop = pNew;
				} else {
					HFree(pNew);
				}
			}
		}while(fIsUnicode ? FindNextFileW(hFind, &wfd.W) : FindNextFileA(hFind, &wfd.A));
		FindClose(hFind);
	}
	nCount = i;
	return TRUE;
}

static BOOL SetBassWaDsp(DWORD hChan, BOOL fSw){
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
	return TRUE;
}		

static void FreeBassWaDsp(){
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

// サブクラス化したウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_F4B24_IPC:
		if (wp == WM_F4B24_HOOK_CREATE_STREAM) {
			SetBassWaDsp((DWORD)lp, TRUE);
		} else if (wp == WM_F4B24_HOOK_FREE_STREAM) {
			SetBassWaDsp((DWORD)lp, FALSE);
		} else if (F4B24_WADSP_INVOKE_CONFIG && wp == F4B24_WADSP_INVOKE_CONFIG){
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
		} else if (F4B24_WADSP_GET_LIST && wp == F4B24_WADSP_GET_LIST){
			int i;
			WADSPNODE *pDsp = pDspListTop;
			for (i = 0; pDsp; i++){
				WADSPNODE *pNext = pDsp->pNext;
				LRESULT lRet = SendMessageA((HWND)lp, LB_ADDSTRING, (WPARAM)0, (LPARAM)BASS_WADSP_GetName(pDsp->h));
				if (lRet != LB_ERR && lRet != LB_ERRSPACE){
					lRet = SendMessageA((HWND)lp, LB_SETITEMDATA, (WPARAM)lRet, (LPARAM)i);
				}
				pDsp = pNext;
			}
		}
		break;
	}
	return (fIsUnicode ? CallWindowProcW : CallWindowProcA)(hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	fIsUnicode = ((GetVersion() & 0x80000000) == 0) || IsWindowUnicode(fpi.hParent);
	F4B24_WADSP_INVOKE_CONFIG = AddAtom(TEXT("F4B24_WADSP_INVOKE_CONFIG"));
	F4B24_WADSP_GET_LIST = AddAtom(TEXT("F4B24_WADSP_GET_LIST"));
	InitBassWaDsp(fpi.hParent);
	hOldProc = (WNDPROC)(fIsUnicode ? SetWindowLongW : SetWindowLongA)(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);
	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
static void OnQuit(){
	FreeBassWaDsp();
	if (F4B24_WADSP_INVOKE_CONFIG){
		DeleteAtom(F4B24_WADSP_INVOKE_CONFIG);
		F4B24_WADSP_INVOKE_CONFIG = 0;
	}
	if (F4B24_WADSP_GET_LIST){
		DeleteAtom(F4B24_WADSP_GET_LIST);
		F4B24_WADSP_GET_LIST = 0;
	}
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

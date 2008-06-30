/*
 * taskvol
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/src/plugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/SECTION:.sharedata,RWS")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

#define IDM_VOLUP	40058
#define IDM_VOLDOWN	40059

static HMODULE hDLL = 0;
static BOOL g_fDefProcUnicode = FALSE;
static WNDPROC g_DefProc = NULL;
static enum {
	notinitialized,
	subclassed,
	uninstalled
} g_state = notinitialized;

#pragma data_seg(".sharedata")
HHOOK	g_hHook = NULL;
HWND	g_hTaskWnd = NULL;
HWND	g_hParent = NULL;
#pragma data_seg()

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


static void UnsubclassWindow(void){
	if (g_state == subclassed){
		if (IsWindow(g_hTaskWnd)){
			(g_fDefProcUnicode ? SetWindowLongW : SetWindowLongA)(g_hTaskWnd, GWL_WNDPROC, IsBadCodePtr((FARPROC)g_DefProc) ? (LONG)(g_fDefProcUnicode ? GetClassLongW : GetClassLongA)(g_hTaskWnd, GCL_WNDPROC) : (LONG)g_DefProc);
		}
		g_state = uninstalled;
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
		UnsubclassWindow();
	}
	return TRUE;
}

/* サブクラス化したプロシージャ */
static LRESULT CALLBACK SubClassProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp){
	switch(msg)	{
		case WM_MOUSEWHEEL:
			if((signed short)HIWORD(wp) > 0){
				SendMessage(g_hParent, WM_COMMAND, IDM_VOLUP, 0);
			}else{
				SendMessage(g_hParent, WM_COMMAND, IDM_VOLDOWN, 0);
			}
			return 0;

		case WM_APP + 5:
			UnsubclassWindow();
			return 0;
	}
	return (g_fDefProcUnicode ? CallWindowProcW : CallWindowProcA)(IsBadCodePtr((FARPROC)g_DefProc) ? (WNDPROC)(g_fDefProcUnicode ? GetClassLongW : GetClassLongA)(hWnd, GCL_WNDPROC): g_DefProc, hWnd, msg, wp , lp);
}

/* フックプロシージャ */
static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPCWPSTRUCT	lpcs = (LPCWPSTRUCT)lParam;

	if(nCode == HC_ACTION){
		if(g_state == notinitialized && lpcs->hwnd == g_hTaskWnd){
			g_state = subclassed;
			g_fDefProcUnicode = IsWindowUnicode(g_hTaskWnd);
			g_DefProc = (WNDPROC)(g_fDefProcUnicode ? SetWindowLongW : SetWindowLongA)(g_hTaskWnd, GWL_WNDPROC, (LONG)SubClassProc);
		}
	}
	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	DWORD idThread;

	g_hTaskWnd = FindWindow("Shell_TrayWnd", NULL);
	if(g_hTaskWnd == NULL){
		return FALSE;
	}

	idThread = GetWindowThreadProcessId(g_hTaskWnd, NULL);
	g_hHook = (IsWindowUnicode(g_hTaskWnd) ? SetWindowsHookExW : SetWindowsHookExA)(WH_CALLWNDPROC, CallWndProc, fpi.hDllInst, idThread);
	if(g_hHook == NULL){
		return FALSE;
	}
	
	g_hParent = fpi.hParent;

	SendMessage(g_hTaskWnd, WM_SIZE, SIZE_RESTORED, 0);

	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
static void OnQuit(){
	SendMessage(g_hTaskWnd, WM_APP + 5, 0, 0);
	if(g_hHook){
		UnhookWindowsHookEx(g_hHook);
		g_hHook = 0;
	}
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
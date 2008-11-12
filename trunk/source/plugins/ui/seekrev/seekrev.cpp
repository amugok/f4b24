#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/plugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL	0x020A
#endif

#define ID_REBAR	109
#define TD_SEEKBAR	104


static HMODULE m_hDLL = 0;
static BOOL m_bOldProc = FALSE;
static WNDPROC m_hOldProc = 0;

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

// サブクラス化したウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_MOUSEWHEEL:
		SendMessage(fpi.hParent, WM_COMMAND, MAKEWPARAM(((short)HIWORD(wp)<0) ? IDM_SEEKBACK : IDM_SEEKFRONT, 0), 0);
		return 0;
	}
	return (m_bOldProc ? CallWindowProcW : CallWindowProcA)(m_hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	HWND hSeekbar = GetDlgItem(GetDlgItem(fpi.hParent, ID_REBAR), TD_SEEKBAR);
	m_bOldProc = IsWindowUnicode(hSeekbar);
	m_hOldProc = (WNDPROC)(m_bOldProc ? SetWindowLongW : SetWindowLongA)(hSeekbar, GWL_WNDPROC, (LONG)WndProc);
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
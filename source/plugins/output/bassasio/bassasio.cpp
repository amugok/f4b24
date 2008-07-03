#define WINVER		0x0400	// 98à»ç~
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4à»ç~
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "../../../../extra/bass24/bass.h"
#include "../../../../extra/bassasio10/bassasio.h"
#include "../../../fittle/src/oplugin.h"
#include "../../../fittle/src/f4b24.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"../../../../extra/bass24/bass.lib")
#pragma comment(lib,"../../../../extra/bassasio10/bassasio.lib")
#pragma comment(linker, "/EXPORT:GetOPluginInfo=_GetOPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif


static HMODULE hDLL = 0;

#define OUTPUT_PLUGIN_ID_BASE (0x20000)

static int CALLBACK GetDeviceCount(void);
static DWORD CALLBACK GetDefaultDeviceID(void);
static DWORD CALLBACK GetDeviceID(int nIndex);
static BOOL CALLBACK GetDeviceNameA(DWORD dwID, LPSTR szBuf, int nBufSize);
static int CALLBACK Init(DWORD dwID);
static int CALLBACK GetStatus(void);
static void CALLBACK Start(BASS_CHANNELINFO *info, float sVolume, BOOL fFloat);
static void CALLBACK End(void);
static void CALLBACK Play(void);
static void CALLBACK Pause(void);
static void CALLBACK Stop(void);
static void CALLBACK SetVolume(float sVolume);
static void CALLBACK FadeIn(float sVolume, DWORD dwTime);
static void CALLBACK FadeOut(DWORD dwTime);
static BOOL CALLBACK IsSupportFloatOutput(void);

static OUTPUT_PLUGIN_INFO opinfo = {
	OPDK_VER,
	GetDeviceCount,
	GetDefaultDeviceID,
	GetDeviceID,
	GetDeviceNameA,
	Init,
	GetStatus,
	Start,
	End,
	Play,
	Pause,
	Stop,
	SetVolume,
	FadeIn,
	FadeOut,
	IsSupportFloatOutput
};

#ifdef __cplusplus
extern "C"
#endif
OUTPUT_PLUGIN_INFO * CALLBACK GetOPluginInfo(void){
	return &opinfo;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

static int CALLBACK GetDeviceCount(void){
	BASS_ASIO_DEVICEINFO info;
	int i = 0;
	while (BASS_ASIO_GetDeviceInfo(i, &info)) i++;
	return i > 0xFFF ? 0xFFF : i;
}
static DWORD CALLBACK GetDefaultDeviceID(void){
	return OUTPUT_PLUGIN_ID_BASE + 0;
}
static DWORD CALLBACK GetDeviceID(int nIndex){
	return OUTPUT_PLUGIN_ID_BASE + (nIndex & 0xFFF);
}
static BOOL CALLBACK GetDeviceNameA(DWORD dwID, LPSTR szBuf, int nBufSize){
	if ((dwID & 0xF0000) == OUTPUT_PLUGIN_ID_BASE) {
		BASS_ASIO_DEVICEINFO info;
		if (BASS_ASIO_GetDeviceInfo(dwID & 0xFFF, &info)) {
			lstrcpynA(szBuf, "ASIO : ", nBufSize);
			if (nBufSize > 7) lstrcpynA(szBuf + 7, info.name, nBufSize - 7);
			return TRUE;
		}
	}
	return FALSE;
}
static int CALLBACK Init(DWORD dwID){
	return 0;
}


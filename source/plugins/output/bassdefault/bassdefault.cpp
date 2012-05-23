#define WINVER		0x0400	// 98以降
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4以降
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/src/oplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(linker, "/EXPORT:GetOPluginInfo=_GetOPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#define BASSDEF(f) (WINAPI * f)
#define BASS_SAMPLE_8BITS		1
#define BASS_SAMPLE_FLOAT		256
#define BASS_DATA_FLOAT		0x40000000
#define BASS_STREAMPROC_END		0x80000000

#define BASS_DEVICE_DEFAULT		2

#define BASS_STREAM_DECODE		0x200000
#define BASS_MUSIC_DECODE		BASS_STREAM_DECODE

#define BASS_ACTIVE_STOPPED	0
#define BASS_ACTIVE_PLAYING	1
#define BASS_ACTIVE_STALLED	2
#define BASS_ACTIVE_PAUSED	3

#define BASS_ATTRIB_VOL				2

typedef struct {
	DWORD freq;
	DWORD chans;
	DWORD flags;
	DWORD ctype;
	DWORD origres;
	DWORD plugin;
	DWORD sample;
	const char *filename;
} BASS_CHANNELINFO;

typedef struct {
	const char *name;
	const char *driver;
	DWORD flags;
} BASS_DEVICEINFO;

typedef DWORD (CALLBACK STREAMPROC)(DWORD handle, void *buffer, DWORD length, void *user);

DWORD BASSDEF(BASS_ChannelGetData)(DWORD handle, void *buffer, DWORD length);
BOOL BASSDEF(BASS_ChannelGetInfo)(DWORD handle, BASS_CHANNELINFO *info);
DWORD BASSDEF(BASS_ChannelIsActive)(DWORD handle);
BOOL BASSDEF(BASS_ChannelIsSliding)(DWORD handle, DWORD attrib);
BOOL BASSDEF(BASS_ChannelPause)(DWORD handle);
BOOL BASSDEF(BASS_ChannelPlay)(DWORD handle, BOOL restart);
BOOL BASSDEF(BASS_ChannelSetAttribute)(DWORD handle, DWORD attrib, float value);
BOOL BASSDEF(BASS_ChannelSlideAttribute)(DWORD handle, DWORD attrib, float value, DWORD time);
BOOL BASSDEF(BASS_ChannelStop)(DWORD handle);
BOOL BASSDEF(BASS_GetDeviceInfo)(DWORD device, BASS_DEVICEINFO *info);
DWORD BASSDEF(BASS_StreamCreate)(DWORD freq, DWORD chans, DWORD flags, STREAMPROC *proc, void *user);
BOOL BASSDEF(BASS_StreamFree)(DWORD handle);

#define FUNC_PREFIXA "BASS_"
static const CHAR szDllNameA[] = "bass.dll";
static const struct IMPORT_FUNC_TABLE {
	LPSTR lpszFuncName;
	FARPROC * ppFunc;
} functbl[] = {
	{ "ChannelGetData", (FARPROC *)&BASS_ChannelGetData },
	{ "ChannelGetInfo", (FARPROC *)&BASS_ChannelGetInfo },
	{ "ChannelIsActive", (FARPROC *)&BASS_ChannelIsActive },
	{ "ChannelIsSliding", (FARPROC *)&BASS_ChannelIsSliding },
	{ "ChannelPause", (FARPROC *)&BASS_ChannelPause },
	{ "ChannelPlay", (FARPROC *)&BASS_ChannelPlay },
	{ "ChannelSetAttribute", (FARPROC *)&BASS_ChannelSetAttribute },
	{ "ChannelSlideAttribute", (FARPROC *)&BASS_ChannelSlideAttribute },
	{ "ChannelStop", (FARPROC *)&BASS_ChannelStop },
	{ "GetDeviceInfo", (FARPROC *)&BASS_GetDeviceInfo },
	{ "StreamCreate", (FARPROC *)&BASS_StreamCreate },
	{ "StreamFree", (FARPROC *)&BASS_StreamFree },
	{ 0, (FARPROC *)0 }
};

static HMODULE m_hDLL = 0;

static HMODULE m_hBASS = 0;

#include "../../../fittle/src/wastr.h"
#include "../../../fittle/src/wastr.cpp"

#define OUTPUT_PLUGIN_ID_BASE (0x1ffff)

static int CALLBACK GetDeviceCount(void);
static DWORD CALLBACK GetDefaultDeviceID(void);
static DWORD CALLBACK GetDeviceID(int nIndex);
static BOOL CALLBACK GetDeviceNameA(DWORD dwID, LPSTR szBuf, int nBufSize);
static int CALLBACK Init(DWORD dwID);
static void CALLBACK Term(void);
static BOOL CALLBACK Setup(HWND hWnd);
static int CALLBACK GetStatus(void);
static void CALLBACK Start(void *pchinfo, float sVolume, BOOL fFloat);
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
	Term,
	Setup,
	GetStatus,
	Start,
	End,
	Play,
	Pause,
	Stop,
	SetVolume,
	FadeIn,
	FadeOut,
	IsSupportFloatOutput,
	0,0,0,0
};


BOOL LoadBASS(void){
	const struct IMPORT_FUNC_TABLE *pft;
	CHAR funcname[32];
	WASTR szPath;
	int l;
	if (m_hBASS) return TRUE;

	WAGetModuleParentDir(NULL, &szPath);
	WAstrcatA(&szPath, "Plugins\\BASS\\");
	WAstrcatA(&szPath, szDllNameA);
	m_hBASS = WALoadLibrary(&szPath);
	if(!m_hBASS) return FALSE;

	lstrcpynA(funcname, FUNC_PREFIXA, 32);
	l = lstrlenA(funcname);

	for (pft = functbl; pft->lpszFuncName; pft++){
		lstrcpynA(funcname + l, pft->lpszFuncName, 32 - l);
		FARPROC fp = GetProcAddress(m_hBASS, funcname);
		if (!fp){
			FreeLibrary(m_hBASS);
			m_hBASS = NULL;
			return FALSE;
		}
		*pft->ppFunc = fp;
	}
	return TRUE;
}
	
#ifdef __cplusplus
extern "C"
#endif
OUTPUT_PLUGIN_INFO * CALLBACK GetOPluginInfo(void){
	WAIsUnicode();
	if (!LoadBASS()) return NULL;
	return &opinfo;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

static int CALLBACK GetDeviceCount(void){
	return 1;
}
static DWORD CALLBACK GetDefaultDeviceID(void){
	return OUTPUT_PLUGIN_ID_BASE;
}
static DWORD CALLBACK GetDeviceID(int nIndex){
	return OUTPUT_PLUGIN_ID_BASE;
}
static BOOL CALLBACK GetDeviceNameA(DWORD dwID, LPSTR szBuf, int nBufSize){
	if ((dwID & 0xFFFFF) == OUTPUT_PLUGIN_ID_BASE) {
		lstrcpynA(szBuf, "BASS : (Default)", nBufSize);
		return TRUE;
	}
	return FALSE;
}
static int CALLBACK Init(DWORD dwID){
	return -1;
}

static void CALLBACK Term(void){
}

static BOOL CALLBACK Setup(HWND hWnd){
	return FALSE;
}

/*
	-------------------------------------------------------------------------------------------------------------------------
*/

static DWORD m_hChanOut = NULL;	// ストリームハンドル

static int CALLBACK GetStatus(void){
	if (m_hChanOut) {
		int nActive = BASS_ChannelIsActive(m_hChanOut);
		switch (nActive) {
		case BASS_ACTIVE_PAUSED:
			return OUTPUT_PLUGIN_STATUS_PAUSE;
		case BASS_ACTIVE_PLAYING:
		case BASS_ACTIVE_STALLED:
			return OUTPUT_PLUGIN_STATUS_PLAY;
		}
	}
	return OUTPUT_PLUGIN_STATUS_STOP;
}

static void CheckStartDecodeStream(DWORD hChan) {
	static DWORD hOldChan = 0;
	static DWORD dwResult;
	if (hOldChan != hChan) {
		hOldChan = hChan;
//		SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_START_DECODE_STREAM, (LPARAM)hChan);
		SendMessageTimeout(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_START_DECODE_STREAM, (LPARAM)hChan, SMTO_ABORTIFHUNG, 100, &dwResult);
	}
}

// メインストリームプロシージャ
static DWORD CALLBACK MainStreamProc(DWORD handle, void *buf, DWORD len, void *user){
	DWORD r;
	BASS_CHANNELINFO bci;
	float sAmp = 0;
	DWORD hChan = opinfo.GetDecodeChannel(&sAmp);
	CheckStartDecodeStream(hChan);
	if(BASS_ChannelGetInfo(handle ,&bci) && BASS_ChannelIsActive(hChan)){
		BOOL fFloat = bci.flags & BASS_SAMPLE_FLOAT;
		r = BASS_ChannelGetData(hChan, buf, fFloat ? len | BASS_DATA_FLOAT : len);
		if (r == (DWORD)-1) r = 0;
		if (sAmp >= 0 && r > 0 && r < BASS_STREAMPROC_END) {
			DWORD i;
			if (BASS_ChannelGetInfo(hChan ,&bci)) {
				if (fFloat || (bci.flags & BASS_SAMPLE_FLOAT)){
					float *p = (float *)buf;
					float v = sAmp;
					DWORD l = r >> 2;
					for (i = 0; i < l; i++){
						p[i] *= v;
					}
				}else if (bci.flags & BASS_SAMPLE_8BITS){
					unsigned char *p = (unsigned char *)buf;
					int v = (int)(sAmp * 256);
					DWORD l = r;
					int s;
					for (i = 0; i < l; i++){
						s = ((((int)p[i]) - 128) * v) >> 8;
						if (s < -128)
							s = -128;
						else if (s > 127)
							s = 127;
						p[i] = (unsigned char)(s + 128);
					}
				}else{
					signed short *p = (signed short *)buf;
					int v = (int)(sAmp * 256);
					DWORD l = r >> 1;
					int s;
					for (i = 0; i < l; i++){
						s = (((int)p[i]) * v) >> 8;
						if (s < -32768)
							s = -32768;
						else if (s > 32767)
							s = 32767;
						p[i] = (signed short)s;
					}
				}
			}
		}
		if(opinfo.IsEndCue() || !BASS_ChannelIsActive(hChan)){
			opinfo.PlayNext(opinfo.hWnd);
		}
	}else{
		opinfo.PlayNext(opinfo.hWnd);
		r = 0;
		/* r = BASS_STREAMPROC_END; */
	}
	return r;
}

// メインストリームを作成
static void CALLBACK Start(void *pchinfo, float sVolume, BOOL fFloat){
	BASS_CHANNELINFO *info = (BASS_CHANNELINFO *)pchinfo;
	DWORD dwFlag = info->flags & ~(BASS_STREAM_DECODE | BASS_MUSIC_DECODE);
	if (fFloat) dwFlag = (dwFlag & ~BASS_SAMPLE_8BITS) | BASS_SAMPLE_FLOAT;

	End();

	m_hChanOut = BASS_StreamCreate(info->freq, info->chans, dwFlag, &MainStreamProc, 0);

	if (m_hChanOut){
		SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_CREATE_STREAM, (LPARAM)m_hChanOut);
	}

	SetVolume(sVolume);
	Play();
}

static void CALLBACK End(void){
	if (m_hChanOut) {
		SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_FREE_STREAM, (LPARAM)m_hChanOut);
		BASS_StreamFree(m_hChanOut);
		m_hChanOut = 0;
	}
}

static void CALLBACK Play(void){
	if (m_hChanOut) {
		BASS_ChannelPlay(m_hChanOut, FALSE);
	}
}

static void CALLBACK Pause(void){
	if (m_hChanOut) {
		BASS_ChannelPause(m_hChanOut);
	}
}

static void CALLBACK Stop(void){
	if (m_hChanOut) {
		BASS_ChannelStop(m_hChanOut);
	}
}

static void CALLBACK SetVolume(float sVolume){
	if (m_hChanOut) {
		BASS_ChannelSetAttribute(m_hChanOut, BASS_ATTRIB_VOL, sVolume);
	}
}
static void CALLBACK FadeIn(float sVolume, DWORD dwTime){
	if (m_hChanOut) {
		BASS_ChannelSlideAttribute(m_hChanOut, BASS_ATTRIB_VOL, sVolume, dwTime);
		while (BASS_ChannelIsSliding(m_hChanOut, BASS_ATTRIB_VOL)) Sleep(1);
	}
}
static void CALLBACK FadeOut(DWORD dwTime){
	if (m_hChanOut) {
		BASS_ChannelSlideAttribute(m_hChanOut, BASS_ATTRIB_VOL, 0.0f, dwTime);
		while (BASS_ChannelIsSliding(m_hChanOut, BASS_ATTRIB_VOL)) Sleep(1);
	}
}

// 音声出力デバイスが浮動小数点出力をサポートしているか調べる
static BOOL CALLBACK IsSupportFloatOutput(void){
	DWORD floatable = BASS_StreamCreate(44100, 1, BASS_SAMPLE_FLOAT, NULL, 0); // try creating FP stream
	if (floatable){
		BASS_StreamFree(floatable); // floating-point channels are supported!
		return TRUE;
	}
	return FALSE;
}


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
#define BASS_DATA_FLOAT		0x40000000

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

DWORD BASSDEF(BASS_ChannelGetData)(DWORD handle, void *buffer, DWORD length);
DWORD BASSDEF(BASS_ChannelIsActive)(DWORD handle);

#define BASSWASAPIDEF(f) (WINAPI * f)

typedef struct {
	const char *name;
	const char *id;
	DWORD type;
	DWORD flags;
	float minperiod;
	float defperiod;
	DWORD mixfreq;
	DWORD mixchans;
} BASS_WASAPI_DEVICEINFO;

typedef struct {
	DWORD initflags;
	DWORD freq;
	DWORD chans;
	DWORD format;
	DWORD buflen;
	float volmax;
	float volmin;
	float volstep;
} BASS_WASAPI_INFO;

#define BASS_DEVICE_ENABLED		1
#define BASS_DEVICE_DEFAULT		2
#define BASS_DEVICE_INIT		4
#define BASS_DEVICE_LOOPBACK	8
#define BASS_DEVICE_INPUT		16
#define BASS_DEVICE_UNPLUGGED	32
#define BASS_DEVICE_DISABLED	64

#define BASS_WASAPI_EXCLUSIVE	1
#define BASS_WASAPI_AUTOFORMAT	2
#define BASS_WASAPI_BUFFER		4
#define BASS_WASAPI_SESSIONVOL	8

#define BASS_WASAPI_FORMAT_FLOAT	0
#define BASS_WASAPI_FORMAT_8BIT		1
#define BASS_WASAPI_FORMAT_16BIT	2
#define BASS_WASAPI_FORMAT_24BIT	3
#define BASS_WASAPI_FORMAT_32BIT	4

typedef DWORD (CALLBACK WASAPIPROC)(void *buffer, DWORD length, void *user);

BOOL BASSWASAPIDEF(BASS_WASAPI_GetDeviceInfo)(DWORD device, BASS_WASAPI_DEVICEINFO *info);
BOOL BASSWASAPIDEF(BASS_WASAPI_Init)(int device, DWORD freq, DWORD chans, DWORD flags, float buffer, float period, WASAPIPROC *proc, void *user);
BOOL BASSWASAPIDEF(BASS_WASAPI_Free)();
BOOL BASSWASAPIDEF(BASS_WASAPI_Start)();
BOOL BASSWASAPIDEF(BASS_WASAPI_Stop)(BOOL reset);
BOOL BASSWASAPIDEF(BASS_WASAPI_IsStarted)();
BOOL BASSWASAPIDEF(BASS_WASAPI_SetVolume)(BOOL linear, float volume);


static LPCSTR szFuncBase[2] = { "BASS_", "BASS_WASAPI_" };
static LPCSTR szDllNameA[2] = { "bass.dll", "basswasapi.dll" };
typedef struct IMPORT_FUNC_TABLE {
	LPCSTR lpszFuncName;
	FARPROC * ppFunc;
} IMPORT_FUNC_TABLE, *LPIMPORT_FUNC_TABLE;
typedef const IMPORT_FUNC_TABLE *LPCIMPORT_FUNC_TABLE;
static const IMPORT_FUNC_TABLE functbl0[] = {
	{ "ChannelGetData", (FARPROC *)&BASS_ChannelGetData },
	{ "ChannelIsActive", (FARPROC *)&BASS_ChannelIsActive },
	{ 0, (FARPROC *)0 }
};
static const IMPORT_FUNC_TABLE functbl1[] = {
	{ "GetDeviceInfo", (FARPROC *)&BASS_WASAPI_GetDeviceInfo },
	{ "Init", (FARPROC *)&BASS_WASAPI_Init },
	{ "Free", (FARPROC *)&BASS_WASAPI_Free },
	{ "Start", (FARPROC *)&BASS_WASAPI_Start },
	{ "Stop", (FARPROC *)&BASS_WASAPI_Stop },
	{ "IsStarted", (FARPROC *)&BASS_WASAPI_IsStarted },
	{ "SetVolume", (FARPROC *)&BASS_WASAPI_SetVolume },
	{ 0, (FARPROC *)0 }
};
static const LPCIMPORT_FUNC_TABLE functbl[2] = { functbl0, functbl1 };


static HMODULE m_hDLL = 0;
static int m_nDevice = 0;

#include "../../../fittle/src/wastr.h"
#include "../../../fittle/src/wastr.cpp"

static int m_nChShift = 0;

#define OUTPUT_PLUGIN_ID_BASE (0x40000)

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

static void LoadConfig(void){
	WASetIniFile(NULL, "Fittle.ini");
}

static BOOL LoadBASSWASAPI(void){
	int i;
	HMODULE h[2];
	for (i = 0; i < 2; i++){
		LPCIMPORT_FUNC_TABLE pft;
		CHAR funcname[32];
		WASTR szPath;
		int l;

		WAGetModuleParentDir(NULL, &szPath);
		WAstrcatA(&szPath, "Plugins\\BASS\\");
		WAstrcatA(&szPath, szDllNameA[i]);
		h[i] = WALoadLibrary(&szPath);
		if(!h[i])
		{
			if (i > 0) FreeLibrary(h[0]);
			return FALSE;
		}

		lstrcpynA(funcname, szFuncBase[i], 32);
		l = lstrlenA(funcname);

		for (pft = functbl[i]; pft->lpszFuncName; pft++){
			lstrcpynA(funcname + l, pft->lpszFuncName, 32 - l);
			FARPROC fp = GetProcAddress(h[i], funcname);
			if (!fp){
				if (i > 0) FreeLibrary(h[1]);
				FreeLibrary(h[0]);
				return FALSE;
			}
			*pft->ppFunc = fp;
		}
	}
	LoadConfig();
	return TRUE;
}

#ifdef __cplusplus
extern "C"
#endif
OUTPUT_PLUGIN_INFO * CALLBACK GetOPluginInfo(void){
	WAIsUnicode();
	return LoadBASSWASAPI() ? &opinfo : 0;
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
	int c = 0;
	BASS_WASAPI_DEVICEINFO info;
	for (int i = 0; BASS_WASAPI_GetDeviceInfo(i, &info); i++){
		if (!(info.flags & BASS_DEVICE_INPUT))
		{
			c += 2;
		}
	}
	return c > 0xFFF ? 0xFFF : c;
}
static DWORD CALLBACK GetDefaultDeviceID(void){
	int c = 0;
	BASS_WASAPI_DEVICEINFO info;
	for (int i = 0; BASS_WASAPI_GetDeviceInfo(i, &info); i++){
		if (!(info.flags & BASS_DEVICE_INPUT))
		{
			if ((info.flags & BASS_DEVICE_DEFAULT) && i <= 0xFFF)
			{
				return OUTPUT_PLUGIN_ID_BASE + (i & 0xFFF);
			}
			c += 2;
		}
	}
	return OUTPUT_PLUGIN_ID_BASE + 0;
}
static DWORD CALLBACK GetDeviceID(int nIndex){
	int c = 0;
	BASS_WASAPI_DEVICEINFO info;
	for (int i = 0; BASS_WASAPI_GetDeviceInfo(i, &info); i++){
		if (!(info.flags & BASS_DEVICE_INPUT))
		{
			if ((nIndex & 0xFFE) == c)
			{
				if (nIndex & 1)
					return OUTPUT_PLUGIN_ID_BASE + (i & 0xFFF) + 0x8000;
				else
					return OUTPUT_PLUGIN_ID_BASE + (i & 0xFFF);
			}
			c += 2;
		}
	}
	return OUTPUT_PLUGIN_ID_BASE + 0;
}
static BOOL CALLBACK GetDeviceNameA(DWORD dwID, LPSTR szBuf, int nBufSize){
	if ((dwID & 0xF0000) == OUTPUT_PLUGIN_ID_BASE) {
		BASS_WASAPI_DEVICEINFO info;
		if (BASS_WASAPI_GetDeviceInfo(dwID & 0xFFF, &info)) {
			if (dwID & 0x8000)
				lstrcpynA(szBuf, "WASAPI(E) : ", nBufSize);
			else
				lstrcpynA(szBuf, "WASAPI(S) : ", nBufSize);
			if (nBufSize > 12) lstrcpynA(szBuf + 12, info.name, nBufSize - 12);
			return TRUE;
		}
	}
	return FALSE;
}
static int CALLBACK Init(DWORD dwID){
	m_nDevice = dwID & 0x8FFF;
	return 0;
}

static void CALLBACK Term(void){
}

static BOOL CALLBACK Setup(HWND hWnd){
	return FALSE;
}


/*
	-------------------------------------------------------------------------------------------------------------------------
*/

static int m_nChanOut = -1;
static int m_nChanNum = 0;
static int m_nChanFreq = 44100;
static int m_nPause = 0;
static float m_sVolume = 1; 

static int CALLBACK GetStatus(void){
	if (m_nPause) return OUTPUT_PLUGIN_STATUS_PAUSE;
	if (BASS_WASAPI_IsStarted())
	{
		return OUTPUT_PLUGIN_STATUS_PLAY;
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

static DWORD CALLBACK WasapiProc(void *buffer, DWORD length, void *user)
{
	DWORD r = 0;
	BASS_CHANNELINFO bci;
	float sAmp;
	DWORD hChan = opinfo.GetDecodeChannel(&sAmp);
	if (BASS_ChannelIsActive(hChan)){
		CheckStartDecodeStream(hChan);
		r = BASS_ChannelGetData(hChan, buffer, length | BASS_DATA_FLOAT);
		if (r == (DWORD)-1) r = 0;
		if(opinfo.IsEndCue() || !BASS_ChannelIsActive(hChan)){
			opinfo.PlayNext(opinfo.hWnd);
		}
	} else {
		opinfo.PlayNext(opinfo.hWnd);
	}
	return r;
}

static BOOL WASAPI_Init(int freq, int ch){
	if ((m_nDevice & 0x8000) && BASS_WASAPI_Init(m_nDevice & 0xFFF, freq, ch, BASS_WASAPI_EXCLUSIVE, 0, 0, WasapiProc, 0)) {
		return TRUE;
	}
	if (BASS_WASAPI_Init(m_nDevice & 0xFFF, freq, ch, BASS_WASAPI_SESSIONVOL, 0, 0, WasapiProc, 0)) {
		return TRUE;
	}
	return FALSE;
}

static void CALLBACK Start(void *pchinfo, float sVolume, BOOL fFloat){
	BASS_CHANNELINFO *info = (BASS_CHANNELINFO *)pchinfo;
	End();

	if (WASAPI_Init(info->freq, info->chans)) {
		m_nChanOut = 0;
		m_nChanNum = info->chans;
		m_nChanFreq = info->freq;
		if (m_nChanOut){
			SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_CREATE_STREAM, (LPARAM)0);
			SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_CREATE_ASIO_STREAM, (LPARAM)m_nChanOut);
		}
		SetVolume(sVolume);
		Play();
	}
}

static void CALLBACK End(void){
	if (m_nChanOut >= 0) {
		SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_FREE_ASIO_STREAM, (LPARAM)m_nChanOut);
		SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_FREE_STREAM, (LPARAM)0);
		BASS_WASAPI_Stop(FALSE);
		BASS_WASAPI_Free();
		m_nChanOut = -1;
	}
}


static void CALLBACK Play(void){
	if (m_nChanOut >= 0) {
		SetVolume(m_sVolume);
		BASS_WASAPI_Start();
		SetVolume(m_sVolume);
	} else if (m_nPause) {
		if (WASAPI_Init(m_nChanFreq, m_nChanNum)) {
			m_nChanOut = 0;
			SetVolume(m_sVolume);
			BASS_WASAPI_Start();
			SetVolume(m_sVolume);
		}
	}
	m_nPause = 0;
}

static void CALLBACK Pause(void){
	if (m_nChanOut >= 0) {
		if (GetStatus() == OUTPUT_PLUGIN_STATUS_PLAY)
		{
			m_nPause = 1; 
			BASS_WASAPI_Stop(FALSE);
			BASS_WASAPI_Free();
			m_nChanOut = -1;
		}
		//BASS_ASIO_ChannelPause(FALSE, m_nChanOut);
	}
}

static void CALLBACK Stop(void){
	if (m_nChanOut >= 0) {
		BASS_WASAPI_Stop(FALSE);
	}
	m_nPause = 0;
}

static void SetVolumeRaw(float sVolume){
	if (m_nChanOut >= 0) {
		BASS_WASAPI_SetVolume(TRUE, sVolume);
	}
}

static void CALLBACK SetVolume(float sVolume){
	m_sVolume = sVolume;
	if (m_nChanOut >= 0) {
		float sAmp;
		DWORD hChan = opinfo.GetDecodeChannel(&sAmp);
		if (sAmp >= 0)
			sAmp *= sVolume;
		else
			sAmp = sVolume;
		SetVolumeRaw(sAmp);
	}
}
static void CALLBACK FadeIn(float sVolume, DWORD dwTime){
	if (m_nChanOut >= 0 && dwTime) {
		float sAmp;
		DWORD hChan = opinfo.GetDecodeChannel(&sAmp);
		DWORD dwTimeOrg = GetTickCount();
		DWORD dwTimeNow = 0;
		if (sAmp >= 0)
			sAmp *= sVolume;
		else
			sAmp = sVolume;
		do {
			float sFader = ((float)dwTimeNow) / dwTime;
			if (sFader > 1) sFader = 1;
			SetVolumeRaw(sFader * sAmp);
			Sleep(1);
			dwTimeNow = GetTickCount() - dwTimeOrg;
		} while (dwTimeNow < dwTime);
	}
}
static void CALLBACK FadeOut(DWORD dwTime){
	if (m_nChanOut >= 0 && dwTime) {
		float sAmp;
		DWORD hChan = opinfo.GetDecodeChannel(&sAmp);
		DWORD dwTimeOrg = GetTickCount();
		DWORD dwTimeNow = 0;
		if (sAmp >= 0)
			sAmp *= m_sVolume;
		else
			sAmp = m_sVolume;
		do {
			float sFader = ((float)dwTimeNow) / dwTime;
			if (sFader > 1) sFader = 1;
			SetVolumeRaw((1 - sFader) * sAmp);
			Sleep(1);
			dwTimeNow = GetTickCount() - dwTimeOrg;
		} while (dwTimeNow < dwTime);
	}
	SetVolumeRaw(0);
}

// 音声出力デバイスが浮動小数点出力をサポートしているか調べる
static BOOL CALLBACK IsSupportFloatOutput(void){
	return TRUE;
}


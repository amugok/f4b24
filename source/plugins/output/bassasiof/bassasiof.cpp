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
#define BASS_ASIO_THREAD	1
#define BASS_ASIO_JOINORDER	2

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

#define BASSASIODEF(f) (WINAPI * f)

#define BASS_ASIO_FORMAT_FLOAT  19 // 32-bit floating-point
#define BASS_ASIO_RESET_PAUSE	4 // unpause channel
#define BASS_ASIO_ACTIVE_DISABLED	0
#define BASS_ASIO_ACTIVE_ENABLED	1
#define BASS_ASIO_ACTIVE_PAUSED		2
typedef struct {
	const char *name;	// description
	const char *driver;	// driver
} BASS_ASIO_DEVICEINFO;
typedef DWORD (CALLBACK ASIOPROC)(BOOL input, DWORD channel, void *buffer, DWORD length, void *user);

DWORD BASSASIODEF(BASS_ASIO_GetVersion)();
BOOL BASSASIODEF(BASS_ASIO_ChannelEnable)(BOOL input, DWORD channel, ASIOPROC *proc, void *user);
DWORD BASSASIODEF(BASS_ASIO_ChannelIsActive)(BOOL input, DWORD channel);
BOOL BASSASIODEF(BASS_ASIO_ChannelJoin)(BOOL input, DWORD channel, int channel2);
BOOL BASSASIODEF(BASS_ASIO_ChannelPause)(BOOL input, DWORD channel);
BOOL BASSASIODEF(BASS_ASIO_ChannelReset)(BOOL input, int channel, DWORD flags);
BOOL BASSASIODEF(BASS_ASIO_ChannelSetFormat)(BOOL input, DWORD channel, DWORD format);
BOOL BASSASIODEF(BASS_ASIO_ChannelSetRate)(BOOL input, DWORD channel, double rate);
BOOL BASSASIODEF(BASS_ASIO_ChannelSetVolume)(BOOL input, int channel, float volume);
BOOL BASSASIODEF(BASS_ASIO_ControlPanel)();
BOOL BASSASIODEF(BASS_ASIO_Free)();
BOOL BASSASIODEF(BASS_ASIO_GetDeviceInfo)(DWORD device, BASS_ASIO_DEVICEINFO *info);
BOOL BASSASIODEF(BASS_ASIO_Init)(DWORD device, DWORD flags);
BOOL BASSASIODEF(BASS_ASIO_SetRate)(double rate);
BOOL BASSASIODEF(BASS_ASIO_Start)(DWORD buflen);
BOOL BASSASIODEF(BASS_ASIO_Start_13)(DWORD buflen, DWORD threads);
BOOL BASSASIODEF(BASS_ASIO_Stop)();


static LPCSTR szFuncBase[2] = { "BASS_", "BASS_ASIO_" };
static LPCSTR szDllNameA[2] = { "bass.dll", "bassasio.dll" };
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
	{ "ChannelEnable", (FARPROC *)&BASS_ASIO_ChannelEnable },
	{ "ChannelIsActive", (FARPROC *)&BASS_ASIO_ChannelIsActive },
	{ "ChannelJoin", (FARPROC *)&BASS_ASIO_ChannelJoin },
	{ "ChannelPause", (FARPROC *)&BASS_ASIO_ChannelPause },
	{ "ChannelReset", (FARPROC *)&BASS_ASIO_ChannelReset },
	{ "ChannelSetFormat", (FARPROC *)&BASS_ASIO_ChannelSetFormat },
	{ "ChannelSetRate", (FARPROC *)&BASS_ASIO_ChannelSetRate },
	{ "ChannelSetVolume", (FARPROC *)&BASS_ASIO_ChannelSetVolume },
	{ "ControlPanel", (FARPROC *)&BASS_ASIO_ControlPanel },
	{ "Free", (FARPROC *)&BASS_ASIO_Free },
	{ "GetDeviceInfo", (FARPROC *)&BASS_ASIO_GetDeviceInfo },
	{ "GetVersion", (FARPROC *)&BASS_ASIO_GetVersion },
	{ "Init", (FARPROC *)&BASS_ASIO_Init },
	{ "SetRate", (FARPROC *)&BASS_ASIO_SetRate },
	{ "Start", (FARPROC *)&BASS_ASIO_Start },
	{ "Start", (FARPROC *)&BASS_ASIO_Start_13 },
	{ "Stop", (FARPROC *)&BASS_ASIO_Stop },
	{ 0, (FARPROC *)0 }
};
static const LPCIMPORT_FUNC_TABLE functbl[2] = { functbl0, functbl1 };


static HMODULE m_hDLL = 0;
static int m_nDevice = 0;

#include "../../../fittle/src/wastr.h"
#include "../../../fittle/src/wastr.cpp"

static int m_nChShift = 0;

#define OUTPUT_PLUGIN_ID_BASE (0x30000)

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
	m_nChShift = WAGetIniInt("BASSASIO", "ChannelShift", 0);
}

static BOOL LoadBASSASIO(void){
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
	return LoadBASSASIO() ? &opinfo : 0;
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
	int i = 0;
	BASS_ASIO_DEVICEINFO info;
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
			lstrcpynA(szBuf, "ASIO(F) : ", nBufSize);
			if (nBufSize > 10) lstrcpynA(szBuf + 10, info.name, nBufSize - 10);
			return TRUE;
		}
	}
	return FALSE;
}
static int CALLBACK Init(DWORD dwID){
	m_nDevice = dwID & 0xFFF;
	return 0;
}

static void CALLBACK Term(void){
}

static BOOL CALLBACK Setup(HWND hWnd){
	return BASS_ASIO_ControlPanel();
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
	if (m_nChanOut >= 0) {
		int nActive = BASS_ASIO_ChannelIsActive(FALSE, m_nChanOut);
		switch (nActive) {
		case BASS_ASIO_ACTIVE_PAUSED:
			return OUTPUT_PLUGIN_STATUS_PAUSE;
		case BASS_ASIO_ACTIVE_ENABLED:
			return OUTPUT_PLUGIN_STATUS_PLAY;
		case BASS_ASIO_ACTIVE_DISABLED:
			break;
		}
	}
	if (m_nPause) return OUTPUT_PLUGIN_STATUS_PAUSE;
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

static DWORD CALLBACK AsioProc(BOOL input, DWORD channel, void *buffer, DWORD length, void *user)
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

static void CALLBACK Start(void *pchinfo, float sVolume, BOOL fFloat){
	BASS_CHANNELINFO *info = (BASS_CHANNELINFO *)pchinfo;
	End();

	if (BASS_ASIO_Init(m_nDevice, 0)) {
		int i;
		int ch = m_nChShift;
		BASS_ASIO_ChannelEnable(FALSE, ch, &AsioProc, (void*)NULL);
		for (i = 1; i< info->chans ; i++)
			BASS_ASIO_ChannelJoin(FALSE, ch + i, ch);
		BASS_ASIO_ChannelSetFormat(FALSE, ch, BASS_ASIO_FORMAT_FLOAT);
		BASS_ASIO_SetRate(info->freq);
		BASS_ASIO_ChannelSetRate(FALSE, ch, info->freq);
		m_nChanOut = ch;
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
		BASS_ASIO_Stop();
		BASS_ASIO_Free();
		m_nChanOut = -1;
	}
}

static void ASIO_Start()
{
	if (BASS_ASIO_GetVersion() >= 0x103) {
		BASS_ASIO_Start_13(0, 0);
	} else {
		BASS_ASIO_Start(0);
	}
}

static void CALLBACK Play(void){
	if (m_nChanOut >= 0) {
		ASIO_Start();
	} else if (m_nPause) {
		if (BASS_ASIO_Init(m_nDevice, 0)) {
			int i;
			int ch = m_nChShift;
			BASS_ASIO_ChannelEnable(FALSE, ch, &AsioProc, (void*)NULL);
			for (i = 1; i< m_nChanNum ; i++)
				BASS_ASIO_ChannelJoin(FALSE, ch + i, ch);
			BASS_ASIO_ChannelSetFormat(FALSE, ch, BASS_ASIO_FORMAT_FLOAT);
			BASS_ASIO_SetRate(m_nChanFreq);
			BASS_ASIO_ChannelSetRate(FALSE, ch, m_nChanFreq);
			m_nChanOut = ch;
			SetVolume(m_sVolume);
			ASIO_Start();
		}
	}
	m_nPause = 0;
}

static void CALLBACK Pause(void){
	if (m_nChanOut >= 0) {
		if (GetStatus() == OUTPUT_PLUGIN_STATUS_PLAY)
		{
			m_nPause = 1; 
			BASS_ASIO_Stop();
			BASS_ASIO_Free();
			m_nChanOut = -1;
		}
		//BASS_ASIO_ChannelPause(FALSE, m_nChanOut);
	}
}

static void CALLBACK Stop(void){
	if (m_nChanOut >= 0) {
		BASS_ASIO_Stop();
	}
	m_nPause = 0;
}

static void SetVolumeRaw(float sVolume){
	if (m_nChanOut >= 0) {
		int i;
		for (i = 0; i < m_nChanNum; i++)
			BASS_ASIO_ChannelSetVolume(FALSE, m_nChanOut + i, sVolume);
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


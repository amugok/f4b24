#define WINVER		0x0400	// 98以降
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4以降
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "../../../../extra/bass24/bass.h"
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
#pragma comment(linker, "/EXPORT:GetOPluginInfo=_GetOPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif


static HMODULE hDLL = 0;

#define OUTPUT_PLUGIN_ID_BASE (0x10000)

static int CALLBACK GetDeviceCount(void);
static DWORD CALLBACK GetDefaultDeviceID(void);
static DWORD CALLBACK GetDeviceID(int nIndex);
static BOOL CALLBACK GetDeviceNameA(DWORD dwID, LPSTR szBuf, int nBufSize);
static int CALLBACK Init(DWORD dwID);
static void CALLBACK Term(void);
static int CALLBACK GetRate(void);
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
/*
	,GetRate
*/
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
	BASS_DEVICEINFO info;
	int i = 0;
	while (BASS_GetDeviceInfo(i, &info)) i++;
	return i > 0xFFF ? 0xFFF : i;
}
static DWORD CALLBACK GetDefaultDeviceID(void){
	BASS_DEVICEINFO info;
	int i = 0;
	while (BASS_GetDeviceInfo(i, &info) && i <= 0xFFF) {
		if (info.flags & BASS_DEVICE_DEFAULT)
			return OUTPUT_PLUGIN_ID_BASE + i;
		i++;
	}
	return OUTPUT_PLUGIN_ID_BASE + 1;
}
static DWORD CALLBACK GetDeviceID(int nIndex){
	return OUTPUT_PLUGIN_ID_BASE + (nIndex & 0xFFF);
}
static BOOL CALLBACK GetDeviceNameA(DWORD dwID, LPSTR szBuf, int nBufSize){
	if ((dwID & 0xF0000) == OUTPUT_PLUGIN_ID_BASE) {
		BASS_DEVICEINFO info;
		if (BASS_GetDeviceInfo(dwID & 0xFFF, &info)) {
			lstrcpynA(szBuf, "BASS : ", nBufSize);
			if (nBufSize > 7) lstrcpynA(szBuf + 7, info.name, nBufSize - 7);
			return TRUE;
		}
	}
	return FALSE;
}
static int CALLBACK Init(DWORD dwID){
	return dwID & 0xFFF;
}

static void CALLBACK Term(void){
}

static int CALLBACK GetRate(void){
	return 44100;
}

static BOOL CALLBACK Setup(HWND hWnd){
	return FALSE;
}


/*
	-------------------------------------------------------------------------------------------------------------------------
*/

static HSTREAM m_hChanOut = NULL;	// ストリームハンドル

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

// メインストリームプロシージャ
static DWORD CALLBACK MainStreamProc(HSTREAM handle, void *buf, DWORD len, void *user){
	DWORD r;
	BASS_CHANNELINFO bci;
	float sAmp = 0;
	DWORD hChan = opinfo.GetDecodeChannel(&sAmp);
	if(BASS_ChannelGetInfo(handle ,&bci) && BASS_ChannelIsActive(hChan)){
		BOOL fFloat = bci.flags & BASS_SAMPLE_FLOAT;
		r = BASS_ChannelGetData(hChan, buf, fFloat ? len | BASS_DATA_FLOAT : len);
		if (r == (DWORD)-1) r = 0;
		if (sAmp > 0 && r > 0 && r < BASS_STREAMPROC_END) {
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
		r = BASS_STREAMPROC_END;
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
		if (!BASS_ChannelPlay(m_hChanOut, FALSE)) {
			int nErr = BASS_ErrorGetCode();
		}
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


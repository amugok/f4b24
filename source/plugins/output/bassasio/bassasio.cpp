#define WINVER		0x0400	// 98以降
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4以降
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


static HMODULE m_hDLL = 0;
static int m_nDevice = 0;

#define OUTPUT_PLUGIN_ID_BASE (0x20000)

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
			lstrcpynA(szBuf, "ASIO : ", nBufSize);
			if (nBufSize > 7) lstrcpynA(szBuf + 7, info.name, nBufSize - 7);
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

static int CALLBACK GetRate(void){
	if (BASS_ASIO_Init(m_nDevice)) {
		float freq = BASS_ASIO_GetRate();
		BASS_ASIO_Free();
		if (freq > 0) return freq;
	}
	return 44100;
}

static BOOL CALLBACK Setup(HWND hWnd){
	return BASS_ASIO_ControlPanel();
}


/*
	-------------------------------------------------------------------------------------------------------------------------
*/

static int m_nChanOut = -1;
static int m_nChanNum = 0;
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
	return OUTPUT_PLUGIN_STATUS_STOP;
}

static DWORD CALLBACK AsioProc(BOOL input, DWORD channel, void *buffer, DWORD length, void *user)
{
	DWORD r = 0;
	BASS_CHANNELINFO bci;
	float sAmp;
	DWORD hChan = opinfo.GetDecodeChannel(&sAmp);
	if (BASS_ChannelIsActive(hChan)){
		r = BASS_ChannelGetData(hChan, buffer, length | BASS_DATA_FLOAT);
		if (r == (DWORD)-1) r = 0;
		if(opinfo.IsEndCue() || !BASS_ChannelIsActive(hChan)){
			opinfo.PlayNext(opinfo.hWnd);
		}
	}
	return r;
}

static void CALLBACK Start(void *pchinfo, float sVolume, BOOL fFloat){
	BASS_CHANNELINFO *info = (BASS_CHANNELINFO *)pchinfo;
	End();

	if (BASS_ASIO_Init(m_nDevice)) {
		int ch = 0;
		int i;
		BASS_ASIO_ChannelEnable(FALSE, ch, &AsioProc, (void*)NULL);
		for (i = 1; i< info->chans ; i++)
			BASS_ASIO_ChannelJoin(FALSE, ch + i, ch);
		BASS_ASIO_ChannelSetFormat(FALSE, ch, BASS_ASIO_FORMAT_FLOAT);
		BASS_ASIO_ChannelSetRate(FALSE, ch, info->freq);
		BASS_ASIO_SetRate(info->freq);
		m_nChanOut = ch;
		m_nChanNum = info->chans;
		if (m_nChanOut){
			SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_CREATE_STREAM, (LPARAM)m_nChanOut);
		}
		SetVolume(sVolume);
		Play();
	}
}

static void CALLBACK End(void){
	if (m_nChanOut >= 0) {
		SendMessage(opinfo.hWnd, WM_F4B24_IPC, WM_F4B24_HOOK_FREE_STREAM, (LPARAM)m_nChanOut);
		BASS_ASIO_Stop();
		BASS_ASIO_Free();
		m_nChanOut = -1;
	}
}


static void CALLBACK Play(void){
	if (m_nChanOut >= 0) {
		if (GetStatus() == OUTPUT_PLUGIN_STATUS_PAUSE){
			BASS_ASIO_ChannelReset(FALSE, m_nChanOut, BASS_ASIO_RESET_PAUSE);
		}
		else
		{
			BASS_ASIO_Start(0);
		}
	}
}

static void CALLBACK Pause(void){
	if (m_nChanOut >= 0) {
		BASS_ASIO_ChannelPause(FALSE, m_nChanOut);
	}
}

static void CALLBACK Stop(void){
	if (m_nChanOut >= 0) {
		BASS_ASIO_Stop();
	}
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
		if (sAmp > 0)
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
		if (sAmp > 0)
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
		if (sAmp > 0)
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


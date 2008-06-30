/*
 * replaygain.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../../extra/bass24/bass.h"
#include <shlwapi.h>
#include <math.h>
#include "../../../fittle/src/bass_tag.h"
#include "../../../fittle/src/readtag.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"../../../../extra/bass24/bass.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
//#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

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

typedef struct GAININFO {
	float album_gain;
	float album_peak;
	float track_gain;
	float track_peak;
	BOOL has_album_gain;
	BOOL has_album_peak;
	BOOL has_track_gain;
	BOOL has_track_peak;
} GAININFO, *LPGAININFO;

static int XATOI(LPCTSTR p){
	int r = 0;
	while (*p == TEXT(' ') || *p == TEXT('\t')) p++;
	while (*p >= TEXT('0') && *p <= TEXT('9')){
		r = r * 10 + (*p - TEXT('0'));
		p++;
	}
	return r;
}

static float XATOF(LPCTSTR p){
	float s = 1;
	float f = 0;
	while (*p == TEXT(' ') || *p == TEXT('\t')) p++;
	if (*p == '-') {
		s = -1;
		p++;
	}
	while (*p >= TEXT('0') && *p <= TEXT('9')){
		f = f * 10 + (*p - TEXT('0'));
		p++;
	}
	if (*p == '.'){
		float b = 1;
		p++;
		while (*p >= TEXT('0') && *p <= TEXT('9')){
			b *= 10;
			f += (*p - TEXT('0')) / b;
			p++;
		}
	}
	return f * s;
}

static void ReadGainLine(LPBYTE p, LPGAININFO pgi){
	TCHAR szBuf[256];
	if (Vorbis_ReadFrame(p, "replaygain_album_peak=", szBuf, 256)){
		pgi->has_album_peak = TRUE;
		pgi->album_peak = XATOF(szBuf);
	} else if (Vorbis_ReadFrame(p, "replaygain_album_gain=", szBuf, 256)){
		pgi->has_album_gain = TRUE;
		pgi->album_gain = XATOF(szBuf);
	} else if (Vorbis_ReadFrame(p, "replaygain_track_peak=", szBuf, 256)){
		pgi->has_track_peak = TRUE;
		pgi->track_peak = XATOF(szBuf);
	} else if (Vorbis_ReadFrame(p, "replaygain_track_gain=", szBuf, 256)){
		pgi->has_track_gain = TRUE;
		pgi->track_gain = XATOF(szBuf);
	}
}

static void ReadGainID3Ext(LPTSTR pExt, LPGAININFO pgi){
	LPTSTR pValue = pExt + lstrlen(pExt) + 1;
	if (!lstrcmp(pExt, TEXT("replaygain_album_peak"))){
		pgi->has_album_peak = TRUE;
		pgi->album_peak = XATOF(pValue);
	} else if (!lstrcmp(pExt, TEXT("replaygain_album_gain"))){
		pgi->has_album_gain = TRUE;
		pgi->album_gain = XATOF(pValue);
	} else if (!lstrcmp(pExt, TEXT("replaygain_track_peak"))){
		pgi->has_track_peak = TRUE;
		pgi->track_peak = XATOF(pValue);
	} else if (!lstrcmp(pExt, TEXT("replaygain_track_gain"))){
		pgi->has_track_gain = TRUE;
		pgi->track_gain = XATOF(pValue);
	}
}

static BOOL ID3V2_ReadGain(DWORD handle, LPGAININFO pgi){
	TCHAR szExt[256];
	LPBYTE p = (LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_ID3V2);
	if(p && p[0] == 'I' && p[1] == 'D' && p[2] == '3'){
		char szFrameID[5];
		unsigned nFrameSize;
		unsigned nTotal = 0;	// ヘッダサイズを足しておく
		unsigned nTagSize = GetSyncSafeInt(p + 6);
		unsigned nVersion = *(p + 3);
		unsigned nFlag = *(p + 5);
		LPBYTE pUnsync = NULL;
		if (nFlag & 0x80)
			p = pUnsync = Unsync(p + 10, 0, nTagSize, &nTagSize);
		else
			p += 10;
		if (!p) return FALSE;

		// フレームを前から順に取得
		if(nVersion >=3){	// バージョンの取得
			if (nFlag & 0x40) {
				/* 拡張ヘッダ */
				nTotal += (nVersion == 3) ? (4 + GetNonSyncSafeInt23(p + nTotal)) : GetSyncSafeInt(p + nTotal);
			}
			while(nTotal<nTagSize){
				int nFrameFlag = 0;
				int nLenID;
				// Ver.2.3以上
				lstrcpynA(szFrameID, (LPCSTR)(p + nTotal), 5); // フレームIDの取得
				if (nVersion == 3)
					nFrameSize = GetNonSyncSafeInt23(p + nTotal + 4); // フレームサイズの取得
				else
					nFrameSize = GetSyncSafeInt(p + nTotal + 4); // フレームサイズの取得
				nLenID = lstrlenA(szFrameID);
				if (nLenID > 0 && nLenID < 4 && nTotal > 4){
					/* broken tag */
					lstrcpynA(szFrameID, (LPCSTR)(p + nTotal - 4 + nLenID), 5); // フレームIDの取得
					if (lstrlenA(szFrameID) == 4){
						nTotal += - 4 + nLenID;
						continue;
					}
				}
				if(nLenID !=4) break;
				if (nVersion == 4)
					nFrameFlag = p[nTotal+9] & 3;
				if(!lstrcmpA(szFrameID, "TXXX") && ID3V23_ReadFrame(nFrameFlag, p + nTotal + 10, nFrameSize, szExt, 256)){
					ReadGainID3Ext(szExt, pgi);
				}

				nTotal += nFrameSize + 10;
			}
		}else{
			while(nTotal<nTagSize){
				int nFrameFlag = 0;
				// Ver.2.2以下
				lstrcpynA(szFrameID, (LPCSTR)(p + nTotal), 4); // フレームIDの取得
				nFrameSize = GetNonSyncSafeInt22(p + nTotal + 3); // フレームサイズの取得
				if(lstrlenA(szFrameID)!=3) break;

				if(!lstrcmpA(szFrameID, "TXX") && ID3V23_ReadFrame(nFrameFlag, p + nTotal + 6, nFrameSize, szExt, 256)){
					ReadGainID3Ext(szExt, pgi);
				}

				nTotal += nFrameSize + 6;
			}
		}

		if (pUnsync) HFree(pUnsync);
	}
	return pgi->has_album_gain || pgi->has_album_peak || pgi->has_track_gain || pgi->has_track_peak;
}

static BOOL WMA_ReadGain(DWORD handle, LPGAININFO pgi){
	int i;
	TCHAR szBuf[256];
	LPBYTE p = (LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_WMA);
	while(p && *p){
		if (Vorbis_ReadFrame(p, "replaygain_album_peak=", szBuf, 256)){
			pgi->has_album_peak = TRUE;
			i = XATOF(szBuf);
			pgi->album_peak = *(float *)&i;
		} else if (Vorbis_ReadFrame(p, "replaygain_album_gain=", szBuf, 256)){
			pgi->has_album_gain = TRUE;
			i = XATOF(szBuf);
			pgi->album_gain = *(float *)&i;
		} else if (Vorbis_ReadFrame(p, "replaygain_track_peak=", szBuf, 256)){
			pgi->has_track_peak = TRUE;
			i = XATOF(szBuf);
			pgi->track_peak = *(float *)&i;
		} else if (Vorbis_ReadFrame(p, "replaygain_track_gain=", szBuf, 256)){
			pgi->has_track_gain = TRUE;
			i = XATOF(szBuf);
			pgi->track_gain = *(float *)&i;
		}
		p += lstrlenA((LPCSTR)p) + 1;
	}
	return pgi->has_album_gain || pgi->has_album_peak || pgi->has_track_gain || pgi->has_track_peak;
}

static BOOL ReadGainSub(LPBYTE p, LPGAININFO pgi){
	while(p && *p){
		ReadGainLine(p, pgi);
		p += lstrlenA((LPCSTR)p) + 1;
	}
	return pgi->has_album_gain || pgi->has_album_peak || pgi->has_track_gain || pgi->has_track_peak;
}

static BOOL ReadGain(LPGAININFO pgi, DWORD hBass){
	BASS_CHANNELINFO bcinfo = {0};

	pgi->album_gain = 0;
	pgi->album_peak = 1;
	pgi->track_gain = 0;
	pgi->track_peak = 1;
	pgi->has_album_gain = FALSE;
	pgi->has_album_peak = FALSE;
	pgi->has_track_gain = FALSE;
	pgi->has_track_peak = FALSE;

	BASS_ChannelGetInfo(hBass, &bcinfo);
	switch(bcinfo.ctype){
		case BASS_CTYPE_STREAM_AIFF:
		case BASS_CTYPE_STREAM_WAV:
		case BASS_CTYPE_STREAM_WAV_PCM:  
		case BASS_CTYPE_STREAM_WAV_FLOAT:  
		case BASS_CTYPE_STREAM_WAV_MP3: /* removed in BASS 2.4 */
			return FALSE;

		case BASS_CTYPE_STREAM_MP1:
		case BASS_CTYPE_STREAM_MP2:
		case BASS_CTYPE_STREAM_MP3:
		case BASS_CTYPE_STREAM_APE:
		case BASS_CTYPE_STREAM_MPC:
		case BASS_CTYPE_STREAM_TTA:
		case BASS_CTYPE_STREAM_OFR:
		case BASS_CTYPE_STREAM_WV:
		case BASS_CTYPE_STREAM_WV_H:
		case BASS_CTYPE_STREAM_WV_L:
		case BASS_CTYPE_STREAM_WV_LH:
		case BASS_CTYPE_STREAM_TAK:
			/* bass_ape.dllを入れても読んでくれない */
			if (ReadGainSub((LPBYTE)BASS_ChannelGetTags(hBass, BASS_TAG_APE), pgi)) return TRUE;
			return ID3V2_ReadGain(hBass, pgi);

		case BASS_CTYPE_STREAM_WMA:
		case BASS_CTYPE_STREAM_WMA_MP3:
			return WMA_ReadGain(hBass, pgi);

		case BASS_CTYPE_STREAM_OGG:	// Vorbis comment
		case BASS_CTYPE_STREAM_FLAC:
		case BASS_CTYPE_STREAM_FLAC_OGG:
		case BASS_CTYPE_STREAM_SPX:
			return ReadGainSub((LPBYTE)BASS_ChannelGetTags(hBass, BASS_TAG_OGG), pgi);

		case BASS_CTYPE_STREAM_MP4: // iTunes metadata
		case BASS_CTYPE_STREAM_ALAC:
		case BASS_CTYPE_STREAM_AAC:
			return ReadGainSub((LPBYTE)BASS_ChannelGetTags(hBass, BASS_TAG_MP4), pgi);

		case BASS_CTYPE_STREAM_AC3:
			return FALSE;

		case BASS_CTYPE_STREAM_CD:
			return FALSE;

		case BASS_CTYPE_STREAM:
			return FALSE;

	}
	return FALSE;
}

static BOOL GetGain(float *pGain, DWORD hBass){
	GAININFO gi;
	float volume = 1;

	int nGainMode = SendMessage(fpi.hParent, WM_F4B24_IPC, WM_F4B24_IPC_GET_REPLAYGAIN_MODE, 0);
	
	if (!ReadGain(&gi, hBass)) return FALSE;

	if (!gi.has_album_gain) gi.album_gain = gi.track_gain;
	if (!gi.has_track_gain) gi.track_gain = gi.album_gain;
	if (!gi.has_album_peak) gi.album_peak = gi.track_peak;
	if (!gi.has_track_peak) gi.track_peak = gi.album_peak;

	switch (nGainMode){
	case 1:
		volume = pow(10, gi.album_gain / 20);
		break;
	case 2:
		volume = 1 / gi.album_peak;
		break;
	case 3:
		volume = pow(10, gi.track_gain / 20);
		break;
	case 4:
		volume = 1 / gi.track_peak;
		break;
	}

	*pGain = volume;
	return TRUE;
}

// サブクラス化したウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_F4B24_IPC:
		if (wp == WM_F4B24_HOOK_REPLAY_GAIN){
			float sGain = 1;
			if (GetGain(&sGain, (DWORD)lp)){
				return (LRESULT)*(DWORD *)&sGain;
			}
		}
	}
	return (fIsUnicode ? CallWindowProcW : CallWindowProcA)(hOldProc, hWnd, msg, wp, lp);
}

/* 起動時に一度だけ呼ばれます */
static BOOL OnInit(){
	fIsUnicode = ((GetVersion() & 0x80000000) == 0) || IsWindowUnicode(fpi.hParent);
	hOldProc = (WNDPROC)(fIsUnicode ? SetWindowLongW : SetWindowLongA)(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);
	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
static void OnQuit(){
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


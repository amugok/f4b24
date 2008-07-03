#define APDK_VER 3
#include "../../fittle/src/aplugin.h"
#include "../../fittle/src/f4b24.h"
#include <shlwapi.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")
#ifdef UNICODE
#pragma comment(linker, "/EXPORT:GetAPluginInfoW=_GetAPluginInfoW@0")
#define GetAPluginInfo GetAPluginInfoW
#else
#pragma comment(linker, "/EXPORT:GetAPluginInfo=_GetAPluginInfo@0")
#endif
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
//#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#ifndef MAX_FITTLE_PATH
#define MAX_FITTLE_PATH 260*2
#endif

typedef enum {CUECHAR_DEFAULT,CUECHAR_UTF8,CUECHAR_UTF16LE,CUECHAR_UTF16BE,CUECHAR_NA} CueCharset;

static const BYTE UTF8BOM[] = {0xEF,0xBB,0xBF};
static const BYTE UTF16LEBOM[] = {0xFF,0xFE};
static const BYTE UTF16BEBOM[] = {0xFE,0xFF};

static HMODULE hDLL = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

static BOOL CALLBACK IsArchiveExt(LPTSTR pszExt);
static LPTSTR CALLBACK CheckArchivePath(LPTSTR pszFilePath);
static BOOL CALLBACK EnumArchive(LPTSTR pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData);
static BOOL CALLBACK ExtractArchive(LPTSTR pszArchivePath, LPTSTR pszFileName, void **ppBuf, DWORD *pSize);
static BOOL CALLBACK EnumArchive2(LPTSTR pszFilePath, LPFNADDLISTPROC lpfnAddListProc, LPFNCHECKFILETYPEPROC lpfnCheckProc, void *pData);
static BOOL CALLBACK ResolveIndirect(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pszStart, LPTSTR pszEnd);
static BOOL CALLBACK GetBasicTag(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pszTrack, LPTSTR pszTitle, LPTSTR pszAlbum, LPTSTR pszArtist);
static BOOL CALLBACK GetItemType(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pBuf, int nBufMax);
static LPTSTR CALLBACK GetItemFileName(LPTSTR pszArchivePath, LPTSTR pszTrackPart);
static BOOL CALLBACK GetGain(LPTSTR pszArchivePath, LPTSTR pszTrackPart, float *pGain, DWORD hBass);

static ARCHIVE_PLUGIN_INFO apinfo = {
	0,
	APDK_VER,
	IsArchiveExt,
	CheckArchivePath,
	EnumArchive,
	ExtractArchive,
	0,
	0,
	EnumArchive2,
	ResolveIndirect,
	GetBasicTag,
	GetItemType,
	GetItemFileName,
	GetGain
};

static BOOL InitArchive(){	
	return TRUE;
}

#ifdef __cplusplus
extern "C"
#endif
ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void){
	return InitArchive() ? &apinfo : 0;
}

static BOOL CALLBACK IsArchiveExt(LPTSTR pszExt){
	if(lstrcmpi(pszExt, TEXT("cue"))==0){
		return TRUE;
	}
	return FALSE;
}

static LPTSTR CALLBACK CheckArchivePath(LPTSTR pszFilePath)
{
	return StrStrI(pszFilePath, TEXT(".cue/"));
}

static BOOL CALLBACK EnumArchive(LPTSTR pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	return FALSE;
}

static BOOL CALLBACK ExtractArchive(LPTSTR pszArchivePath, LPTSTR pszFileName, void **ppBuf, DWORD *pSize)
{
	return FALSE;
}

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

// Detect CUE-Sheet Charset
static CueCharset DetectCharset(HANDLE hFile){
	BYTE buf[3];
	DWORD readed;
	ReadFile(hFile,buf,3,&readed,NULL);
	SetFilePointer(hFile,0,0,FILE_BEGIN);
	if(readed != 3)return CUECHAR_NA;
	if(memcmp(buf,UTF8BOM,3)==0){
		return CUECHAR_UTF8;
	}else if(memcmp(buf,UTF16LEBOM,2)==0 || buf[1] == 0){
		return CUECHAR_UTF16LE;
	}else if(memcmp(buf,UTF16BEBOM,2)==0 || buf[0] == 0){
		return CUECHAR_UTF16BE;
	}else{
		return CUECHAR_DEFAULT;
	}
}

// copied from bass_tag.cpp
static BOOL UTF8toShift_Jisn(LPSTR szOut, LPCSTR szIn, int cbOutBytes){
	int nSize;
	LPWSTR wTmp;

	// サイズ取得
	nSize = MultiByteToWideChar(CP_UTF8, 0, szIn, -1, NULL, 0);
	if(!nSize) return FALSE;

	// 領域確保
	wTmp = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR)*(nSize+1));
	if(!wTmp) return FALSE;
	wTmp[nSize] = L'\0';

	MultiByteToWideChar(CP_UTF8, 0, szIn, -1, wTmp, nSize);
	if(!nSize){
		HeapFree(GetProcessHeap(), 0, wTmp);
		return FALSE;
	}

	WideCharToMultiByte(CP_ACP, 0, wTmp, lstrlenW(wTmp), szOut, cbOutBytes*sizeof(CHAR), NULL, NULL);
	szOut[cbOutBytes-1] = '\0';
	HeapFree(GetProcessHeap(), 0, wTmp);

	return TRUE;
}

/* 単語を取得 */
static LPTSTR GetWord(LPTSTR pszLine, LPTSTR szToken, int nSize){
	int j = 0;
	LPTSTR p = pszLine;
	BOOL bInQuate = FALSE;

	// skip whitespace
	while (*p==TEXT(' ') || *p== TEXT('\t')){
		p++;
	}

	// get word
	do {
		if (*p==TEXT('\"')) bInQuate = !bInQuate;

		*(szToken + j++) = *p++;
		if(j>=nSize) return NULL;
	} while( bInQuate || !(*p==TEXT(' ') || *p==TEXT('\t')) && !*p==TEXT('\0'));
	*(szToken + j) = TEXT('\0');
	return p;
}

// convert string to SJIS
static void ConvertLine(LPTSTR pszOut, LPCSTR pszIn, int nOutSize, CueCharset Charset){
	int i;
	LPSTR tmp = NULL;
	ZeroMemory(pszOut, nOutSize * sizeof(TCHAR));
	switch(Charset){
		case CUECHAR_UTF8:
#ifdef UNICODE
			MultiByteToWideChar(CP_UTF8, 0, pszIn, -1, pszOut, nOutSize);
#else
			UTF8toShift_Jisn(pszOut,pszIn,nOutSize);
#endif
			break;
		case CUECHAR_UTF16LE:
#ifdef UNICODE
			lstrcpynW(pszOut, (LPWSTR)(pszIn+(pszIn[0]==0?1:0)), nOutSize);
#else
			WideCharToMultiByte(CP_ACP,0,(LPWSTR)(pszIn+(pszIn[0]==0?1:0)),-1,pszOut,nOutSize,0,0);
#endif
			break;
		case CUECHAR_UTF16BE:
			tmp = (LPSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(lstrlenW((LPCWSTR)pszIn)+1)*2);
			for(i=0;i<lstrlenW((LPCWSTR)pszIn);i++){
				tmp[i*2] = pszIn[i*2+1];
				tmp[i*2+1] = pszIn[i*2];
			}
#ifdef UNICODE
			lstrcpynW(pszOut, (LPWSTR)tmp, nOutSize);
#else
			WideCharToMultiByte(CP_ACP,0,(LPWSTR)tmp,-1,pszOut,nOutSize,0,0);
#endif
			HeapFree(GetProcessHeap(),0,tmp);
			break;
		default:
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, 0, pszIn, -1, pszOut, nOutSize);
#else
			memcpy(pszOut,pszIn,nOutSize);
#endif
			break;
	}

}

static int ReadLine(HANDLE hFile, LPTSTR pszBuff, int nSize, CueCharset charset){
	DWORD bpc = (charset==CUECHAR_UTF16BE||charset==CUECHAR_UTF16LE?2:1);
	DWORD dwAccBytes = bpc;
	char buf[MAX_FITTLE_PATH];
	int i=0;
	char c[2] = {0};

	while (dwAccBytes>0) {
        ReadFile(hFile, c, bpc, &dwAccBytes, NULL);
		if(dwAccBytes != bpc){
			memset(buf+i,0,bpc);
			break;
		}
		if( ( (charset==CUECHAR_UTF16BE) && (c[0]=='\0') ) ||
			( (charset==CUECHAR_UTF16LE) && (c[1]=='\0') ) ||
			(bpc==1) ){
			switch(c[0]|c[1]){
				case '\r':
					i-=bpc; // cancel \r
					break;
				case '\n':
					*(buf+i) = '\0'; 
					if(bpc==2){*(buf+i+1) = '\0';}
					dwAccBytes = 0;
					break;
				default:
					*(buf+i) = c[0]; 
					if(bpc==2){*(buf+i+1) = c[1];}
					break;
			}
		}else{
			*(WCHAR*)(buf+i)=*(WCHAR*)c;
		}
		i+=bpc;
		if( i+bpc >= MAX_FITTLE_PATH) return -1;
	}
	ConvertLine(pszBuff,buf,nSize,charset);

	return i;
}

/* CUEシートを読み込む */
static BOOL CALLBACK EnumArchive2(LPTSTR pszFilePath, LPFNADDLISTPROC lpfnAddListProc, LPFNCHECKFILETYPEPROC lpfnCheckProc, void *pData){
	HANDLE hFile;
	TCHAR szLine[MAX_FITTLE_PATH] = {0};
	TCHAR szCuePath[MAX_FITTLE_PATH] = {0};
	TCHAR szAddPath[MAX_FITTLE_PATH] = {0};
	TCHAR szToken[MAX_FITTLE_PATH] = {0};
	TCHAR szAudioPath[MAX_FITTLE_PATH] = {0};
	TCHAR szTitle[256] = {0};
	CueCharset charset = CUECHAR_NA;

	// オープン
	hFile = CreateFile(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;
	charset = DetectCharset(hFile);

	// 一行ずつ読み込む
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
		LPTSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, TEXT("FILE"))){
			// 対応形式であるかチェック
			GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(!lpfnCheckProc(szToken)) return -1;
			lstrcpyn(szAudioPath, szToken, MAX_FITTLE_PATH);
		}else if(!lstrcmp(szToken, TEXT("TRACK"))){
			if (szAddPath[0]){
				lpfnAddListProc(szAddPath, TEXT("-"), TEXT("-"), pData);
			}
			GetWord(p, szToken, MAX_FITTLE_PATH);
			wsprintf(szCuePath, TEXT("%s/%s."), pszFilePath, szToken);
			lstrcpyn(szAddPath, szCuePath, MAX_FITTLE_PATH);
			lstrcpyn(szTitle, PathFindFileName(szAudioPath), 256);
		}else if(!lstrcmp(szToken, TEXT("TITLE"))){
			// タイトル
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			lstrcpyn(szTitle, szToken, 256);
		}else if(!lstrcmp(szToken, TEXT("INDEX"))){
			// タイトルを連結
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			if(!lstrcmp(szToken, TEXT("01"))){
				//lstrcpyn(szAddPath, szCuePath, MAX_FITTLE_PATH);
				lstrcat(szAddPath, szTitle);
			}
		}
	}

	if (szAddPath[0]){
		lpfnAddListProc(szAddPath, TEXT("-"), TEXT("-"), pData);
	}

	// クローズ
	CloseHandle(hFile);

	return szAddPath[0] ? TRUE : FALSE;
}


/* CUEシートをパースし、実際のファイルと時間を取得 */
static BOOL CALLBACK ResolveIndirect(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pszStart, LPTSTR pszEnd){
	HANDLE hFile;
	TCHAR szLine[MAX_FITTLE_PATH] = {0};
	TCHAR szCuePath[MAX_FITTLE_PATH] = {0};
	TCHAR szToken[MAX_FITTLE_PATH] = {0};
	int nCurrent = -1;
	CueCharset charset = CUECHAR_NA;
	int nIndex = XATOI(pszTrackPart);

	*pszStart = TEXT('\0');
	*pszEnd = TEXT('\0');

	lstrcpyn(szCuePath, pszArchivePath, MAX_FITTLE_PATH);

	// オープン
	hFile = CreateFile(szCuePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;
	charset = DetectCharset(hFile);

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
		LPTSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, TEXT("FILE"))){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(PathIsRelative(szToken)){
				lstrcpyn(pszArchivePath, szCuePath, MAX_FITTLE_PATH);
				lstrcpyn(PathFindFileName(pszArchivePath), szToken, MAX_FITTLE_PATH);
			}else{
				lstrcpyn(pszArchivePath, szToken, MAX_FITTLE_PATH);
			}
		}else if(!lstrcmp(szToken, TEXT("TRACK"))){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			nCurrent = XATOI(szToken);
		}else if(!lstrcmp(szToken, TEXT("INDEX"))){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			if(!lstrcmp(szToken, TEXT("01"))){
				GetWord(p, szToken, MAX_FITTLE_PATH);
				if(nIndex==nCurrent){
					lstrcpyn(pszStart, szToken, 100);
				}else if(nIndex+1==nCurrent && *pszEnd==TEXT('\0')){
					lstrcpyn(pszEnd, szToken, 100);
				}
			}else if(!lstrcmp(szToken, TEXT("00"))){
				GetWord(p, szToken, MAX_FITTLE_PATH);
				if(nIndex+1==nCurrent){
					lstrcpyn(pszEnd, szToken, 100);
				}
			}
		}
	}

	// クローズ
	CloseHandle(hFile);
	return TRUE;
}

/* CUEシートをパースし、タグ情報を取得 */
static BOOL  CALLBACK GetBasicTag(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pszTrack, LPTSTR pszTitle, LPTSTR pszAlbum, LPTSTR pszArtist){
	HANDLE hFile;
	TCHAR szLine[MAX_FITTLE_PATH] = {0};
	TCHAR szToken[MAX_FITTLE_PATH] = {0};
	int nCurrent = -1;
	CueCharset charset = CUECHAR_NA;
	int nIndex = XATOI(pszTrackPart);

	wsprintf(pszTrack, TEXT("%d"), nIndex);

	// オープン
	hFile = CreateFile(pszArchivePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;
	charset = DetectCharset(hFile);

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
		LPTSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, TEXT("TRACK"))){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			nCurrent = XATOI(szToken);
		}else if(!lstrcmp(szToken, TEXT("TITLE"))){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(nCurrent==-1){
				lstrcpyn(pszAlbum, szToken, 256);
			}else if(nIndex==nCurrent){
				lstrcpyn(pszTitle, szToken, 256);
			}
		}else if(!lstrcmp(szToken, TEXT("PERFORMER"))){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(nCurrent==-1 || nIndex==nCurrent){
				lstrcpyn(pszArtist, szToken, 256);
			}
		}
	}

	// クローズ
	CloseHandle(hFile);
	return (lstrlen(pszTitle) > 0);
}

static BOOL  CALLBACK GetItemType(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pBuf, int nBufMax){
	lstrcpyn(pBuf, TEXT("CUE"), nBufMax);
	return TRUE;
}

static LPTSTR  CALLBACK GetItemFileName(LPTSTR pszArchivePath, LPTSTR pszTrackPart){
	return pszTrackPart;
}

static BOOL CALLBACK GetGain(LPTSTR pszArchivePath, LPTSTR pszTrackPart, float *pGain, DWORD hBass){
	HANDLE hFile;
	TCHAR szLine[MAX_FITTLE_PATH] = {0};
	TCHAR szToken[MAX_FITTLE_PATH] = {0};
	int nCurrent = -1;
	CueCharset charset = CUECHAR_NA;
	int nIndex = XATOI(pszTrackPart);

	float album_gain = 0;
	float album_peak = 1;
	float track_gain = 0;
	float track_peak = 1;
	float volume = 1;

	int nGainMode = SendMessage(apinfo.hwndMain, WM_F4B24_IPC, WM_F4B24_IPC_GET_REPLAYGAIN_MODE, 0);
	int nPreAmp = SendMessage(apinfo.hwndMain, WM_F4B24_IPC, WM_F4B24_IPC_GET_PREAMP, 0);
	if (!nPreAmp) nPreAmp = 100;

	// オープン
	hFile = CreateFile(pszArchivePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;
	charset = DetectCharset(hFile);

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
		LPTSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, TEXT("TRACK"))){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			nCurrent = XATOI(szToken);
		}else if(!lstrcmp(szToken, TEXT("REM"))){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			if(!lstrcmp(szToken, TEXT("REPLAYGAIN_ALBUM_GAIN"))){
				p = GetWord(p, szToken, MAX_FITTLE_PATH);
				track_gain = album_gain = XATOF(szToken);
			}else if(!lstrcmp(szToken, TEXT("REPLAYGAIN_ALBUM_PEAK"))){
				p = GetWord(p, szToken, MAX_FITTLE_PATH);
				track_peak = album_peak = XATOF(szToken);
			}else if(!lstrcmp(szToken, TEXT("REPLAYGAIN_TRACK_GAIN"))){
				if(nIndex==nCurrent){
					p = GetWord(p, szToken, MAX_FITTLE_PATH);
					track_gain = XATOF(szToken);
				}
			}else if(!lstrcmp(szToken, TEXT("REPLAYGAIN_TRACK_PEAK"))){
				if(nIndex==nCurrent){
					p = GetWord(p, szToken, MAX_FITTLE_PATH);
					track_peak = XATOF(szToken);
				}
			}
		}
	}

	switch (nGainMode){
	case 1:
		volume = nPreAmp * pow(10, album_gain / 20 - 2);
		break;
	case 2:
		volume = 1 / album_peak;
		break;
	case 3:
		volume = nPreAmp * pow(10, track_gain / 20 - 2);
		break;
	case 4:
		volume = 1 / track_peak;
		break;
	case 5:
		volume = nPreAmp * pow(10, album_gain / 20 - 2);
		if (volume > 1 / album_peak)
			volume = 1 / album_peak;
		break;
	case 6:
		volume = nPreAmp * pow(10, track_gain / 20 - 2);
		if (volume > 1 / track_peak)
			volume = 1 / track_peak;
		break;
	}
	
	// クローズ
	CloseHandle(hFile);

	*pGain = volume;
	return TRUE;
}

#include "../../fittle/src/aplugin.h"
#include <shlwapi.h>
#include <time.h>

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(linker, "/EXPORT:GetAPluginInfo=_GetAPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#define MAX_FITTLE_PATH 260*2

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

static BOOL CALLBACK IsArchiveExt(char *pszExt){
	if(lstrcmpi(pszExt, "cue")==0){
		return TRUE;
	}
	return FALSE;
}

static char * CALLBACK CheckArchivePath(char *pszFilePath)
{
	char *p = StrStrI(pszFilePath, ".cue/");
	return p;
}

static BOOL CALLBACK EnumArchive(char *pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	return FALSE;
}

static BOOL CALLBACK ExtractArchive(char *pszArchivePath, char *pszFileName, void **ppBuf, DWORD *pSize)
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
	wchar_t *wTmp;

	// サイズ取得
	nSize = MultiByteToWideChar(CP_UTF8, 0, szIn, -1, NULL, 0);
	if(!nSize) return FALSE;

	// 領域確保
	wTmp = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(wchar_t)*(nSize+1));
	if(!wTmp) return FALSE;
	wTmp[nSize] = L'\0';

	MultiByteToWideChar(CP_UTF8, 0, szIn, -1, wTmp, nSize);
	if(!nSize){
		HeapFree(GetProcessHeap(), 0, wTmp);
		return FALSE;
	}

	WideCharToMultiByte(CP_ACP, 0, wTmp, lstrlenW(wTmp), szOut, cbOutBytes*sizeof(char), NULL, NULL);
	szOut[cbOutBytes-1] = '\0';
	HeapFree(GetProcessHeap(), 0, wTmp);

	return TRUE;
}

/* 単語を取得 */
static LPSTR GetWord(LPSTR pszLine, LPSTR szToken, int nSize){
	int j = 0;
	LPSTR p = pszLine;
	BOOL bInQuate = FALSE;

	// skip whitespace
	while (*p==' ' || *p=='\t'){
		p++;
	}

	// get word
	do {
		if (*p=='\"') bInQuate = !bInQuate;

		*(szToken + j++) = *p++;
		if(j>=nSize) return NULL;
	} while( bInQuate || !(*p==' ' || *p=='\t') && !*p=='\0');
	*(szToken + j) = '\0';
	return p;
}

// convert string to SJIS
static void ConverttoShift_Jis(LPSTR pszOut, LPCSTR pszIn, int cbOutBytes, CueCharset Charset){
	int i;
	LPSTR tmp = NULL;
	ZeroMemory(pszOut,cbOutBytes);
	switch(Charset){
		case CUECHAR_UTF8:
			UTF8toShift_Jisn(pszOut,pszIn,cbOutBytes);
			break;
		case CUECHAR_UTF16LE:
			WideCharToMultiByte(CP_ACP,0,(LPWSTR)(pszIn+(pszIn[0]==0?1:0)),-1,pszOut,cbOutBytes,0,0);
			break;
		case CUECHAR_UTF16BE:
			tmp = (LPSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(lstrlenW((LPCWSTR)pszIn)+1)*2);
			for(i=0;i<lstrlenW((LPCWSTR)pszIn);i++){
				tmp[i*2] = pszIn[i*2+1];
				tmp[i*2+1] = pszIn[i*2];
			}
			WideCharToMultiByte(CP_ACP,0,(LPWSTR)tmp,-1,pszOut,cbOutBytes,0,0);
			HeapFree(GetProcessHeap(),0,tmp);
			break;
		default:
			memcpy(pszOut,pszIn,cbOutBytes);
			break;
	}

}

static int ReadLine(HANDLE hFile, LPSTR pszBuff, int nSize, CueCharset charset){
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
	ConverttoShift_Jis(pszBuff,buf,nSize,charset);

	return i;
}

/* CUEシートを読み込む */
static BOOL CALLBACK EnumArchive2(char *pszFilePath, LPFNADDLISTPROC lpfnAddListProc, LPFNCHECKFILETYPEPROC lpfnCheckProc, void *pData){
	HANDLE hFile;
	char szLine[MAX_FITTLE_PATH] = {0};
	char szCuePath[MAX_FITTLE_PATH] = {0};
	char szAddPath[MAX_FITTLE_PATH] = {0};
	char szToken[MAX_FITTLE_PATH] = {0};
	char szAudioPath[MAX_FITTLE_PATH] = {0};
	char szTitle[256] = {0};
	CueCharset charset = CUECHAR_NA;

	// オープン
	hFile = CreateFile(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;
	charset = DetectCharset(hFile);

	// 一行ずつ読み込む
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
		LPSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, "FILE")){
			// 対応形式であるかチェック
			GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(!lpfnCheckProc(szToken)) return -1;
			lstrcpyn(szAudioPath, szToken, MAX_FITTLE_PATH);
		}else if(!lstrcmp(szToken, "TRACK")){
			if (szAddPath[0]){
				lpfnAddListProc(szAddPath, "-", "-", pData);
			}
			GetWord(p, szToken, MAX_FITTLE_PATH);
			wsprintf(szCuePath, "%s/%s.", pszFilePath, szToken);
			lstrcpyn(szAddPath, szCuePath, MAX_FITTLE_PATH);
			lstrcpyn(szTitle, PathFindFileName(szAudioPath), 256);
		}else if(!lstrcmp(szToken, "TITLE")){
			// タイトル
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			lstrcpyn(szTitle, szToken, 256);
		}else if(!lstrcmp(szToken, "INDEX")){
			// タイトルを連結
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			if(!lstrcmp(szToken, "01")){
				//lstrcpyn(szAddPath, szCuePath, MAX_FITTLE_PATH);
				lstrcat(szAddPath, szTitle);
			}
		}
	}

	if (szAddPath[0]){
		lpfnAddListProc(szAddPath, "-", "-", pData);
	}

	// クローズ
	CloseHandle(hFile);

	return szAddPath[0] ? TRUE : FALSE;
}


/* CUEシートをパースし、実際のファイルと時間を取得 */
static BOOL CALLBACK ResolveIndirect(char *pszArchivePath, char *pszTrackPart, char *pszStart, char *pszEnd){
	HANDLE hFile;
	char szLine[MAX_FITTLE_PATH] = {0};
	char szCuePath[MAX_FITTLE_PATH] = {0};
	char szToken[MAX_FITTLE_PATH] = {0};
	int nCurrent = -1;
	CueCharset charset = CUECHAR_NA;
	int nIndex = XATOI(pszTrackPart);

	*pszStart = '\0';
	*pszEnd = '\0';

	lstrcpyn(szCuePath, pszArchivePath, MAX_FITTLE_PATH);

	// オープン
	hFile = CreateFile(szCuePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;
	charset = DetectCharset(hFile);

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
		LPSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, "FILE")){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(PathIsRelative(szToken)){
				lstrcpyn(pszArchivePath, szCuePath, MAX_FITTLE_PATH);
				lstrcpyn(PathFindFileName(pszArchivePath), szToken, MAX_FITTLE_PATH);
			}else{
				lstrcpyn(pszArchivePath, szToken, MAX_FITTLE_PATH);
			}
		}else if(!lstrcmp(szToken, "TRACK")){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			nCurrent = XATOI(szToken);
		}else if(!lstrcmp(szToken, "INDEX")){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			if(!lstrcmp(szToken, "01")){
				GetWord(p, szToken, MAX_FITTLE_PATH);
				if(nIndex==nCurrent){
					lstrcpyn(pszStart, szToken, 100);
				}else if(nIndex+1==nCurrent && *pszEnd=='\0'){
					lstrcpyn(pszEnd, szToken, 100);
				}
			}else if(!lstrcmp(szToken, "00")){
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
static BOOL  CALLBACK GetBasicTag(char *pszArchivePath, char *pszTrackPart, char *pszTrack, char *pszTitle, char *pszAlbum, char *pszArtist){
	HANDLE hFile;
	char szLine[MAX_FITTLE_PATH] = {0};
	char szToken[MAX_FITTLE_PATH] = {0};
	int nCurrent = -1;
	CueCharset charset = CUECHAR_NA;
	int nIndex = XATOI(pszTrackPart);

	wsprintf(pszTrack, "%d", nIndex);

	// オープン
	hFile = CreateFile(pszArchivePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;
	charset = DetectCharset(hFile);

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
		LPSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, "TRACK")){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			nCurrent = XATOI(szToken);
		}else if(!lstrcmp(szToken, "TITLE")){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(nCurrent==-1){
				lstrcpyn(pszAlbum, szToken, 256);
			}else if(nIndex==nCurrent){
				lstrcpyn(pszTitle, szToken, 256);
			}
		}else if(!lstrcmp(szToken, "PERFORMER")){
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

static BOOL  CALLBACK GetItemType(char *pszArchivePath, char *pszTrackPart, char *pBuf, int nBufMax){
	lstrcpyn(pBuf, "CUE", nBufMax);
	return TRUE;
}

static char * CALLBACK GetItemFileName(char *pszArchivePath, char *pszTrackPart){
	return pszTrackPart;
}


static ARCHIVE_PLUGIN_INFO apinfo = {
	0,
	2,
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
	GetItemFileName
};

static BOOL InitArchive(){	
	return TRUE;
}

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

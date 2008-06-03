#include "../fittle/src/aplugin.h"
#include <shlwapi.h>
#include <time.h>
#include "../../extra/unlha32/UNLHA32.H"

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

typedef HARC (WINAPI *LPUNZIPOPENARCHIVE)(const HWND, LPCSTR, const DWORD);
typedef int (WINAPI *LPUNZIPFINDFIRST)(HARC, LPCSTR, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNZIPFINDNEXT)(HARC, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNZIPCLOSEARCHIVE)(HARC);
typedef int (WINAPI *LPUNZIPEXTRACTMEM)(const HWND, LPCSTR,	LPBYTE, const DWORD, int *,	LPWORD, LPDWORD);

static LPUNZIPOPENARCHIVE lpUnZipOpenArchive = NULL;
static LPUNZIPFINDFIRST lpUnZipFindFirst = NULL;
static LPUNZIPFINDNEXT lpUnZipFindNext = NULL;
static LPUNZIPCLOSEARCHIVE lpUnZipCloseArchive = NULL;
static LPUNZIPEXTRACTMEM lpUnZipExtractMem = NULL;

static HMODULE hUnzip32 = 0;
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
	if(lstrcmpi(pszExt, "zip")==0 || lstrcmpi(pszExt, "abz")==0){
		return TRUE;
	}
	return FALSE;
}

static char * CALLBACK CheckArchivePath(char *pszFilePath)
{
	char *p = StrStrI(pszFilePath, ".zip/");
	if(!p){
		p = StrStrI(pszFilePath, ".abz/");
	}
	return p;
}

static BOOL CALLBACK EnumArchive(char *pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	INDIVIDUALINFO iinfo;
	HARC hArc;
	// アーカイブをオープン
	hArc = lpUnZipOpenArchive(NULL, pszFilePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnZipFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			FILETIME ft;
			DosDateTimeToFileTime(iinfo.wDate, iinfo.wTime, &ft);
			lpfnProc(iinfo.szFileName, iinfo.dwOriginalSize, ft, pData);
		}while(lpUnZipFindNext(hArc, &iinfo)!=-1);
	}

	lpUnZipCloseArchive(hArc);
	return TRUE;
}

static BOOL CALLBACK ExtractArchive(char *pszArchivePath, char *pszFileName, void **ppBuf, DWORD *pSize)
{
	char *p, *q;
	int i=0;
	INDIVIDUALINFO iinfo;
	HARC hArc;
	char cmd[MAX_PATH*2*2];
	char szPlayFile[MAX_PATH*2] = {0};
	int ret;

	p = pszFileName;

	// エスケープシーケンスの処理
	for(i=0;*p;p++){
		if(IsDBCSLeadByte(*p)){
			szPlayFile[i++] = *p++;
			szPlayFile[i++] = *p;
		}else{
			if(*p=='[' || *p==']' || *p=='!' || *p=='^' || *p=='-' || *p=='\\') szPlayFile[i++] = '\\';
			szPlayFile[i++] = *p;
		}
	}

	// アーカイブをオープン
	hArc = lpUnZipOpenArchive(NULL, pszArchivePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnZipFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			if(!lstrcmpi(iinfo.szFileName, pszFileName)) break;
		}while(lpUnZipFindNext(hArc, &iinfo)!=-1);
	}
	
	// 解凍
	*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), /*HEAP_ZERO_MEMORY*/0, iinfo.dwOriginalSize);
	if (*ppBuf)
	{
		wsprintf(cmd, "--i -qq \"%s\" \"%s\"", pszArchivePath, szPlayFile);
		ret = lpUnZipExtractMem(NULL, cmd, (LPBYTE)*ppBuf, iinfo.dwOriginalSize, NULL, NULL, NULL);
		if(!ret){
			lpUnZipCloseArchive(hArc);
			*pSize = iinfo.dwOriginalSize;
			return TRUE;
		}
		HeapFree(GetProcessHeap(), 0, *ppBuf);
	}
	lpUnZipCloseArchive(hArc);
	return FALSE;

}

static ARCHIVE_PLUGIN_INFO apinfo = {
	0,
	APDK_VER,
	IsArchiveExt,
	CheckArchivePath,
	EnumArchive,
	ExtractArchive
};

static BOOL InitArchive(){	
	if (!hUnzip32) hUnzip32 = LoadLibrary("Unzip32.dll");
	if(!hUnzip32) return FALSE;

	lpUnZipOpenArchive = (LPUNZIPOPENARCHIVE )GetProcAddress(hUnzip32,"UnZipOpenArchive");
	if(!lpUnZipOpenArchive) return FALSE;
	lpUnZipFindFirst = (LPUNZIPFINDFIRST )GetProcAddress(hUnzip32,"UnZipFindFirst");
	if(!lpUnZipFindFirst) return FALSE;
	lpUnZipFindNext = (LPUNZIPFINDNEXT )GetProcAddress(hUnzip32,"UnZipFindNext");
	if(!lpUnZipFindNext) return FALSE;
	lpUnZipCloseArchive = (LPUNZIPCLOSEARCHIVE )GetProcAddress(hUnzip32,"UnZipCloseArchive");
	if(!lpUnZipCloseArchive) return FALSE;
	lpUnZipExtractMem = (LPUNZIPEXTRACTMEM )GetProcAddress(hUnzip32,"UnZipExtractMem");
	if(!lpUnZipExtractMem) return FALSE;
	return TRUE;
}

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

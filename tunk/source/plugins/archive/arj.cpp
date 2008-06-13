#include "../../fittle/src/aplugin.h"
#include <shlwapi.h>
#include <time.h>
#include "../../../extra/unlha32/UNLHA32.H"

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

typedef HARC (WINAPI *LPUNARJOPENARCHIVE)(const HWND, LPCSTR, const DWORD);
typedef int (WINAPI *LPUNARJFINDFIRST)(HARC, LPCSTR, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNARJFINDNEXT)(HARC, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNARJCLOSEARCHIVE)(HARC);
typedef int (WINAPI *LPUNARJEXTRACTMEM)(const HWND, LPCSTR,	LPBYTE, const DWORD, int *,	LPWORD, LPDWORD);

static LPUNARJOPENARCHIVE lpUnArjOpenArchive = NULL;
static LPUNARJFINDFIRST lpUnArjFindFirst = NULL;
static LPUNARJFINDNEXT lpUnArjFindNext = NULL;
static LPUNARJCLOSEARCHIVE lpUnArjCloseArchive = NULL;
static LPUNARJEXTRACTMEM lpUnArjExtractMem = NULL;

static HMODULE hUnarj32 = 0;
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
	if(lstrcmpi(pszExt, "lzh")==0 || lstrcmpi(pszExt, "arj")==0){
		return TRUE;
	}
	return FALSE;
}

static char * CALLBACK CheckArchivePath(char *pszFilePath)
{
	char *p = StrStrI(pszFilePath, ".lzh/");
	if(!p){
		p = StrStrI(pszFilePath, ".arj/");
	}
	return p;
}

static BOOL CALLBACK EnumArchive(char *pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	INDIVIDUALINFO iinfo;
	HARC hArc;
	// アーカイブをオープン
	hArc = lpUnArjOpenArchive(NULL, pszFilePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnArjFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			FILETIME ft;
			DosDateTimeToFileTime(iinfo.wDate, iinfo.wTime, &ft);
			lpfnProc(iinfo.szFileName, iinfo.dwOriginalSize, ft, pData);
		}while(lpUnArjFindNext(hArc, &iinfo)!=-1);
	}

	lpUnArjCloseArchive(hArc);
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
//			if(*p=='[' || *p==']' || *p=='!' || *p=='^' || *p=='-' || *p=='\\') szPlayFile[i++] = '\\';
			szPlayFile[i++] = *p;
		}
	}

	// アーカイブをオープン
	hArc = lpUnArjOpenArchive(NULL, pszArchivePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnArjFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			if(!lstrcmpi(iinfo.szFileName, pszFileName)) break;
		}while(lpUnArjFindNext(hArc, &iinfo)!=-1);
	}
	lpUnArjCloseArchive(hArc);
	
	// 解凍
	*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), /*HEAP_ZERO_MEMORY*/0, iinfo.dwOriginalSize);
	if (*ppBuf)
	{
		wsprintf(cmd, "-n -gm \"%s\" \"%s\"", pszArchivePath, szPlayFile);
		ret = lpUnArjExtractMem(NULL, cmd, (LPBYTE)*ppBuf, iinfo.dwOriginalSize, NULL, NULL, NULL);
		if(!ret){
			*pSize = iinfo.dwOriginalSize;
			return TRUE;
		}
		char erx[64];
		wsprintf(erx,"%08x",ret);
		MessageBox(NULL,erx,pszArchivePath,MB_OK);
		HeapFree(GetProcessHeap(), 0, *ppBuf);
	}
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
	if (!hUnarj32) hUnarj32 = LoadLibrary("UNARJ32J.DLL");
	if(!hUnarj32) return FALSE;

	lpUnArjOpenArchive = (LPUNARJOPENARCHIVE )GetProcAddress(hUnarj32,"UnarjOpenArchive");
	if(!lpUnArjOpenArchive) return FALSE;
	lpUnArjFindFirst = (LPUNARJFINDFIRST )GetProcAddress(hUnarj32,"UnarjFindFirst");
	if(!lpUnArjFindFirst) return FALSE;
	lpUnArjFindNext = (LPUNARJFINDNEXT )GetProcAddress(hUnarj32,"UnarjFindNext");
	if(!lpUnArjFindNext) return FALSE;
	lpUnArjCloseArchive = (LPUNARJCLOSEARCHIVE )GetProcAddress(hUnarj32,"UnarjCloseArchive");
	if(!lpUnArjCloseArchive) return FALSE;
	lpUnArjExtractMem = (LPUNARJEXTRACTMEM )GetProcAddress(hUnarj32,"UnarjExtractMem");
	if(!lpUnArjExtractMem) return FALSE;
	return TRUE;
}

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

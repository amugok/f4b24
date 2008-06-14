#include "../../fittle/src/aplugin.h"
#include <shlwapi.h>
#include <time.h>
#include "../../../extra/unlha32/UNLHA32.H"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")
#ifdef UNICODE
#pragma comment(linker, "/EXPORT:GetAPluginInfoW=_GetAPluginInfoW@0")
#define GetAPluginInfo GetAPluginInfoW
#define UNICODE_POSTFIX "W"
#else
#pragma comment(linker, "/EXPORT:GetAPluginInfo=_GetAPluginInfo@0")
#define UNICODE_POSTFIX
#endif
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

typedef HARC (WINAPI *LPUNLHAOPENARCHIVE)(const HWND, LPCTSTR, const DWORD);
typedef int (WINAPI *LPUNLHAFINDFIRST)(HARC, LPCTSTR, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNLHAFINDNEXT)(HARC, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNLHACLOSEARCHIVE)(HARC);
typedef int (WINAPI *LPUNLHAEXTRACTMEM)(const HWND, LPCTSTR, LPBYTE, const DWORD, int *, LPWORD, LPDWORD);

static LPUNLHAOPENARCHIVE lpUnLhaOpenArchive = NULL;
static LPUNLHAFINDFIRST lpUnLhaFindFirst = NULL;
static LPUNLHAFINDNEXT lpUnLhaFindNext = NULL;
static LPUNLHACLOSEARCHIVE lpUnLhaCloseArchive = NULL;
static LPUNLHAEXTRACTMEM lpUnLhaExtractMem = NULL;

static HMODULE hUnlha32 = 0;
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

static BOOL CALLBACK IsArchiveExt(LPTSTR pszExt){
	if(lstrcmpi(pszExt, TEXT("arj"))==0){
		return TRUE;
	}
	return FALSE;
}

static LPTSTR CALLBACK CheckArchivePath(LPTSTR pszFilePath)
{
	return StrStrI(pszFilePath, TEXT(".arj/"));
}

static BOOL CALLBACK EnumArchive(LPTSTR pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	INDIVIDUALINFO iinfo;
	HARC hArc;
	// アーカイブをオープン
	hArc = lpUnLhaOpenArchive(NULL, pszFilePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnLhaFindFirst(hArc, TEXT("*.*"), &iinfo)!=-1){
		do{
			FILETIME ft;
			DosDateTimeToFileTime(iinfo.wDate, iinfo.wTime, &ft);
			lpfnProc(iinfo.szFileName, iinfo.dwOriginalSize, ft, pData);
		}while(lpUnLhaFindNext(hArc, &iinfo)!=-1);
	}

	lpUnLhaCloseArchive(hArc);
	return TRUE;
}

static BOOL CALLBACK ExtractArchive(LPTSTR pszArchivePath, LPTSTR pszFileName, void **ppBuf, DWORD *pSize)
{
	LPTSTR p, q;
	int i=0;
	INDIVIDUALINFO iinfo;
	HARC hArc;
	TCHAR cmd[MAX_PATH*2*2];
	TCHAR szPlayFile[MAX_PATH*2] = {0};
	int ret;

	p = pszFileName;

	// エスケープシーケンスの処理
	for(i=0;*p;p++){
#ifdef UNICODE
#else
		if(IsDBCSLeadByte(*p)){
			szPlayFile[i++] = *p++;
			szPlayFile[i++] = *p;
			continue;
		}
#endif
//		if(*p==TEXT('[') || *p==TEXT(']') || *p==TEXT('!') || *p==TEXT('^') || *p==TEXT('-') || *p==TEXT('\\')) szPlayFile[i++] = TEXT('\\');
		szPlayFile[i++] = *p;
	}

	// アーカイブをオープン
	hArc = lpUnLhaOpenArchive(NULL, pszArchivePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnLhaFindFirst(hArc, TEXT("*.*"), &iinfo)!=-1){
		do{
			if(!lstrcmpi(iinfo.szFileName, pszFileName)) break;
		}while(lpUnLhaFindNext(hArc, &iinfo)!=-1);
	}
	lpUnLhaCloseArchive(hArc);

	if (iinfo.dwOriginalSize == 0) return FALSE;

	// 解凍
	*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), /*HEAP_ZERO_MEMORY*/0, iinfo.dwOriginalSize);
	if (*ppBuf)
	{
		wsprintf(cmd, TEXT("-n -gm \"%s\" \"%s\""), pszArchivePath, szPlayFile);
		ret = lpUnLhaExtractMem(NULL, cmd, (LPBYTE)*ppBuf, iinfo.dwOriginalSize, NULL, NULL, NULL);
		if(!ret){
			*pSize = iinfo.dwOriginalSize;
			return TRUE;
		}
		TCHAR erx[64];
		wsprintf(erx,TEXT("%08x"),ret);
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
	if (!hUnlha32) hUnlha32 = LoadLibrary(TEXT("UNARJ32J.DLL"));
	if(!hUnlha32) return FALSE;

	lpUnLhaOpenArchive = (LPUNLHAOPENARCHIVE )GetProcAddress(hUnlha32,"UnarjOpenArchive" UNICODE_POSTFIX);
	if(!lpUnLhaOpenArchive) return FALSE;
	lpUnLhaFindFirst = (LPUNLHAFINDFIRST )GetProcAddress(hUnlha32,"UnarjFindFirst" UNICODE_POSTFIX);
	if(!lpUnLhaFindFirst) return FALSE;
	lpUnLhaFindNext = (LPUNLHAFINDNEXT )GetProcAddress(hUnlha32,"UnarjFindNext" UNICODE_POSTFIX);
	if(!lpUnLhaFindNext) return FALSE;
	lpUnLhaExtractMem = (LPUNLHAEXTRACTMEM )GetProcAddress(hUnlha32,"UnarjExtractMem" UNICODE_POSTFIX);
	if(!lpUnLhaExtractMem) return FALSE;
	lpUnLhaCloseArchive = (LPUNLHACLOSEARCHIVE )GetProcAddress(hUnlha32,"UnarjCloseArchive");
	if(!lpUnLhaCloseArchive) return FALSE;
	return TRUE;
}

#ifdef __cplusplus
extern "C"
#endif
ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

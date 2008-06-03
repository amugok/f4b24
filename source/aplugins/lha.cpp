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

typedef HARC (WINAPI *LPUNLHAOPENARCHIVE)(const HWND, LPCSTR, const DWORD);
typedef int (WINAPI *LPUNLHAFINDFIRST)(HARC, LPCSTR, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNLHAFINDNEXT)(HARC, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNLHACLOSEARCHIVE)(HARC);
typedef int (WINAPI *LPUNLHAEXTRACTMEM)(const HWND, LPCSTR,	LPBYTE, const DWORD, int *,	LPWORD, LPDWORD);

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

static BOOL CALLBACK IsArchiveExt(char *pszExt){
	if(lstrcmpi(pszExt, "lzh")==0 || lstrcmpi(pszExt, "lha")==0){
		return TRUE;
	}
	return FALSE;
}

static char * CALLBACK CheckArchivePath(char *pszFilePath)
{
	char *p = StrStrI(pszFilePath, ".lzh/");
	if(!p){
		p = StrStrI(pszFilePath, ".lha/");
	}
	return p;
}

static BOOL CALLBACK EnumArchive(char *pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	INDIVIDUALINFO iinfo;
	HARC hArc;
	// アーカイブをオープン
	hArc = lpUnLhaOpenArchive(NULL, pszFilePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnLhaFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			FILETIME ft;
			DosDateTimeToFileTime(iinfo.wDate, iinfo.wTime, &ft);
			lpfnProc(iinfo.szFileName, iinfo.dwCompressedSize, ft, pData);
		}while(lpUnLhaFindNext(hArc, &iinfo)!=-1);
	}

	lpUnLhaCloseArchive(hArc);
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
	hArc = lpUnLhaOpenArchive(NULL, pszArchivePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnLhaFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			if(!lstrcmpi(iinfo.szFileName, pszFileName)) break;
		}while(lpUnLhaFindNext(hArc, &iinfo)!=-1);
	}
	lpUnLhaCloseArchive(hArc);
	
	// 解凍
	*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), /*HEAP_ZERO_MEMORY*/0, iinfo.dwOriginalSize);
	if (*ppBuf)
	{
		wsprintf(cmd, "-n -gm \"%s\" \"%s\"", pszArchivePath, szPlayFile);
		ret = lpUnLhaExtractMem(NULL, cmd, (LPBYTE)*ppBuf, iinfo.dwOriginalSize, NULL, NULL, NULL);
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
	if (!hUnlha32) hUnlha32 = LoadLibrary("UNLHA32.DLL");
	if(!hUnlha32) return FALSE;

	lpUnLhaOpenArchive = (LPUNLHAOPENARCHIVE )GetProcAddress(hUnlha32,"UnlhaOpenArchive");
	if(!lpUnLhaOpenArchive) return FALSE;
	lpUnLhaFindFirst = (LPUNLHAFINDFIRST )GetProcAddress(hUnlha32,"UnlhaFindFirst");
	if(!lpUnLhaFindFirst) return FALSE;
	lpUnLhaFindNext = (LPUNLHAFINDNEXT )GetProcAddress(hUnlha32,"UnlhaFindNext");
	if(!lpUnLhaFindNext) return FALSE;
	lpUnLhaCloseArchive = (LPUNLHACLOSEARCHIVE )GetProcAddress(hUnlha32,"UnlhaCloseArchive");
	if(!lpUnLhaCloseArchive) return FALSE;
	lpUnLhaExtractMem = (LPUNLHAEXTRACTMEM )GetProcAddress(hUnlha32,"UnlhaExtractMem");
	if(!lpUnLhaExtractMem) return FALSE;
	return TRUE;
}

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

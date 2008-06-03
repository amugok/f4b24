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

typedef HARC (WINAPI *LPUNTAROPENARCHIVE)(const HWND, LPCSTR, const DWORD);
typedef int (WINAPI *LPUNTARFINDFIRST)(HARC, LPCSTR, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNTARFINDNEXT)(HARC, LPINDIVIDUALINFO);
typedef int (WINAPI *LPUNTARCLOSEARCHIVE)(HARC);
typedef int (WINAPI *LPUNTAREXTRACTMEM)(const HWND, LPCSTR,	LPBYTE, const DWORD, int *,	LPWORD, LPDWORD);

static LPUNTAROPENARCHIVE lpTarOpenArchive = NULL;
static LPUNTARFINDFIRST lpTarFindFirst = NULL;
static LPUNTARFINDNEXT lpTarFindNext = NULL;
static LPUNTARCLOSEARCHIVE lpTarCloseArchive = NULL;
static LPUNTAREXTRACTMEM lpTarExtractMem = NULL;

static HMODULE hTar32 = 0;
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
	if(lstrcmpi(pszExt, "tar")==0 || lstrcmpi(pszExt, "gz")==0 || lstrcmpi(pszExt, "tgz")==0 || lstrcmpi(pszExt, "bz2")==0 || lstrcmpi(pszExt, "tbz")==0){
		return TRUE;
	}
	return FALSE;
}

static char * CALLBACK CheckArchivePath(char *pszFilePath)
{
	char *p = StrStrI(pszFilePath, ".tar/");
	if(!p){
		p = StrStrI(pszFilePath, ".gz/");
		if(!p){
			p = StrStrI(pszFilePath, ".tgz/");
			if(!p){
				p = StrStrI(pszFilePath, ".bz2/");
				if(!p){
					p = StrStrI(pszFilePath, ".tbz/");
				}
			}
		}
	}
	return p;
}

static BOOL CALLBACK EnumArchive(char *pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	INDIVIDUALINFO iinfo;
	HARC hArc;
	// アーカイブをオープン
	hArc = lpTarOpenArchive(NULL, pszFilePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpTarFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			FILETIME ft;
			DosDateTimeToFileTime(iinfo.wDate, iinfo.wTime, &ft);
			lpfnProc(iinfo.szFileName, iinfo.dwCompressedSize, ft, pData);
		}while(lpTarFindNext(hArc, &iinfo)!=-1);
	}

	lpTarCloseArchive(hArc);
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
	hArc = lpTarOpenArchive(NULL, pszArchivePath, M_CHECK_FILENAME_ONLY);
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpTarFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			if(!lstrcmpi(iinfo.szFileName, pszFileName)) break;
		}while(lpTarFindNext(hArc, &iinfo)!=-1);
	}
	lpTarCloseArchive(hArc);
	
	// 解凍
	*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), /*HEAP_ZERO_MEMORY*/0, iinfo.dwOriginalSize);
	if (*ppBuf)
	{
		wsprintf(cmd, "--display-dialog=0 \"%s\" \"%s\"", pszArchivePath, szPlayFile);
		ret = lpTarExtractMem(NULL, cmd, (LPBYTE)*ppBuf, iinfo.dwOriginalSize, NULL, NULL, NULL);
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
	if (!hTar32) hTar32 = LoadLibrary("TAR32.DLL");
	if(!hTar32) return FALSE;

	lpTarOpenArchive = (LPUNTAROPENARCHIVE )GetProcAddress(hTar32,"TarOpenArchive");
	if(!lpTarOpenArchive) return FALSE;
	lpTarFindFirst = (LPUNTARFINDFIRST )GetProcAddress(hTar32,"TarFindFirst");
	if(!lpTarFindFirst) return FALSE;
	lpTarFindNext = (LPUNTARFINDNEXT )GetProcAddress(hTar32,"TarFindNext");
	if(!lpTarFindNext) return FALSE;
	lpTarCloseArchive = (LPUNTARCLOSEARCHIVE )GetProcAddress(hTar32,"TarCloseArchive");
	if(!lpTarCloseArchive) return FALSE;
	lpTarExtractMem = (LPUNTAREXTRACTMEM )GetProcAddress(hTar32,"TarExtractMem");
	if(!lpTarExtractMem) return FALSE;
	return TRUE;
}

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

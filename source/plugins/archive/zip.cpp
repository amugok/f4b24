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

typedef HARC (WINAPI *LPUNLHAOPENARCHIVE)(const HWND, LPCSTR, const DWORD);
typedef int (WINAPI *LPUNLHAFINDFIRST)(HARC, LPCSTR, LPINDIVIDUALINFOA);
typedef int (WINAPI *LPUNLHAFINDNEXT)(HARC, LPINDIVIDUALINFOA);
typedef int (WINAPI *LPUNLHAEXTRACTMEM)(const HWND, LPCSTR, LPBYTE, const DWORD, int *, LPWORD, LPDWORD);
typedef int (WINAPI *LPUNLHACLOSEARCHIVE)(HARC);

static LPUNLHAOPENARCHIVE lpUnLhaOpenArchive = NULL;
static LPUNLHAFINDFIRST lpUnLhaFindFirst = NULL;
static LPUNLHAFINDNEXT lpUnLhaFindNext = NULL;
static LPUNLHAEXTRACTMEM lpUnLhaExtractMem = NULL;
static LPUNLHACLOSEARCHIVE lpUnLhaCloseArchive = NULL;

static HMODULE hUnlha32 = 0;
static HMODULE hDLL = 0;

#define FUNC_PREFIXA "UnZip"
static /*const*/ TCHAR szDllName[] = TEXT("UNZIP32.DLL");
static /*const*/ struct IMPORT_FUNC_TABLE {
	LPSTR/*LPCSTR*/ lpszFuncName;
	FARPROC * ppFunc;
} functbl[] = {
	{ FUNC_PREFIXA "OpenArchive", (FARPROC *)&lpUnLhaOpenArchive },
	{ FUNC_PREFIXA "FindFirst", (FARPROC *)&lpUnLhaFindFirst },
	{ FUNC_PREFIXA "FindNext", (FARPROC *)&lpUnLhaFindNext },
	{ FUNC_PREFIXA "ExtractMem", (FARPROC *)&lpUnLhaExtractMem },
	{ FUNC_PREFIXA "CloseArchive", (FARPROC *)&lpUnLhaCloseArchive },
	{ 0, (FARPROC *)0 }
};

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
	if(lstrcmpi(pszExt, TEXT("abz"))==0) return TRUE;
	if(lstrcmpi(pszExt, TEXT("zip"))==0) return TRUE;
	return FALSE;
}

static LPTSTR CALLBACK CheckArchivePath(LPTSTR pszFilePath){
	LPTSTR p;
	p = StrStrI(pszFilePath, TEXT(".abz/"));
	if (p) return p;
	return StrStrI(pszFilePath, TEXT(".zip/"));
}

static BOOL CALLBACK EnumArchive(LPTSTR pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData){
	INDIVIDUALINFOA iinfo;
	HARC hArc;
	// アーカイブをオープン
#ifdef UNICODE
	CHAR nameA[MAX_PATH + 1];
	WideCharToMultiByte(CP_ACP, 0, pszFilePath, -1, nameA, MAX_PATH + 1, NULL, NULL);
	hArc = lpUnLhaOpenArchive(NULL, nameA, M_CHECK_FILENAME_ONLY);
#else
	hArc = lpUnLhaOpenArchive(NULL, pszFilePath, M_CHECK_FILENAME_ONLY);
#endif
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnLhaFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			FILETIME ft;
			DosDateTimeToFileTime(iinfo.wDate, iinfo.wTime, &ft);
#ifdef UNICODE
			{
				WCHAR nameW[MAX_PATH + 1];
				MultiByteToWideChar(CP_ACP, 0, iinfo.szFileName, -1, nameW, MAX_PATH + 1);
				lpfnProc(nameW, iinfo.dwOriginalSize, ft, pData);
			}
#else
			lpfnProc(iinfo.szFileName, iinfo.dwOriginalSize, ft, pData);
#endif
		}while(lpUnLhaFindNext(hArc, &iinfo)!=-1);
	}

	lpUnLhaCloseArchive(hArc);
	return TRUE;
}

static BOOL CALLBACK ExtractArchive(LPTSTR pszArchivePath, LPTSTR pszFileName, void **ppBuf, DWORD *pSize)
{
	INDIVIDUALINFOA iinfo;
	HARC hArc;
	DWORD dwOutputSize;
	DWORD dwBufferSize;

	// アーカイブをオープン
#ifdef UNICODE
	CHAR nameA[MAX_PATH + 1];
	WideCharToMultiByte(CP_ACP, 0, pszArchivePath, -1, nameA, MAX_PATH + 1, NULL, NULL);
	hArc = lpUnLhaOpenArchive(NULL, nameA, M_CHECK_FILENAME_ONLY);
#else
	hArc = lpUnLhaOpenArchive(NULL, pszArchivePath, M_CHECK_FILENAME_ONLY);
#endif
	if(!hArc){
		return FALSE;
	}
	// 検索開始
	if(lpUnLhaFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
#ifdef UNICODE
			WCHAR nameW[MAX_PATH + 1];
			MultiByteToWideChar(CP_ACP, 0, iinfo.szFileName, -1, nameW, MAX_PATH + 1);
			if(!lstrcmpi(nameW, pszFileName)) break;
#else
			if(!lstrcmpi(iinfo.szFileName, pszFileName)) break;
#endif
		}while(lpUnLhaFindNext(hArc, &iinfo)!=-1);
	}
	lpUnLhaCloseArchive(hArc);
	
	// 解凍
	if (iinfo.dwOriginalSize != 0){
		dwBufferSize = iinfo.dwOriginalSize;
#if 0
	} else if (iinfo.dwCompressedSize != 0) {
		/* 圧縮率50%を仮定 */
		dwBufferSize = iinfo.dwCompressedSize * 2;
	} else {
		HANDLE h = CreateFile(pszArchivePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h == INVALID_HANDLE_VALUE) return FALSE;
		dwBufferSize = GetFileSize(h, NULL) * 2;
		CloseHandle(h);
#else
	} else {
		dwBufferSize = 0;
#endif
	}
	if (dwBufferSize == 0) return FALSE;
	*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), /*HEAP_ZERO_MEMORY*/0, dwBufferSize);
	while (*ppBuf)
	{
		CHAR cmd[MAX_PATH*2*2];
		int ret;
#ifdef UNICODE
		wsprintfA(cmd, "--i -qq \"%S\" \"%S\"", pszArchivePath, pszFileName);
#else
		int i;
		TCHAR szPlayFile[MAX_PATH*2] = {0};
		LPTSTR p = pszFileName;
		// エスケープシーケンスの処理
		for(i=0;*p;p++){
			if(IsDBCSLeadByte(*p)){
				szPlayFile[i++] = *p++;
				szPlayFile[i++] = *p;
				continue;
			}
			if(*p==TEXT('[') || *p==TEXT(']') || *p==TEXT('!') || *p==TEXT('^') || *p==TEXT('-') || *p==TEXT('\\')) szPlayFile[i++] = TEXT('\\');
			szPlayFile[i++] = *p;
		}
		wsprintfA(cmd, "--i -qq \"%s\" \"%s\"", pszArchivePath, szPlayFile);
#endif
		ret = lpUnLhaExtractMem(NULL, cmd, (LPBYTE)*ppBuf, dwBufferSize, NULL, NULL, &dwOutputSize);
#if 0
		if (iinfo.dwOriginalSize == 0 && dwBufferSize == dwOutputSize)
		{
			/* バッファが足りなそうなら再試行 */
			HeapFree(GetProcessHeap(), 0, *ppBuf);
			dwBufferSize += dwBufferSize;
			*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), /*HEAP_ZERO_MEMORY*/0, dwBufferSize);
			continue;
		}
#endif
		if(!ret){
			*pSize = dwOutputSize;
			return TRUE;
		}
#ifdef _DEBUG
		{
			TCHAR erx[64];
			wsprintf(erx,TEXT("%08x"),ret);
			MessageBox(NULL,erx,pszArchivePath,MB_OK);
		}
#endif
		HeapFree(GetProcessHeap(), 0, *ppBuf);
		return FALSE;
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
	const struct IMPORT_FUNC_TABLE *pft;
	if (!hUnlha32) hUnlha32 = LoadLibrary(szDllName);
	if (!hUnlha32) {
		TCHAR szDllPath[MAX_PATH];
		GetModuleFileName(hDLL, szDllPath, MAX_PATH);
		lstrcpy(PathFindFileName(szDllPath), szDllName);
		hUnlha32 = LoadLibrary(szDllPath);
		if (!hUnlha32) return FALSE;
	}
	for (pft = functbl; pft->lpszFuncName; pft++){
		FARPROC fp = GetProcAddress(hUnlha32, pft->lpszFuncName);
		if (!fp){
			FreeLibrary(hUnlha32);
			hUnlha32 = NULL;
			return FALSE;
		}
		*pft->ppFunc = fp;
	}
	return TRUE;
}

#ifdef __cplusplus
extern "C"
#endif
ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

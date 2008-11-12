#include "../../fittle/src/aplugin.h"
#include <shlwapi.h>
#include "../../../extra/unrar/unrar.h"

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
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif


static HMODULE hDLL = 0;
static HMODULE hUNRARDLL = 0;

#if 0
extern "C" void *memcpy(void *dest, const void *src, size_t count)
{
	CopyMemory(dest, src, count);
	return dest;
}

extern "C" void *memset( void *dest, int c, size_t count )
{
	FillMemory(dest, count, c);
	return dest;
}
#endif

#ifdef UNICODE
typedef HANDLE (PASCAL * LPFNRAROPENARCHIVEEX)(struct RAROpenArchiveDataEx *ArchiveData);
typedef int    (PASCAL * LPFNRARREADHEADEREX)(HANDLE hArcData,struct RARHeaderDataEx *HeaderData);
#else
typedef HANDLE (PASCAL * LPFNRAROPENARCHIVE)(struct RAROpenArchiveData *ArchiveData);
typedef int    (PASCAL * LPFNRARREADHEADER)(HANDLE hArcData,struct RARHeaderData *HeaderData);
#endif
typedef int    (PASCAL * LPFNRARCLOSEARCHIVE)(HANDLE hArcData);
typedef int    (PASCAL * LPFNRARPROCESSFILE)(HANDLE hArcData,int Operation,char *DestPath,char *DestName);
typedef void   (PASCAL * LPFNRARSETCALLBACK)(HANDLE hArcData,UNRARCALLBACK Callback,LPARAM UserData);

#ifdef UNICODE
static LPFNRAROPENARCHIVEEX pRAROpenArchiveEx;
static LPFNRARREADHEADEREX pRARReadHeaderEx;
#else
static LPFNRAROPENARCHIVE pRAROpenArchive;
static LPFNRARREADHEADER pRARReadHeader;
#endif
static LPFNRARCLOSEARCHIVE pRARCloseArchive;
static LPFNRARPROCESSFILE pRARProcessFile;
static LPFNRARSETCALLBACK pRARSetCallback;

static /*const*/ struct IMPORT_FUNC_TABLE {
	LPSTR/*LPCSTR*/ lpszFuncName;
	FARPROC * ppFunc;
} functbl[] = {
#ifdef UNICODE
	{ "RAROpenArchiveEx", (FARPROC *)&pRAROpenArchiveEx },
	{ "RARReadHeaderEx", (FARPROC *)&pRARReadHeaderEx },
#else
	{ "RAROpenArchive", (FARPROC *)&pRAROpenArchive },
	{ "RARReadHeader", (FARPROC *)&pRARReadHeader },
#endif
	{ "RARCloseArchive", (FARPROC *)&pRARCloseArchive },
	{ "RARProcessFile", (FARPROC *)&pRARProcessFile },
	{ "RARSetCallback", (FARPROC *)&pRARSetCallback },
	{ 0, (FARPROC *)0 }
};
static /*const*/ TCHAR szDllName[] = TEXT("unrar.dll");

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
	if(lstrcmpi(pszExt, TEXT("rar"))==0){
		return TRUE;
	}
	return FALSE;
}

static LPTSTR CALLBACK CheckArchivePath(LPTSTR pszFilePath)
{
	LPTSTR p = StrStrI(pszFilePath, TEXT(".rar/"));
	return p;
}

static int CALLBACK CallbackProcEN(UINT msg,LPARAM UserData,LPARAM P1,LPARAM P2)
{
	switch(msg)
	{
	case UCM_CHANGEVOLUME:
		return(0);
	case UCM_PROCESSDATA:
		return(0);
	case UCM_NEEDPASSWORD:
		return(0);
	}
	return(0);
}

static BOOL CALLBACK EnumArchive(LPTSTR pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	HANDLE hArcData;
	int RHCode,PFCode;
	char CmtBuf[16384];
#ifdef UNICODE
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;
#else
	struct RARHeaderData HeaderData;
	struct RAROpenArchiveData OpenArchiveData;
#endif

	ZeroMemory(&OpenArchiveData, sizeof(OpenArchiveData));
#ifdef UNICODE
	OpenArchiveData.ArcNameW=pszFilePath;
#else
	OpenArchiveData.ArcName=pszFilePath;
#endif
	OpenArchiveData.CmtBuf=CmtBuf;
	OpenArchiveData.CmtBufSize=sizeof(CmtBuf);
	OpenArchiveData.OpenMode=RAR_OM_LIST;
#ifdef UNICODE
	hArcData=pRAROpenArchiveEx(&OpenArchiveData);
#else
	hArcData=pRAROpenArchive(&OpenArchiveData);
#endif

	if (OpenArchiveData.OpenResult!=0) return FALSE;

	pRARSetCallback(hArcData,CallbackProcEN,0);

	HeaderData.CmtBuf=CmtBuf;
	HeaderData.CmtBufSize=sizeof(CmtBuf);

#ifdef UNICODE
	while ((RHCode=pRARReadHeaderEx(hArcData,&HeaderData))==0)
#else
	while ((RHCode=pRARReadHeader(hArcData,&HeaderData))==0)
#endif
	{
		FILETIME ft;
		LONGLONG ll = Int32x32To64(HeaderData.FileTime, 10000000) + 116444736000000000;
		ft.dwLowDateTime = (DWORD)ll;
		ft.dwHighDateTime = ll >> 32;
#ifdef UNICODE
		lpfnProc(HeaderData.FileNameW, HeaderData.UnpSize, ft, pData);
#else
		lpfnProc(HeaderData.FileName, HeaderData.UnpSize, ft, pData);
#endif
		if ((PFCode=pRARProcessFile(hArcData,RAR_SKIP,NULL,NULL))!=0) break;
	}

	pRARCloseArchive(hArcData);

	return TRUE;
}

typedef struct {
	LPBYTE pBuf;
	DWORD dwSize;
	LPBYTE pCur;
} WORK;

static int CALLBACK CallbackProcEX(UINT msg,LPARAM UserData,LPARAM P1,LPARAM P2)
{
	switch(msg)
	{
	case UCM_CHANGEVOLUME:
		return(0);
	case UCM_PROCESSDATA:
		{
			WORK *pWork = (WORK *)UserData;
			if (pWork->pCur + P2 <= pWork->pBuf + pWork->dwSize)
			{
				CopyMemory((PVOID)pWork->pCur, (PVOID)P1, (size_t)P2);
			}
			pWork->pCur += P2;
			return(0);
		}
	case UCM_NEEDPASSWORD:
		return(0);
	}
	return(0);
}

static BOOL CALLBACK ExtractArchive(LPTSTR pszArchivePath, LPTSTR pszFileName, void **ppBuf, DWORD *pSize)
{
	WORK work;
	HANDLE hArcData;
	int RHCode,PFCode;
	char CmtBuf[16384];
#ifdef UNICODE
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;
#else
	struct RARHeaderData HeaderData;
	struct RAROpenArchiveData OpenArchiveData;
#endif

	work.dwSize = 0;
	work.pBuf = 0;

	ZeroMemory(&OpenArchiveData, sizeof(OpenArchiveData));
#ifdef UNICODE
	OpenArchiveData.ArcNameW=pszArchivePath;
#else
	OpenArchiveData.ArcName=pszArchivePath;
#endif
	OpenArchiveData.CmtBuf=CmtBuf;
	OpenArchiveData.CmtBufSize=sizeof(CmtBuf);
	OpenArchiveData.OpenMode=RAR_OM_EXTRACT;
#ifdef UNICODE
	hArcData=pRAROpenArchiveEx(&OpenArchiveData);
#else
	hArcData=pRAROpenArchive(&OpenArchiveData);
#endif

	if (OpenArchiveData.OpenResult!=0) return FALSE;

	pRARSetCallback(hArcData, CallbackProcEX, (LPARAM)&work);

	HeaderData.CmtBuf=NULL;

#ifdef UNICODE
	while ((RHCode=pRARReadHeaderEx(hArcData,&HeaderData)) == 0)
#else
	while ((RHCode=pRARReadHeader(hArcData,&HeaderData)) == 0)
#endif
	{
#ifdef UNICODE
		if (lstrcmpi(HeaderData.FileNameW, pszFileName) == 0)
#else
		if (lstrcmpi(HeaderData.FileName, pszFileName) == 0)
#endif
		{
			work.dwSize = HeaderData.UnpSize;
			if (work.dwSize == 0){
				pRARCloseArchive(hArcData);
				return FALSE;
			}
			work.pBuf = work.dwSize ? (LPBYTE)HeapAlloc(GetProcessHeap(), 0, work.dwSize) : 0;
			work.pCur = work.pBuf;
			if (work.pBuf)
			{
				PFCode=pRARProcessFile(hArcData, RAR_TEST, NULL,NULL);
				if (PFCode!=0) work.dwSize = 0;
			}
			else
			{
				work.dwSize = 0;
			}
			break;
		} else {
			if ((PFCode=pRARProcessFile(hArcData,RAR_SKIP,NULL,NULL))!=0) break;
		}
	}
	pRARCloseArchive(hArcData);
	if (work.dwSize && work.pBuf && work.pBuf + work.dwSize == work.pCur)
	{
		*ppBuf = work.pBuf;
		*pSize = work.dwSize;
		return TRUE;
	}
	if (work.pBuf) HeapFree(GetProcessHeap(), 0, work.pBuf);
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
	if (!hUNRARDLL) hUNRARDLL = LoadLibrary(szDllName);
	if (!hUNRARDLL) {
		TCHAR szDllPath[MAX_PATH];
		GetModuleFileName(hDLL, szDllPath, MAX_PATH);
		lstrcpy(PathFindFileName(szDllPath), szDllName);
		hUNRARDLL = LoadLibrary(szDllPath);
		if (!hUNRARDLL) return FALSE;
	}
	for (pft = functbl; pft->lpszFuncName; pft++){
		FARPROC fp = GetProcAddress(hUNRARDLL, pft->lpszFuncName);
		if (!fp){
			FreeLibrary(hUNRARDLL);
			hUNRARDLL = NULL;
			return FALSE;
		}
		*pft->ppFunc = fp;
	}
	return TRUE;
}

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

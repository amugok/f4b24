#include "../../fittle/src/aplugin.h"
#include <shlwapi.h>
#include "../../../extra/unrar/unrar.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"../../../extra/unrar/unrar.lib")
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
	hArcData=RAROpenArchiveEx(&OpenArchiveData);
#else
	hArcData=RAROpenArchive(&OpenArchiveData);
#endif

	if (OpenArchiveData.OpenResult!=0) return FALSE;

	RARSetCallback(hArcData,CallbackProcEN,0);

	HeaderData.CmtBuf=CmtBuf;
	HeaderData.CmtBufSize=sizeof(CmtBuf);

#ifdef UNICODE
	while ((RHCode=RARReadHeaderEx(hArcData,&HeaderData))==0)
#else
	while ((RHCode=RARReadHeader(hArcData,&HeaderData))==0)
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
		if ((PFCode=RARProcessFile(hArcData,RAR_SKIP,NULL,NULL))!=0) break;
	}

	RARCloseArchive(hArcData);

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
	hArcData=RAROpenArchiveEx(&OpenArchiveData);
#else
	hArcData=RAROpenArchive(&OpenArchiveData);
#endif

	if (OpenArchiveData.OpenResult!=0) return FALSE;

	RARSetCallback(hArcData, CallbackProcEX, (LPARAM)&work);

	HeaderData.CmtBuf=NULL;

#ifdef UNICODE
	while ((RHCode=RARReadHeaderEx(hArcData,&HeaderData)) == 0)
#else
	while ((RHCode=RARReadHeader(hArcData,&HeaderData)) == 0)
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
				RARCloseArchive(hArcData);
				return FALSE;
			}
			work.pBuf = work.dwSize ? (LPBYTE)HeapAlloc(GetProcessHeap(), 0, work.dwSize) : 0;
			work.pCur = work.pBuf;
			if (work.pBuf)
			{
				PFCode=RARProcessFile(hArcData, RAR_TEST, NULL,NULL);
				if (PFCode!=0) work.dwSize = 0;
			}
			else
			{
				work.dwSize = 0;
			}
			break;
		} else {
			if ((PFCode=RARProcessFile(hArcData,RAR_SKIP,NULL,NULL))!=0) break;
		}
	}
	RARCloseArchive(hArcData);
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
	return TRUE;
}

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

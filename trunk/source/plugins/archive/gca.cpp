#include "../../fittle/src/aplugin.h"
#include <shlwapi.h>
#include "../../../extra/gcasdk/GcaSDK.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"../../../extra/gcasdk/GcaSDK.lib")
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
	if(lstrcmpi(pszExt, TEXT("gca"))==0){
		return TRUE;
	}
	return FALSE;
}

static LPTSTR CALLBACK CheckArchivePath(LPTSTR pszFilePath)
{
	return StrStrI(pszFilePath, TEXT(".gca/"));
}

static BOOL CALLBACK EnumArchive(LPTSTR pszArchivePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	CGca sdk;
	if (!sdk.Init()) return FALSE;
#ifdef UNICODE
	{
		CHAR nameA[MAX_PATH + 1];
		WideCharToMultiByte(CP_ACP, 0, pszArchivePath, -1, nameA, MAX_PATH + 1, NULL, NULL);
		sdk.SetArcFilePath(nameA);
	}
#else
	sdk.SetArcFilePath(pszArchivePath);
#endif
	int n = sdk.GetNumFiles();
	for (int i = 0; i < n; i++)
	{
		string fn = sdk.GetFileName(i);
		DWORD sz = (DWORD)sdk.GetFileSize(i);
		FILETIME ft = sdk.GetFileTime(i);
#ifdef UNICODE
		{
			WCHAR nameW[MAX_PATH + 1];
			MultiByteToWideChar(CP_ACP, 0, fn.c_str(), -1, nameW, MAX_PATH + 1);
			lpfnProc(nameW, sz, ft, pData);
		}
#else
		lpfnProc((LPSTR )(fn.c_str()), sz, ft, pData);
#endif
	}
	sdk.Free();
	return TRUE;
}

static int MessageProc(int, string, PVOID)
{
	return GCAM_OK;
}

static BOOL CALLBACK ExtractArchive(LPTSTR pszArchivePath, LPTSTR pszFileName, void **ppBuf, DWORD *pSize)
{
	CGca sdk;
	if (!sdk.Init()) return FALSE;
#ifdef UNICODE
	{
		CHAR nameA[MAX_PATH + 1];
		WideCharToMultiByte(CP_ACP, 0, pszArchivePath, -1, nameA, MAX_PATH + 1, NULL, NULL);
		sdk.SetArcFilePath(nameA);
	}
#else
	sdk.SetArcFilePath(pszArchivePath);
#endif
	sdk.SetMessageProc(MessageProc);
	int n = sdk.GetNumFiles();
	for (int i = 0; i < n; i++)
	{
		string fn = sdk.GetFileName(i);
		DWORD sz = (DWORD)sdk.GetFileSize(i);
		FILETIME ft = sdk.GetFileTime(i);
#ifdef UNICODE
		WCHAR nameW[MAX_PATH + 1];
		MultiByteToWideChar(CP_ACP, 0, fn.c_str(), -1, nameW, MAX_PATH + 1);
		if (lstrcmpiW(nameW, pszFileName) == 0)
#else
		if (lstrcmpiA(fn.c_str(), pszFileName) == 0)
#endif
		{
			if (sz != 0) {
				LPBYTE pBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, sz);
				if (pBuf)
				{
					if (sdk.ExtractFileToMemory(i, pBuf))
					{
						*ppBuf = pBuf;
						*pSize = sz;
						return TRUE;
					}
					HeapFree(GetProcessHeap(), 0, pBuf);
				}
			}
			break;
		}
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
	return TRUE;
}

#ifdef __cplusplus
extern "C"
#endif
ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

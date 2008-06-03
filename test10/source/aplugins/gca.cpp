#include "../fittle/src/aplugin.h"
#include <shlwapi.h>
#include "../../extra/gcasdk/GcaSDK.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"../../extra/gcasdk/GcaSDK.lib")
#pragma comment(linker, "/EXPORT:GetAPluginInfo=_GetAPluginInfo@0")
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

static BOOL CALLBACK IsArchiveExt(char *pszExt){
	if(lstrcmpi(pszExt, "gca")==0){
		return TRUE;
	}
	return FALSE;
}

static char * CALLBACK CheckArchivePath(char *pszFilePath)
{
	char *p = StrStrI(pszFilePath, ".gca/");
	return p;
}

static BOOL CALLBACK EnumArchive(char *pszArchivePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	CGca sdk;
	if (!sdk.Init()) return FALSE;
	sdk.SetArcFilePath(pszArchivePath);
	int n = sdk.GetNumFiles();
	for (int i = 0; i < n; i++)
	{
		string fn = sdk.GetFileName(i);
		DWORD sz = (DWORD)sdk.GetFileSize(i);
		FILETIME ft = sdk.GetFileTime(i);
		lpfnProc((char *)(fn.c_str()), sz, ft, pData);
	}
	sdk.Free();
	return TRUE;
}

static int MessageProc(int, string, PVOID)
{
	return GCAM_OK;
}

static BOOL CALLBACK ExtractArchive(char *pszArchivePath, char *pszFileName, void **ppBuf, DWORD *pSize)
{
	CGca sdk;
	if (!sdk.Init()) return FALSE;
	sdk.SetArcFilePath(pszArchivePath);
	sdk.SetMessageProc(MessageProc);
	int n = sdk.GetNumFiles();
	for (int i = 0; i < n; i++)
	{
		string fn = sdk.GetFileName(i);
		DWORD sz = (DWORD)sdk.GetFileSize(i);
		FILETIME ft = sdk.GetFileTime(i);
		if (lstrcmpi(fn.c_str(), pszFileName) == 0)
		{
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

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}

#define WINVER		0x0400	// 98à»ç~
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4à»ç~

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>

#include "../cplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
//#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:WinMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#define MAX_FITTLE_PATH 260*2

static CONFIG_PLUGIN_INFO *pTop = NULL;

/* ÉvÉâÉOÉCÉìÇìoò^ */
static BOOL RegisterPlugin(FARPROC lpfnProc){
	GetCPluginInfoFunc GetCPluginInfo = (GetCPluginInfoFunc)lpfnProc;
	if (GetCPluginInfo){
		CONFIG_PLUGIN_INFO *pNewPlugins = GetCPluginInfo();
		if (pNewPlugins){
			CONFIG_PLUGIN_INFO *pNext = pNewPlugins;
			do{
				pNext = pNext->pNext;
			} while (pNext);
			for (pNext = pNewPlugins; pNext->pNext; pNext = pNext->pNext);
			pNext->pNext = pTop;
			pTop = pNewPlugins;
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL LoadPlugins(char *pszPath){
	char szPathW[MAX_PATH];
	char szDllPath[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd = {0};

	wsprintf(szPathW, "%s*.fcp", pszPath);

	hFind = FindFirstFile(szPathW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			HINSTANCE hDll;
			wsprintf(szDllPath, "%s%s", pszPath, wfd.cFileName);
			hDll = LoadLibrary(szDllPath);
			if(hDll){
				FARPROC pfnCPlugin = GetProcAddress(hDll, "GetCPluginInfo");
				if (!pfnCPlugin || !RegisterPlugin(pfnCPlugin))
				{
					FreeLibrary(hDll);
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}

	return TRUE;
}


static void GetModuleParentDir(char *szParPath){
	char szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98à»ç~
	*PathFindFileName(szParPath) = '\0';
	return;
}

static void InitPlugins(){
	char szPluginPath[MAX_FITTLE_PATH];
	GetModuleParentDir(szPluginPath);
	LoadPlugins(szPluginPath);
}

static void *HAlloc(DWORD dwSize)
{
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}

static void HFree(void *pPtr)
{
	HeapFree(GetProcessHeap(), 0, pPtr);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int nMaxPage = 0;
	int nNumPage = 0;
	PROPSHEETHEADER psh;
	HPROPSHEETPAGE FAR *phpage;
	CONFIG_PLUGIN_INFO *pCur;

	InitCommonControls();
	InitPlugins();
	
	for (pCur = pTop; pCur; pCur = pCur->pNext)
		nMaxPage += pCur->GetConfigPageCount();

	phpage = (HPROPSHEETPAGE FAR *)HAlloc(sizeof(HPROPSHEETPAGE) * nMaxPage);
	if (phpage) {
		int nLevel;
		for (nLevel = 0; nLevel < 3; nLevel++){
			for (pCur = pTop; pCur; pCur = pCur->pNext){
				int nIndex = 0;
				HPROPSHEETPAGE hpspAdd;
				do{
					hpspAdd = pCur->GetConfigPage(nIndex++, nLevel);
					if (hpspAdd && nNumPage < nMaxPage){
						phpage[nNumPage++] = hpspAdd;
					}
				}while(hpspAdd);
			}
		}
		if (nNumPage > 0)
		{
			psh.dwSize = sizeof (PROPSHEETHEADER);
			psh.dwFlags = PSH_USEICONID;
			psh.hwndParent = 0;
			psh.hInstance = GetModuleHandle(NULL);
			psh.pszIcon = MAKEINTRESOURCE(1);
			psh.pszCaption = TEXT("ê›íË");
			psh.nPages = nNumPage;
			psh.nStartPage = 0;
			psh.phpage = phpage;

			PropertySheet(&psh);
		}
		HFree(phpage);
	}


	ExitProcess(0);
	return 0;
}

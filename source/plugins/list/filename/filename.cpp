#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/src/f4b24lx.h"
#include "../../ui/lplugin/lplugin.h"

#include <shlwapi.h>

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(linker, "/EXPORT:GetLXPluginInfo=_GetLXPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#define COLUMN_TYPE_ID 7

int CALLBACK GetTypeNum();
int CALLBACK GetTypeCode(int nIndex);
LPCSTR CALLBACK GetTypeName(int nIndex);
BOOL CALLBACK IsSupported(int nType);
BOOL CALLBACK InitColumnOrder(int nColumn, int nType);
void CALLBACK AddColumn(HWND hList, int nColumn, int nType);
void CALLBACK GetColumnText(LPVOID pFileInfo, int nRow, int nColumn, int nType, LPVOID pBuf, int nBufSize);
int CALLBACK CompareColumnText(LPVOID pFileInfoLeft, LPVOID pFileInfoRight, int nColumn, int nType);
void CALLBACK OnSave(HWND hList, int nColumn, int nType, int nWidth);

static LX_PLUGIN_INFO lxpinfo = {
	LPDK_VER,
	GetTypeNum,
	GetTypeCode,
	GetTypeName,
	IsSupported,
	InitColumnOrder,
	AddColumn,
	GetColumnText,
	CompareColumnText,
	OnSave
};

#ifdef __cplusplus
extern "C"
#endif
LX_PLUGIN_INFO * CALLBACK GetLXPluginInfo(void){
	return &lxpinfo;
}

static HMODULE m_hDLL = 0;

void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}

void HFree(LPVOID pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

int CALLBACK GetTypeNum(){
	return 1;
}
int CALLBACK GetTypeCode(int nIndex){
	if (nIndex == 0) return COLUMN_TYPE_ID;
	return -1;
}
LPCSTR CALLBACK GetTypeName(int nIndex){
	if (nIndex == 0) return "ファイル名(拡張子無し)";
	return "";
}
BOOL CALLBACK IsSupported(int nType){
	return nType == COLUMN_TYPE_ID;
}
BOOL CALLBACK InitColumnOrder(int nColumn, int nType){
	return IsSupported(nType);
}
void CALLBACK AddColumn(HWND hList, int nColumn, int nType){
	if (IsSupported(nType)) {
		CHAR szSec[16];
		wsprintfA(szSec, "WidthEx%d", nType);
		lxpinfo.plxif->AddColumn(hList, nColumn, lxpinfo.plxif->nUnicode ? (LPVOID)L"ファイル名" : (LPVOID)"ファイル名", lxpinfo.GetIniInt("Column", szSec, 100), LVCFMT_LEFT);
	}
}

static void GetFileNameNoExtW(LPVOID pFileInfo, LPVOID pBuf, int nBufSize){
	LPWSTR pExt;
	lstrcpynW((LPWSTR)pBuf, (LPCWSTR)lxpinfo.plxif->GetFileName(pFileInfo), nBufSize);
	if (!lxpinfo.plxif->CheckPath(pFileInfo, F4B24LX_CHECK_URL) && !lxpinfo.plxif->CheckPath(pFileInfo, F4B24LX_CHECK_CUE)){
		pExt = PathFindExtensionW((LPWSTR)pBuf);
		if (pExt) *pExt = 0;
	}
}

static void GetFileNameNoExtA(LPVOID pFileInfo, LPVOID pBuf, int nBufSize){
	LPSTR pExt;
	lstrcpynA((LPSTR)pBuf, (LPCSTR)lxpinfo.plxif->GetFileName(pFileInfo), nBufSize);
	if (!lxpinfo.plxif->CheckPath(pFileInfo, F4B24LX_CHECK_URL) && !lxpinfo.plxif->CheckPath(pFileInfo, F4B24LX_CHECK_CUE)){
		pExt = PathFindExtensionA((LPSTR)pBuf);
		if (pExt) *pExt = 0;
	}
}

void CALLBACK GetColumnText(LPVOID pFileInfo, int nRow, int nColumn, int nType, LPVOID pBuf, int nBufSize){
	if (lxpinfo.plxif->nUnicode) {
		GetFileNameNoExtW(pFileInfo, pBuf, nBufSize);
	}else{
		GetFileNameNoExtA(pFileInfo, pBuf, nBufSize);
	}
}
int CALLBACK CompareColumnText(LPVOID pFileInfoLeft, LPVOID pFileInfoRight, int nColumn, int nType){
	if (lxpinfo.plxif->nUnicode) {
		WCHAR bufL[MAX_PATH];
		WCHAR bufR[MAX_PATH];
		GetFileNameNoExtW(pFileInfoLeft, bufL, MAX_PATH);
		GetFileNameNoExtW(pFileInfoRight, bufR, MAX_PATH);
		return lstrcmpiW(bufL, bufR);
	} else {
		CHAR bufL[MAX_PATH];
		CHAR bufR[MAX_PATH];
		GetFileNameNoExtA(pFileInfoLeft, bufL, MAX_PATH);
		GetFileNameNoExtA(pFileInfoRight, bufR, MAX_PATH);
		return lstrcmpiA(bufL, bufR);
	}
}

void CALLBACK OnSave(HWND hList, int nColumn, int nType, int nWidth){
	CHAR szSec[16];
	wsprintfA(szSec, "WidthEx%d", nType);
	lxpinfo.SetIniInt("Column", szSec, nWidth);
}


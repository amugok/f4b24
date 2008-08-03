#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/gplugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/src/f4b24lx.h"
#include "lplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(linker, "/EXPORT:GetGPluginInfo=_GetGPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg);
static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode);

static GENERAL_PLUGIN_INFO gpinfo = {
	GPDK_VER,
	HookWndProc,
	OnEvent
};

#ifdef __cplusplus
extern "C"
#endif
GENERAL_PLUGIN_INFO * CALLBACK GetGPluginInfo(void){
	return &gpinfo;
}

static HMODULE m_hDLL = 0;
static F4B24LX_INTERFACE *m_plxif = 0;

#include "../../../fittle/src/wastr.cpp"

/* LXプラグインリスト */
static struct PLUGIN_NODE {
	struct PLUGIN_NODE *pNext;
	LX_PLUGIN_INFO *pInfo;
	HMODULE hDll;
} *pTop = NULL;

void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}

void HFree(LPVOID pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}

static void FreePlugins(){
	struct LX_PLUGIN_NODE *pList = pTop;
	while (pList) {
		struct LX_PLUGIN_NODE *pNext = pList->pNext;
		FreeLibrary(pList->hDll);
		HFree(pList);
		pList = pNext;
	}
	pTop = NULL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
		FreePlugins();
	}
	return TRUE;
}

typedef struct USERDATA_NODE_TAG {
	USERDATA_NODE_TAG *pNext;
	GUID guid;
	LPVOID pUserData;
	void (CALLBACK *pFreeProc)(LPVOID);
} USERDATA_NODE;

LPVOID CALLBACK CBGetUserData(LPVOID pFileInfo, LPVOID pGuid){
}
void CALLBACK CBSetUserData(LPVOID pFileInfo, LPVOID pGuid, LPVOID pUserData, void (CALLBACK *pFreeProc)(LPVOID)){
}
void CALLBACK OnAlloc(LPVOID pFileInfo){
}
void CALLBACK OnFree(LPVOID pFileInfo){
}
BOOL CALLBACK InitColumnOrder(int nColumn, int nType){
	struct LX_PLUGIN_NODE *pList = pTop;
	while (pList) {
		LX_PLUGIN_INFO *pInfo = pList->pInfo;
		if (pInfo->IsSuppported(nType)) {
			return pInfo->InitColumnOrder(nColumn, nType);
		}
		pList = pList->pNext;
	}
	return FALSE;
}
void CALLBACK AddColumn(HWND hList, int nColumn, int nType){
	struct LX_PLUGIN_NODE *pList = pTop;
	while (pList) {
		LX_PLUGIN_INFO *pInfo = pList->pInfo;
		if (pInfo->IsSuppported(nType)) {
			pInfo->AddColumn(hList, nColumn, nType);
			return;
		}
		pList = pList->pNext;
	}
}
void CALLBACK GetColumnText(LPVOID pFileInfo, int nRow, int nColumn, int nType, LPVOID pBuf, int nBufSize){
	struct LX_PLUGIN_NODE *pList = pTop;
	while (pList) {
		LX_PLUGIN_INFO *pInfo = pList->pInfo;
		if (pInfo->IsSuppported(nType)) {
			pInfo->GetColumnText(pFileInfo, nRow, nColumn, nType, pBuf, nBufSize);
			return;
		}
		pList = pList->pNext;
	}
}
int CALLBACK CompareColumnText(LPVOID pFileInfoLeft, LPVOID pFileInfoRight, int nColumn, int nType){
	struct LX_PLUGIN_NODE *pList = pTop;
	while (pList) {
		LX_PLUGIN_INFO *pInfo = pList->pInfo;
		if (pInfo->IsSuppported(nType)) {
			return pInfo->CompareColumnText(pFileInfoLeft, pFileInfoRight, nColumn, nType);
		}
		pList = pList->pNext;
	}
	return 0;
}
void CALLBACK OnSave(){
	struct LX_PLUGIN_NODE *pList = pTop;
	while (pList) {
		LX_PLUGIN_INFO *pInfo = pList->pInfo;
		pInfo->OnQuit();
		pList = pList->pNext;
	}
}


/* LXプラグインを登録 */
static BOOL CALLBACK RegisterPlugin(HMODULE hPlugin, LPVOID user){
	FARPROC lpfnProc = GetProcAddress(hPlugin, "GetLXPluginInfo");
	if (lpfnProc){
		struct LX_PLUGIN_NODE *pNewNode = (struct LX_PLUGIN_NODE *)HAlloc(sizeof(struct LX_PLUGIN_NODE));
		if (pNewNode) {
			pNewNode->pInfo = ((GetLXPluginInfoFunc)lpfnProc)();
			pNewNode->pNext = NULL;
			pNewNode->hDll = hPlugin;
			if (pNewNode->pInfo){
				LX_PLUGIN_INFO *pInfo = pNewNode->pInfo;
				pInfo->hWndMain = gpinfo.hWndMain;
				pInfo->hmodPlugin = hPlugin;
				pInfo->plxif = m_plxif;
				pInfo->GetUserData = CBGetUserData;
				pInfo->SetUserData = CBSetUserData;
				if (pTop) {
					struct LX_PLUGIN_NODE *pList;
					for (pList = pTop; pList->pNext; pList = pList->pNext);
					pList->pNext = pNewNode;
				} else {
					pTop = pNewNode;
				}
				return TRUE;
			}
			HFree(pNewNode);
		}
	}
	return FALSE;
}
















static BOOL CALLBACK HookWndProc(LPGENERAL_PLUGIN_HOOK_WNDPROC pMsg) {
	return FALSE;
}

static BOOL CALLBACK OnEvent(HWND hWnd, GENERAL_PLUGIN_EVENT eCode) {
	if (eCode == GENERAL_PLUGIN_EVENT_INIT0) {
		LONG nAccessRight;

		WAIsUnicode();

		m_plxif = (F4B24LX_INTERFACE *)SendMessage(hWnd, WM_F4B24_IPC, WM_F4B24_IPC_GET_LX_IF, 0);
		if (!m_plxif || m_plxif->nVersion != 36) return FALSE;

		nAccessRight = InterlockedExchange(&m_plxif->nAccessRight, 0);
		if (!nAccessRight) return FALSE;

		m_plxif->HookOnAlloc = OnAlloc;
		m_plxif->HookOnFree = OnFree;
		m_plxif->HookInitColumnOrder = InitColumnOrder;
		m_plxif->HookAddColumn = AddColumn;
		m_plxif->HookGetColumnText = GetColumnText;
		m_plxif->HookCompareColumnText = CompareColumnText;
		m_plxif->HookOnSave = OnSave;

		WAEnumPlugins(NULL, "Plugins\\flp\\", "*.flp", RegisterPlugin, 0);
	} else if (eCode == GENERAL_PLUGIN_EVENT_QUIT) {
		FreePlugins();
	}
	return TRUE;
}

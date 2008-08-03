#include "../../../fittle/src/fittle.h"
#include "../cplugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/src/f4b24lx.h"
#include "../../general/lplugin/lplugin.h"

#include "lplugin.rh"
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
#pragma comment(linker, "/EXPORT:GetCPluginInfo=_GetCPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static void DebugInt(int t) {
#if 1
	char x[32];
	wsprintfA(x, "lx:%d\n", t);
	OutputDebugStringA(x);
#endif
}

static DWORD CALLBACK GetConfigPageCount(void);
static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize);

static CONFIG_PLUGIN_INFO cpinfo = {
	0,
	CPDK_VER,
	GetConfigPageCount,
	GetConfigPage
};

#ifdef __cplusplus
extern "C"
#endif
CONFIG_PLUGIN_INFO * CALLBACK GetCPluginInfo(void){
	return &cpinfo;
}

static HMODULE m_hDLL = 0;

static const BYTE m_aDefaultColumns[] = { 0, 1, 2, 3 };
static int m_nNumColumns = 0;
static LPBYTE m_pColumnTable = NULL;

#include "../../../fittle/src/wastr.cpp"

/* LXプラグインリスト */
static struct LX_PLUGIN_NODE {
	struct LX_PLUGIN_NODE *pNext;
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
				pInfo->hWndMain = 0;
				pInfo->hmodPlugin = hPlugin;
				pInfo->plxif = 0;
				pInfo->GetUserData = 0;
				pInfo->SetUserData = 0;
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


int GetColumnNum(){
	return m_pColumnTable ? m_nNumColumns : (sizeof(m_aDefaultColumns)/sizeof(m_aDefaultColumns[0]));
}
int GetColumnType(int nColumn) {
	if (nColumn < GetColumnNum())
		return m_pColumnTable ? m_pColumnTable[nColumn] : m_aDefaultColumns[nColumn];
	return -1;
}

static void GetListItem(HWND hList, int nRow, LPSTR lpszBuf, int nBufSize) {
	LRESULT l = SendMessageA(hList, LB_GETTEXTLEN, (WPARAM)nRow, (LPARAM)0);
	if (l != LB_ERR) {
		LPSTR p = (LPSTR)HAlloc(sizeof(CHAR) * l + 1);
		if (p) {
			SendMessageA(hList, LB_GETTEXT, (WPARAM)nRow, (LPARAM)p);
			lstrcpynA(lpszBuf, p, nBufSize);
			HFree(p);
			return;
		}
	}
	if (nBufSize > 0) lpszBuf[0] = 0;
}

static BOOL GetListItemSel(HWND hList, int nRow) {
	return SendMessage(hList, LB_GETSEL, (WPARAM)nRow, (LPARAM)0) > 0;
}

static void SetListItemSel(HWND hList, int nRow, BOOL fSel) {
	SendMessage(hList, LB_SETSEL, (WPARAM)fSel, (LPARAM)nRow);
}

static int GetListItemData(HWND hList, int nRow) {
	return SendMessage(hList, LB_GETITEMDATA, (WPARAM)nRow, (LPARAM)0);
}

static void SetListItemData(HWND hList, int nRow, int nItemData) {
	SendMessage(hList, LB_SETITEMDATA, (WPARAM)nRow, (LPARAM)nItemData);
}

static int GetListCount(HWND hList) {
	return SendMessage(hList, LB_GETCOUNT, 0, 0);
}

static int InsertListItem(HWND hList, int nItemData, LPCSTR lpszText, int nTo) {
	int c = GetListCount(hList);
	LRESULT lSel = SendMessageA(hList, LB_INSERTSTRING, (WPARAM)((nTo >= c) ? -1 : nTo), (LPARAM)lpszText);
	if (lSel != LB_ERR && lSel != LB_ERRSPACE) {
		SetListItemData(hList, lSel, nItemData);
		return lSel;
	}
	return -1;
}

static int AddListItem(HWND hList, int nItemData, LPCSTR lpszText) {
	LRESULT lSel = SendMessageA(hList, LB_ADDSTRING, (WPARAM)0, (LPARAM)lpszText);
	if (lSel != LB_ERR && lSel != LB_ERRSPACE) {
		SetListItemData(hList, lSel, nItemData);
		return lSel;
	}
	return -1;
}

static void DeleteListItem(HWND hList, int nRow) {
	SendMessage(hList, LB_DELETESTRING, (WPARAM)nRow, (LPARAM)0);
}

static int FindListSelect(HWND hList) {
	int c = GetListCount(hList);
	int i;
	for (i = 0; i < c; i++){
		if (GetListItemSel(hList, i)) return i;
	}
	return -1;
}

static int FindListSelectR(HWND hList) {
	int c = GetListCount(hList);
	int i;
	for (i = c - 1; i >= 0; i--){
		if (GetListItemSel(hList, i)) return i;
	}
	return -1;
}

static int FindListItem(HWND hList, int nItemData) {
	int c = GetListCount(hList);
	int i;
	for (i = 0; i < c; i++){
		int d = GetListItemData(hList, i);
		if (nItemData == d) return i;
	}
	return -1;
}

static void LoadConfig(HWND hDlg){
	int nColumnNum;
	WASetIniFile(NULL, "Fittle.ini");
	nColumnNum = WAGetIniInt("Column", "ColumnNum", 0);
	if (m_pColumnTable) {
		HFree(m_pColumnTable);
		m_pColumnTable = 0;
	}
	if (nColumnNum) {
		int c;
		int nColumn;
		HWND hListRight = GetDlgItem(hDlg, IDC_LIST4);
		m_pColumnTable = (LPBYTE)HAlloc(sizeof(BYTE) * nColumnNum);
		for (c = nColumn = 0; nColumn < nColumnNum; nColumn++){
			CHAR szSec[16];
			int nType;
			wsprintfA(szSec, "ColumnType%d", nColumn);
			nType = WAGetIniInt("Column", szSec, -1);
			if (FindListItem(hListRight, nType) >= 0){
				m_pColumnTable[c++] = nType;
			}
		}
		m_nNumColumns = c;
	}
}

static BOOL CheckConfig(HWND hDlg){
	return FALSE;
}


static void SaveConfig(HWND hDlg){
}

static int MoveListItemTo(HWND hListFrom, HWND hListTo, int nFind, int nTo){
	CHAR szCaption[64];
	int nItemData = GetListItemData(hListFrom, nFind);
	GetListItem(hListFrom, nFind, szCaption, 64);
	DeleteListItem(hListFrom, nFind);
	return InsertListItem(hListTo, nItemData, szCaption, nTo);
}

static int MoveListItem(HWND hListFrom, HWND hListTo, int nFind){
	return MoveListItemTo(hListFrom, hListTo, nFind, -1);
//	CHAR szCaption[64];
//	int nItemData = GetListItemData(hListFrom, nFind);
//	GetListItem(hListFrom, nFind, szCaption, 64);
//	DeleteListItem(hListFrom, nFind);
//	return AddListItem(hListTo, nItemData, szCaption);
}


static void MoveListItemToLeft(HWND hDlg, int nType){
	HWND hListLeft = GetDlgItem(hDlg, IDC_LIST1);
	HWND hListRight = GetDlgItem(hDlg, IDC_LIST4);
	int nFind = FindListItem(hListRight, nType);
	if (nFind >= 0) {
		MoveListItem(hListRight, hListLeft, nFind);
	}
}

static void MoveList(HWND hListFrom, HWND hListTo){
	int nFind = FindListSelect(hListFrom);
	while (nFind >= 0) {
		int nAdd = MoveListItem(hListFrom, hListTo, nFind);
		if (nAdd >= 0) {
			SetListItemSel(hListTo, nAdd, TRUE);
		}
		nFind = FindListSelect(hListFrom);
	}
}

static void MoveListUp(HWND hDlg){
	int nAdd = -1;
	HWND hListLeft = GetDlgItem(hDlg, IDC_LIST1);
	int nFind = FindListSelect(hListLeft);
	while (nFind >= 1) {
		nAdd = MoveListItemTo(hListLeft, hListLeft, nFind, nFind - 1);
		nFind = FindListSelect(hListLeft);
	}
	if (nAdd >= 0) {
		SetListItemSel(hListLeft, nAdd, TRUE);
	}
}

static void MoveListDown(HWND hDlg){
	int nAdd = -1;
	HWND hListLeft = GetDlgItem(hDlg, IDC_LIST1);
	int nFind = FindListSelectR(hListLeft);
	while (nFind >= 0) {
		nAdd = MoveListItemTo(hListLeft, hListLeft, nFind, nFind + 1);
		nFind = FindListSelectR(hListLeft);
	}
	if (nAdd >= 0) {
		SetListItemSel(hListLeft, nAdd, TRUE);
	}
}


static void MoveListToLeft(HWND hDlg){
	HWND hListLeft = GetDlgItem(hDlg, IDC_LIST1);
	HWND hListRight = GetDlgItem(hDlg, IDC_LIST4);
	MoveList(hListRight, hListLeft);
}
static void MoveListToRight(HWND hDlg){
	HWND hListLeft = GetDlgItem(hDlg, IDC_LIST1);
	HWND hListRight = GetDlgItem(hDlg, IDC_LIST4);
	MoveList(hListLeft, hListRight);
}

static void ViewConfig(HWND hDlg){
	int c, i;
	struct LX_PLUGIN_NODE *pList = pTop;
	HWND hListRight = GetDlgItem(hDlg, IDC_LIST4);

	InsertListItem(hListRight, 0, "ファイル名", -1);
	InsertListItem(hListRight, 1, "サイズ", -1);
	InsertListItem(hListRight, 2, "種類", -1);
	InsertListItem(hListRight, 3, "更新日時", -1);
	
	while(pList) {
		LX_PLUGIN_INFO *pInfo = pList->pInfo;
		c = pInfo->GetTypeNum();
		for (i = 0; i < c; i++){
			InsertListItem(hListRight, pInfo->GetTypeCode(i), pInfo->GetTypeName(i), -1);
		}
		pList = pList->pNext;
	}

	LoadConfig(hDlg);

	c = GetColumnNum();
	for (i = 0; i < c; i++){
		int t = GetColumnType(i);
		MoveListItemToLeft(hDlg, t);
	}
}

static BOOL CALLBACK LXPageProc(HWND hDlg , UINT msg , WPARAM wp , LPARAM lp) {
	switch (msg){
	case WM_INITDIALOG:
		ViewConfig(hDlg);
		return TRUE;
	case WM_COMMAND:
		if(HIWORD(wp)==BN_CLICKED){
			if(LOWORD(wp)==IDC_BUTTON1){
				MoveListUp(hDlg);
			}else if(LOWORD(wp)==IDC_BUTTON2){
				MoveListToLeft(hDlg);
			}else if(LOWORD(wp)==IDC_BUTTON3){
				MoveListToRight(hDlg);
			}else if(LOWORD(wp)==IDC_BUTTON4){
				MoveListDown(hDlg);
			}
		}
		if (CheckConfig(hDlg))
			PropSheet_Changed(GetParent(hDlg) , hDlg);
		else
			PropSheet_UnChanged(GetParent(hDlg) , hDlg);
		return TRUE;

	case WM_NOTIFY:
		if (((NMHDR *)lp)->code == PSN_APPLY){
			SaveConfig(hDlg);
		}
		return TRUE;
	}
	return FALSE;
}

static DWORD CALLBACK GetConfigPageCount(void){
	return 1;
}
static HPROPSHEETPAGE CreateConfigPage(){
	PROPSHEETPAGE psp;

	psp.dwSize = sizeof (PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = m_hDLL;
	psp.pszTemplate = TEXT("LX_SHEET");
	psp.pfnDlgProc = (DLGPROC)LXPageProc;
	return CreatePropertySheetPage(&psp);
}

static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize){
	if (nIndex == 0 && nLevel == 1){
		WAIsUnicode();
		if (!pTop) WAEnumPlugins(NULL, "Plugins\\flp\\", "*.flp", RegisterPlugin, 0);

		lstrcpynA(pszConfigPath, "plugin/lplugin", nConfigPathSize);
		return CreateConfigPage();
	}
	return 0;
}

/*
 * bookmark
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "bookmark.rh"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

// �E�B���h�E���T�u�N���X���A�v���V�[�W���n���h�����E�B���h�E�Ɋ֘A�t����
#define SET_SUBCLASS(hWnd, Proc) \
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Proc))

typedef struct StringList {
	struct StringList *pNext;
	TCHAR szString[1];
} STRING_LIST, *LPSTRING_LIST;

static void LoadConfig();
static BOOL OnInit();
static void OnQuit();
static void OnTrackChange();
static void OnStatusChange();
static void OnConfig();

static BOOL CALLBACK BookMarkDlgProc(HWND, UINT, WPARAM, LPARAM);
static void DrawBookMark(HMENU);
static int AddBookMark(HMENU, HWND);
static void LoadBookMark(HMENU , LPTSTR);
static void SaveBookMark(LPTSTR);

static void HookComboUpdate(HWND);
static BOOL HookTreeRoot(HWND);

static HMODULE m_hinstDLL = 0;
static WNDPROC m_hOldProc = 0;

#define MAX_BM_SIZE 100			// ������̐�

static FITTLE_PLUGIN_INFO fpi = {
	PDK_VER,
	OnInit,
	OnQuit,
	OnTrackChange,
	OnStatusChange,
	OnConfig,
	NULL,
	NULL
};

static struct {
	LPSTRING_LIST lpBookmark;
	int nBMRoot;				// ����������[�g�Ƃ��Ĉ�����
	int nBMFullPath;			// ��������t���p�X�ŕ\��
} g_cfgbm = {
	NULL
};

static void StringListFree(LPSTRING_LIST *pList){
	LPSTRING_LIST pCur = *pList;
	while (pCur){
		LPSTRING_LIST pNext = pCur->pNext;
		HeapFree(GetProcessHeap(), 0, pCur);
		pCur = pNext;
	}
	*pList = NULL;
}

static  LPSTRING_LIST StringListWalk(LPSTRING_LIST *pList, int nIndex){
	LPSTRING_LIST pCur = *pList;
	int i = 0;
	while (pCur){
		if (i++ == nIndex) return pCur;
		pCur = pCur->pNext;
	}
	return NULL;
}

static int StringListAdd(LPSTRING_LIST *pList, LPTSTR szValue){
	int i = 0;
	LPSTRING_LIST pCur = *pList;
	LPSTRING_LIST pNew = (LPSTRING_LIST)HeapAlloc(GetProcessHeap(), 0, sizeof(STRING_LIST) + sizeof(TCHAR) * lstrlen(szValue));
	if (!pNew) return -1;
	pNew->pNext = NULL;
	lstrcpy(pNew->szString, szValue);
	if (pCur){
		i++;
		/* �����ɒǉ� */
		while (pCur->pNext){
			pCur = pCur->pNext;
			i++;
		}
		pCur->pNext = pNew;
	} else {
		/* �擪 */
		*pList = pNew;
	}
	return i;
}

static void ClearBookmark(void){
	StringListFree(&g_cfgbm.lpBookmark);
}

LPCTSTR GetBookmark(int nIndex){
	static TCHAR szNull[1] = {0};
	LPSTRING_LIST lpbm = StringListWalk(&g_cfgbm.lpBookmark, nIndex);
	return lpbm ? lpbm->szString : szNull;
}

static int AppendBookmark(LPTSTR szPath){
	if (!szPath || !*szPath) return TRUE;
	return StringListAdd(&g_cfgbm.lpBookmark, szPath);
}


void GetModuleParentDir(LPTSTR szParPath){
	TCHAR szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98�ȍ~
	*PathFindFileName(szParPath) = '\0';
	return;
}

static void UpdateDriveList(){
	SendMessage(fpi.hParent, WM_F4B24_IPC, (WPARAM)WM_F4B24_IPC_UPDATE_DRIVELIST, (LPARAM)0);
//	SetDrivesToCombo(m_hCombo);
//	SendMessage(hWnd, WM_DEVICECHANGE, 0x8000 ,0);
}

// �w�肵��������������j���[���ڂ�T��
static int GetMenuPosFromString(HMENU hMenu, LPTSTR lpszText){
	int i;
	TCHAR szBuf[64];
	int c = GetMenuItemCount(hMenu);
	for (i = 0; i < c; i++){
		GetMenuString(hMenu, i, szBuf, 64, MF_BYPOSITION);
		if (lstrcmp(lpszText, szBuf) == 0)
			return i;
	}
	return 0;
}

// �ݒ��Ǎ�
static void LoadState(){
	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INI�t�@�C���̃p�X

	// INI�t�@�C���̈ʒu���擾
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("fittle.ini"));

	// ����������[�g�Ƃ��Ĉ���
	g_cfgbm.nBMRoot = GetPrivateProfileInt(TEXT("BookMark"), TEXT("BMRoot"), 1, m_szINIPath);
	// ��������t���p�X�ŕ\��
	g_cfgbm.nBMFullPath = GetPrivateProfileInt(TEXT("BookMark"), TEXT("BMFullPath"), 1, m_szINIPath);


	// ������̓ǂݍ���
	LoadBookMark(GetSubMenu(GetMenu(fpi.hParent), GetMenuPosFromString(GetMenu(fpi.hParent), TEXT("������(&B)"))), m_szINIPath);
//	nTagReverse = GetPrivateProfileInt(TEXT("MiniPanel"), TEXT("TagReverse"), 0, m_szINIPath);
}

//Int�^�Őݒ�t�@�C����������
static int WritePrivateProfileInt(LPTSTR szAppName, LPTSTR szKeyName, int nData, LPTSTR szINIPath){
	TCHAR szTemp[100];

	wsprintf(szTemp, TEXT("%d"), nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}

// �I����Ԃ�ۑ�
static void SaveState(){
	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INI�t�@�C���̃p�X

	// INI�t�@�C���̈ʒu���擾
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("fittle.ini"));

	WritePrivateProfileInt(TEXT("BookMark"), TEXT("BMRoot"), g_cfgbm.nBMRoot, m_szINIPath);
	WritePrivateProfileInt(TEXT("BookMark"), TEXT("BMFullPath"), g_cfgbm.nBMFullPath, m_szINIPath);

	SaveBookMark(m_szINIPath);	// ������̕ۑ�

//	WritePrivateProfileInt(TEXT("MiniPanel"), TEXT("x"), nMiniPanel_x, m_szINIPath);
	WritePrivateProfileString(NULL, NULL, NULL, m_szINIPath);

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hinstDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

static LRESULT FittlePluginInterface(int nCommand){
	return SendMessage(fpi.hParent, WM_FITTLE, nCommand, 0);
}

static int IsCheckedMainMenu(int nCommand){
	HMENU hMainMenu = (HMENU)FittlePluginInterface(GET_MENU);
	return (GetMenuState(hMainMenu, nCommand, MF_BYCOMMAND) & MF_CHECKED) != 0;
}

static void SendCommandMessage(HWND hwnd, int nCommand){
	if (hwnd && IsWindow(hwnd))
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(nCommand, 0), 0);
}

static void SendCommand(int nCommand){
	SendCommandMessage(fpi.hParent, nCommand);
}


static HWND CreateWorkWindow(void){
	return CreateWindow(TEXT("STATIC"),TEXT(""),0,0,0,0,0,NULL,NULL,m_hinstDLL,NULL);
}

static void GetCurPath(LPTSTR lpszBuf, int nBufSize) {
	HWND hwndWork = CreateWorkWindow();
	if (hwndWork){
		SendMessage(fpi.hParent, WM_F4B24_IPC, (WPARAM)WM_F4B24_IPC_GET_CURPATH, (LPARAM)hwndWork);
		SendMessage(hwndWork, WM_GETTEXT, (WPARAM)nBufSize, (LPARAM)lpszBuf);
		DestroyWindow(hwndWork);
	}
}

static void SetCurPath(LPCTSTR lpszPath) {
	HWND hwndWork = CreateWorkWindow();
	if (hwndWork){
		SendMessage(hwndWork, WM_SETTEXT, (WPARAM)0, (LPARAM)lpszPath);
		SendMessage(fpi.hParent, WM_F4B24_IPC, (WPARAM)WM_F4B24_IPC_SET_CURPATH, (LPARAM)hwndWork);
		DestroyWindow(hwndWork);
	}
}

// �T�u�N���X�������E�B���h�E�v���V�[�W��
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_SYSCOMMAND:
	case WM_COMMAND:
		if(IDM_BM_FIRST<=LOWORD(wp) && LOWORD(wp)<IDM_BM_FIRST+MAX_BM_SIZE){
			LPCTSTR lpszBMPath = GetBookmark(LOWORD(wp)-IDM_BM_FIRST);
			if(!lpszBMPath[0]) break;
			SetCurPath(lpszBMPath);
			break;
		}
		switch (LOWORD(wp)){
		case IDM_BM_ADD: //������ɒǉ�
			if(AddBookMark(GetSubMenu(GetMenu(hWnd), GetMenuPosFromString(GetMenu(hWnd), TEXT("������(&B)"))), hWnd)!=-1){
				UpdateDriveList();
			}
			break;

		case IDM_BM_ORG:
			DialogBox(m_hinstDLL, TEXT("BOOKMARK_DIALOG"), hWnd, (DLGPROC)BookMarkDlgProc);
			DrawBookMark(GetSubMenu(GetMenu(hWnd), GetMenuPosFromString(GetMenu(hWnd), TEXT("������(&B)"))));
			UpdateDriveList();
			break;
		}
		break;
	case WM_F4B24_IPC:
		switch (wp){
			case WM_F4B24_HOOK_UPDATE_DRIVELISTE:
				HookComboUpdate((HWND)lp);
				break;
			case WM_F4B24_HOOK_GET_TREE_ROOT:
				if (HookTreeRoot((HWND)lp)) return TRUE;
				break;
		}
		break;
	}
	return CallWindowProc(m_hOldProc, hWnd, msg, wp, lp);
}

/* �N�����Ɉ�x�����Ă΂�܂� */
static BOOL OnInit(){
	LoadState();

	EnableMenuItem(GetMenu(fpi.hParent), IDM_BM_ADD, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(fpi.hParent), IDM_BM_ORG, MF_BYCOMMAND | MF_ENABLED);
	m_hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);

	UpdateDriveList();

	return TRUE;
}

/* �I�����Ɉ�x�����Ă΂�܂� */
static void OnQuit(){
	SaveState();
	ClearBookmark();
}

/* �Ȃ��ς�鎞�ɌĂ΂�܂� */
static void OnTrackChange(){
}

/* �Đ���Ԃ��ς�鎞�ɌĂ΂�܂� */
static void OnStatusChange(){
}

/* �ݒ��ʂ��Ăяo���܂��i�������j*/
static void OnConfig(){
}

#ifdef __cplusplus
extern "C"
{
#endif
__declspec(dllexport) FITTLE_PLUGIN_INFO *GetPluginInfo(){
	return &fpi;
}
#ifdef __cplusplus
}
#endif

// ����������j���[�ɕ`��
static void DrawBookMark(HMENU hBMMenu){
	int i;
	TCHAR szMenuBuff[MAX_FITTLE_PATH+4];

	// �����郁�j���[���폜
	for(i=0;i<MAX_BM_SIZE;i++){
		if(!DeleteMenu(hBMMenu, IDM_BM_FIRST+i, MF_BYCOMMAND)) break;
	}

	// �V�������j���[��ǉ�
	for(i=0;i<MAX_BM_SIZE;i++){
		LPCTSTR lpszBMPath = GetBookmark(i);
		if(!lpszBMPath[0]) break;
		if(g_cfgbm.nBMFullPath){
			wsprintf(szMenuBuff, TEXT("&%d: %s"), i, lpszBMPath);
		}else{
			wsprintf(szMenuBuff, TEXT("&%d: %s"), i, PathFindFileName(lpszBMPath));
		}
		AppendMenu(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuff);
	}
	return;
}

// ������ɒǉ��i���ƂŏC���j
static int AddBookMark(HMENU hBMMenu, HWND hWnd){
	TCHAR szBuf[MAX_FITTLE_PATH];
	TCHAR szMenuBuff[MAX_FITTLE_PATH+4];
	int i;

	if (*GetBookmark(MAX_BM_SIZE-1)){
		MessageBox(hWnd, TEXT("������̌��������z���܂����B"), TEXT("Fittle"), MB_OK | MB_ICONEXCLAMATION);
		return -1;
	}

	szBuf[0] = TEXT('\0');
	GetCurPath(szBuf, MAX_FITTLE_PATH);
	if (!szBuf[0]) return -1;

	i = AppendBookmark(szBuf);
	if (i < 0){
		return -1;
	}
	
	if(g_cfgbm.nBMFullPath){
		wsprintf(szMenuBuff, TEXT("&%d: %s"), i, szBuf);
	}else{
		wsprintf(szMenuBuff, TEXT("&%d: %s"), i, PathFindFileName(szBuf));
	}
	AppendMenu(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuff);
	return i;
}


/*  ���Ɏ��s����Fittle�Ƀp�����[�^�𑗐M���� */
static void SendCopyData(HWND hFittle, int iType, LPTSTR lpszString){
#ifdef UNICODE
	CHAR nameA[MAX_FITTLE_PATH];
	LPBYTE lpWork;
	COPYDATASTRUCT cds;
	DWORD la, lw, cbData;
	WideCharToMultiByte(CP_ACP, 0, lpszString, -1, nameA, MAX_FITTLE_PATH, NULL, NULL);
	la = lstrlenA(nameA) + 1;
	if (la & 1) la++; /* WORD align */
	lw = lstrlenW(lpszString) + 1;
	cbData = la * sizeof(CHAR) + lw * sizeof(WCHAR);
	lpWork = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbData);
	if (!lpWork) return;
	lstrcpyA((LPSTR)(lpWork), nameA);
	lstrcpyW((LPWSTR)(lpWork + la), lpszString);
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpWork;
	cds.cbData = cbData;
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
	HeapFree(GetProcessHeap(), 0, lpWork);
#else
	COPYDATASTRUCT cds;
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpszString;
	cds.cbData = (lstrlenA(lpszString) + 1) * sizeof(CHAR);
	// �����񑗐M
	SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
#endif
}

static BOOL CALLBACK BookMarkDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM /*lp*/){
	static HWND hList = NULL;
	int i;
	LVITEM item;

	switch (msg)
	{
		case WM_INITDIALOG:	// ������
			hList = GetDlgItem(hDlg, IDC_LIST1);

			LVCOLUMN lvcol;
			ZeroMemory(&lvcol, sizeof(LVCOLUMN));
			lvcol.mask = LVCF_FMT;
			lvcol.fmt = LVCFMT_LEFT;
			ListView_InsertColumn(hList, 0, &lvcol);

			ZeroMemory(&item, sizeof(LVITEM));
			item.mask = LVIF_TEXT;
			item.iSubItem = 0;
			int nMax;
			nMax = 300;

			for(i=0;i<MAX_BM_SIZE;i++){
				TCHAR szBMPath[MAX_FITTLE_PATH];
				lstrcpyn(szBMPath, GetBookmark(i), MAX_FITTLE_PATH);
				if(!szBMPath[0]) break;
				item.pszText = szBMPath;
				nMax = ListView_GetStringWidth(hList, item.pszText)+10>nMax?ListView_GetStringWidth(hList, item.pszText)+10:nMax;
				item.iItem = i;
				ListView_InsertItem(hList, &item);
			}
			ListView_SetColumnWidth(hList, 0, nMax);
			ListView_SetItemState(hList, 0, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
			SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)(g_cfgbm.nBMRoot?BST_CHECKED:BST_UNCHECKED), 0);
			SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, (WPARAM)(g_cfgbm.nBMFullPath?BST_CHECKED:BST_UNCHECKED), 0);
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDOK:
					int nCount;

					ClearBookmark();
					nCount = ListView_GetItemCount(hList);
					for(i=0;i<nCount;i++){
						TCHAR szBuf[MAX_FITTLE_PATH];
						ListView_GetItemText(hList, i, 0, szBuf, MAX_FITTLE_PATH);
						AppendBookmark(szBuf);
					}
					g_cfgbm.nBMRoot = SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
					g_cfgbm.nBMFullPath = SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);
					EndDialog(hDlg, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, -1);
					return TRUE;

				case IDC_BUTTON1:	// ���
					int nSel;
					TCHAR szPath[MAX_FITTLE_PATH], szSub[MAX_FITTLE_PATH];

					nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
					if(nSel<=0) return FALSE;
					ListView_GetItemText(hList, nSel-1, 0, szSub, MAX_FITTLE_PATH);
					ListView_GetItemText(hList, nSel, 0, szPath, MAX_FITTLE_PATH);
					ListView_SetItemText(hList, nSel-1, 0, szPath);
					ListView_SetItemText(hList, nSel, 0, szSub);
					ListView_SetItemState(hList, nSel-1, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
					return TRUE;

				case IDC_BUTTON2:	// ����
					nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
					if(nSel+1==ListView_GetItemCount(hList)) return FALSE;
					ListView_GetItemText(hList, nSel+1, 0, szSub, MAX_FITTLE_PATH);
					ListView_GetItemText(hList, nSel, 0, szPath, MAX_FITTLE_PATH);
					ListView_SetItemText(hList, nSel+1, 0, szPath);
					ListView_SetItemText(hList, nSel, 0, szSub);
					ListView_SetItemState(hList, nSel+1, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
					return TRUE;

				case IDC_BUTTON3:
					nSel = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
					if(nSel<0) return FALSE;
					ListView_DeleteItem(hList, nSel);
					return TRUE;

			}
			return FALSE;

		
		case WM_CLOSE:	//�I��
			EndDialog(hDlg, -1);
			return TRUE;

		default:
			return FALSE;
	}
}

// �������ǂݍ���
static void LoadBookMark(HMENU hBMMenu, LPTSTR pszINIPath){
	int i;
	TCHAR szSec[10];
	TCHAR szMenuBuff[MAX_FITTLE_PATH+4];

	ClearBookmark();
	for(i=0;i<MAX_BM_SIZE;i++){
		int j;
		wsprintf(szSec, TEXT("Path%d"), i);
		szMenuBuff[0] = 0;
		GetPrivateProfileString(TEXT("BookMark"), szSec, TEXT(""), szMenuBuff, MAX_FITTLE_PATH, pszINIPath);
		if(!szMenuBuff[0]) break;
		j =  AppendBookmark(szMenuBuff);
		if (j >= 0){
			LPCTSTR lpszBMPath = GetBookmark(j);
			if(g_cfgbm.nBMFullPath){
				wsprintf(szMenuBuff, TEXT("&%d: %s"), j, lpszBMPath);
			}else{
				wsprintf(szMenuBuff, TEXT("&%d: %s"), j, PathFindFileName(lpszBMPath));
			}
			AppendMenu(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+j, szMenuBuff);
		}
	}
	return;
}

// �������ۑ�
static void SaveBookMark(LPTSTR pszINIPath){
	int i;
	TCHAR szSec[10];

	for(i=0;i<MAX_BM_SIZE;i++){
		LPCTSTR lpszBMPath = GetBookmark(i);
		wsprintf(szSec, TEXT("Path%d"), i);
		WritePrivateProfileString(TEXT("BookMark"), szSec, lpszBMPath[0] ? lpszBMPath : NULL, pszINIPath);
		if (!lpszBMPath[0]) break;
	}
	return;
}

static void HookComboUpdate(HWND hCB){
	int i,j;
	TCHAR szDrawBuff[MAX_FITTLE_PATH];

	COMBOBOXEXITEM citem = {0};
	citem.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	citem.cchTextMax = MAX_FITTLE_PATH;

	// ������t�H���_��
	i = SendMessage(hCB, CB_GETCOUNT, 0, 0);
	for(j=0;j<MAX_BM_SIZE;j++){
		LPCTSTR lpszBMPath = GetBookmark(j);
		if (!lpszBMPath[0]) break;
		if((lpszBMPath[0]==TEXT('\\') && lpszBMPath[1]==TEXT('\\')) || PathIsDirectory(lpszBMPath)){
			lstrcpyn(szDrawBuff, lpszBMPath, MAX_FITTLE_PATH);
			PathAddBackslash(szDrawBuff);
			citem.pszText = szDrawBuff;
			citem.iItem = i;
			//TODO
			citem.lParam = citem.iImage = citem.iSelectedImage = -1;
			SendMessage(hCB, CBEM_INSERTITEM, 0, (LPARAM)&citem);
			i++;
//		}else if(IsPlayList(lpszBMPath) || IsArchive(lpszBMPath)){
		} else {
			wsprintf(szDrawBuff, TEXT("%s"), lpszBMPath);
			citem.pszText = szDrawBuff;
			citem.iItem = i;
			citem.lParam = citem.iImage = citem.iSelectedImage = -1;
			SendMessage(hCB, CBEM_INSERTITEM, 0, (LPARAM)&citem);
			i++;
		}
	}
}



static BOOL HookTreeRoot(HWND hwndWork){
	int i;
	TCHAR szSetPath[MAX_FITTLE_PATH];
	TCHAR szTempRoot[MAX_FITTLE_PATH];
	
	szTempRoot[0] = TEXT('\0');
	SendMessage(hwndWork, WM_GETTEXT, (WPARAM)MAX_FITTLE_PATH, (LPARAM)szSetPath);

	// �h���C�u�{�b�N�X�̕ύX
	if(g_cfgbm.nBMRoot){
		for(i=0;i<MAX_BM_SIZE;i++){
			LPCTSTR lpszBMPath = GetBookmark(i);
			// ������I���Ŕ�����
			if(!lpszBMPath[0]) break;

			if(StrStrI(szSetPath, lpszBMPath)){
				int len = lstrlen(lpszBMPath);
				if(len>lstrlen(szTempRoot) && (szSetPath[len]==TEXT('\0') || szSetPath[len]==TEXT('\\'))){
					lstrcpyn(szTempRoot, lpszBMPath, MAX_FITTLE_PATH);
				}
			}
		}
	}
	if (!szTempRoot[0]) return FALSE;

	SendMessage(hwndWork, WM_SETTEXT, (WPARAM)0, (LPARAM)szTempRoot);	
	return TRUE;
}

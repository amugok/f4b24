/*
 * addurl
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/plugin.h"
#include <shlwapi.h>

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#define MAX_FITTLE_PATH 260*2
#define FILE_EXIST(X) (GetFileAttributes(X)==0xFFFFFFFF ? FALSE : TRUE)
#define IsURLPath(X) StrStr(X, "://")

static BOOL OnInit();
static void OnQuit();
static void OnTrackChange();
static void OnStatusChange();
static void OnConfig();

static BOOL CALLBACK AddURLDlgProc(HWND, UINT, WPARAM, LPARAM);
static void AddURL(HWND hWnd);

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

static WNDPROC hOldProc = 0;
static HMODULE hDLL = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
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

static void UpdateMenuItems(HMENU hMenu){
	UINT uState = GetMenuState(hMenu, IDM_LIST_MOVETOP, MF_BYCOMMAND);
	if (uState == (UINT)-1) return;
	uState = GetMenuState(hMenu, IDM_LIST_URL, MF_BYCOMMAND);
	if (uState == (UINT)-1){
		int c = GetMenuItemCount(hMenu);
		int i, p = -1;
		for (i = 0; i < c; i++){
			if (GetMenuItemID(hMenu, i) == IDM_LIST_MOVETOP){
				p = i;
				break;
			}
		}
		if (p >= 0){
			InsertMenu(hMenu, p, MF_STRING | MF_BYPOSITION, IDM_LIST_URL, "�A�h���X��ǉ�(&U)\tCtrl+U");
		}else{
			AppendMenu(hMenu, MF_STRING, IDM_LIST_URL, "�A�h���X��ǉ�(&U)\tCtrl+U");
		}
	}else{
		EnableMenuItem(hMenu, IDM_LIST_URL, MF_BYCOMMAND | MF_ENABLED);
	}
//        MENUITEM "�A�h���X��ǉ�(&U)\tCtrl+U",  IDM_LIST_URL
  //      MENUITEM "��ԏ�Ɉړ�(&T)",            IDM_LIST_MOVETOP

}

// �T�u�N���X�������E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
	case WM_COMMAND:
		if(LOWORD(wp)==IDM_LIST_URL){
			AddURL(hWnd);
			return 0;
		}
		break;
	case WM_INITMENUPOPUP:
		UpdateMenuItems((HMENU)wp);
		DeleteMenu((HMENU)wp, IDM_LIST_DELFILE, MF_BYCOMMAND);
		break;
	}
	return CallWindowProc(hOldProc, hWnd, msg, wp, lp);
}

/* �N�����Ɉ�x�����Ă΂�܂� */
static BOOL OnInit(){
	hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);
	return TRUE;
}

/* �I�����Ɉ�x�����Ă΂�܂� */
static void OnQuit(){
	return;
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

static BOOL CALLBACK AddURLDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	static char *pszText;
	char szTemp[MAX_FITTLE_PATH];

	switch(msg)
	{		
		case WM_INITDIALOG:
			pszText = (char *)lp;

			// �N���b�v�{�[�h�ɗL����URI�������Ă���΃G�f�B�b�g�{�b�N�X�ɃR�s�[
			if(IsClipboardFormatAvailable(CF_TEXT)){
				if(OpenClipboard(NULL)){
					HANDLE hMem = GetClipboardData(CF_TEXT);
					LPTSTR pMem = (LPTSTR)GlobalLock(hMem);
					lstrcpyn((LPTSTR)(LPCTSTR)szTemp, pMem, MAX_FITTLE_PATH);
					GlobalUnlock(hMem);
					CloseClipboard();
				}
				if(IsURLPath(szTemp) || FILE_EXIST(szTemp)){
					SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), szTemp);
					SendMessage(GetDlgItem(hDlg, IDC_EDIT1), EM_SETSEL, (WPARAM)0, (LPARAM)lstrlen(szTemp));
				}
			}
			SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)"�A�h���X��ǉ�");
			SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDOK:	// �ݒ�ۑ�
					GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), szTemp, MAX_FITTLE_PATH);
					if(lstrlen(szTemp)==0){
						EndDialog(hDlg, FALSE);
						return TRUE;
					}else{
						lstrcpyn(pszText, szTemp, MAX_FITTLE_PATH);
						EndDialog(hDlg, TRUE);
					}
					return TRUE;

				case IDCANCEL:	// �ݒ��ۑ������ɏI��
					EndDialog(hDlg, FALSE);
					return TRUE;
			}
			return FALSE;

		// �I��
		case WM_CLOSE:
			EndDialog(hDlg, FALSE);
			return FALSE;

		default:
			return FALSE;
	}
}

static void AddURL(HWND hWnd){
	char szLabel[MAX_FITTLE_PATH];
	szLabel[0] = '\0';
	if(DialogBoxParam(hDLL, "TAB_NAME_DIALOG", hWnd, AddURLDlgProc, (LPARAM)szLabel)){
		COPYDATASTRUCT cds;
		cds.dwData = 1;
		cds.cbData = lstrlen(szLabel) + 1;
		cds.lpData = szLabel;
		SendMessage(fpi.hParent, WM_COPYDATA, (WPARAM)fpi.hParent, (LPARAM)&cds);
	}
}

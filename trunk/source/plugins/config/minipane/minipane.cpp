#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/f4b24.h"
#include "../cplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
//#pragma comment(lib,"gdi32.lib")
//#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(linker, "/EXPORT:GetCPluginInfo=_GetCPluginInfo@0")
#pragma comment(linker, "/EXPORT:OldMode=_OldMode@4")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static HMODULE hDLL = 0;

/*�ݒ�֌W*/
static int nTrayClick[6];			// �N���b�N���̓���
static int nTagReverse;			// �^�C�g���A�A�[�e�B�X�g�𔽓]
static int nMiniTop;
static int nMiniScroll;
static int nMiniTimeShow;
static int nMiniToolShow;

static DWORD CALLBACK GetConfigPageCount(void);
static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize);

static CONFIG_PLUGIN_INFO cpinfo = {
	0,
	CPDK_VER,
	GetConfigPageCount,
	GetConfigPage
};


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

// ���s�t�@�C���̃p�X���擾
void GetModuleParentDir(char *szParPath){
	char szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98�ȍ~
	*PathFindFileName(szParPath) = '\0';
	return;
}

// �����^�Őݒ�t�@�C����������
static int WritePrivateProfileInt(char *szAppName, char *szKeyName, int nData, char *szINIPath){
	char szTemp[100];

	wsprintf(szTemp, "%d", nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}

// �ݒ��Ǎ�
static void LoadConfig(){
	char m_szINIPath[MAX_FITTLE_PATH];	// INI�t�@�C���̃p�X

	// INI�t�@�C���̈ʒu���擾
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, "minipane.ini");

	// �N���b�N���̓���
	nTrayClick[0] = GetPrivateProfileInt("MiniPanel", "Click0", 0, m_szINIPath);
	nTrayClick[1] = GetPrivateProfileInt("MiniPanel", "Click1", 6, m_szINIPath);
	nTrayClick[2] = GetPrivateProfileInt("MiniPanel", "Click2", 8, m_szINIPath);
	nTrayClick[3] = GetPrivateProfileInt("MiniPanel", "Click3", 0, m_szINIPath);
	nTrayClick[4] = GetPrivateProfileInt("MiniPanel", "Click4", 5, m_szINIPath);
	nTrayClick[5] = GetPrivateProfileInt("MiniPanel", "Click5", 0, m_szINIPath);

	// �^�O�𔽓]
	nTagReverse = GetPrivateProfileInt("MiniPanel", "TagReverse", 0, m_szINIPath);

	nMiniTop = (int)GetPrivateProfileInt("MiniPanel", "TopMost", 1, m_szINIPath);
	nMiniScroll = (int)GetPrivateProfileInt("MiniPanel", "Scroll", 1, m_szINIPath);
	nMiniTimeShow = (int)GetPrivateProfileInt("MiniPanel", "TimeShow", 1, m_szINIPath);
	nMiniToolShow = (int)GetPrivateProfileInt("MiniPanel", "ToolShow", 1, m_szINIPath);
}

// �ݒ��ۑ�
static void SaveConfig(){
	int i;
	char szSec[10];

	char m_szINIPath[MAX_FITTLE_PATH];	// INI�t�@�C���̃p�X

	// INI�t�@�C���̈ʒu���擾
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, "minipane.ini");

	// �^�X�N�g���C��ۑ�
	for(i=0;i<6;i++){
		wsprintf(szSec, "Click%d", i);
		WritePrivateProfileInt("MiniPanel", szSec, nTrayClick[i], m_szINIPath); //�z�b�g�L�[
	}

	WritePrivateProfileInt("MiniPanel", "TagReverse", nTagReverse, m_szINIPath);

	WritePrivateProfileInt("MiniPanel", "TopMost", nMiniTop, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "Scroll", nMiniScroll, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "TimeShow", nMiniTimeShow, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "ToolShow", nMiniToolShow, m_szINIPath);
	WritePrivateProfileString(NULL, NULL, NULL, m_szINIPath);
}

static void ViewConfig(HWND hDlg){
	int i;
	SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)nMiniTop, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, (WPARAM)nTagReverse, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, (WPARAM)nMiniTimeShow, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, (WPARAM)nMiniScroll, 0);
	SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, (WPARAM)nMiniToolShow, 0);
	for(i=0;i<6;i++){
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)0, (LPARAM)"�Ȃ�");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)1, (LPARAM)"�Đ�");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)2, (LPARAM)"�ꎞ��~");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)3, (LPARAM)"��~");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)4, (LPARAM)"�O�̋�");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)5, (LPARAM)"���̋�");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)6, (LPARAM)"�E�B���h�E�\��/��\��");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)7, (LPARAM)"�I��");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)8, (LPARAM)"���j���[�\��");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)9, (LPARAM)"�Đ�/�ꎞ��~");
		SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_SETCURSEL, (WPARAM)nTrayClick[i], (LPARAM)0);
	}
}

static bool CheckConfig(HWND hDlg){
	int i;
	if (nMiniTop !=      (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0)) return true;
	if (nTagReverse !=   (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0)) return true;
	if (nMiniTimeShow != (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0)) return true;
	if (nMiniScroll !=   (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0)) return true;
	if (nMiniToolShow != (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0)) return true;
	for(i=0;i<6;i++){
		if (nTrayClick[i] != SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) return true;
	}
	return false;
}

static void ApplyConfig(HWND hDlg){
	int i;
	nMiniTop =      (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
	nTagReverse =   (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0);
	nMiniTimeShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0);
	nMiniScroll =   (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0);
	nMiniToolShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0);
	for(i=0;i<6;i++){
		nTrayClick[i] = SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
	}
}

static void ApplyFittle(){
	HWND hFittle = FindWindow("Fittle", NULL);
	if (hFittle && IsWindow(hFittle)){
		HWND hMiniPanel = (HWND)SendMessage(hFittle, WM_FITTLE, GET_MINIPANEL, 0);
		if (hMiniPanel && IsWindow(hMiniPanel)){
			PostMessage(hMiniPanel, WM_F4B24_IPC, WM_F4B24_IPC_APPLY_CONFIG, 0);
		}
	}
}

static BOOL CALLBACK MiniPanePageProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp) {
	switch (msg){
	case WM_INITDIALOG:
		LoadConfig();
		ViewConfig(hWnd);
	case WM_COMMAND:
		if (CheckConfig(hWnd))
			PropSheet_Changed(GetParent(hWnd) , hWnd);
		else
			PropSheet_UnChanged(GetParent(hWnd) , hWnd);
		return TRUE;

	case WM_NOTIFY:
		if (((NMHDR *)lp)->code == PSN_APPLY){
			ApplyConfig(hWnd);
			SaveConfig();
			ApplyFittle();
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
	psp.hInstance = hDLL;
	psp.pszTemplate = TEXT("MINIPANE_SHEET");
	psp.pfnDlgProc = (DLGPROC)MiniPanePageProc;
	return CreatePropertySheetPage(&psp);
}

static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize){
	if (nIndex == 0 && nLevel == 1){
		lstrcpyn(pszConfigPath, "plugin/minipane", nConfigPathSize);
		return CreateConfigPage();
	}
	return 0;
}

#ifdef __cplusplus
extern "C"
#endif
CONFIG_PLUGIN_INFO * CALLBACK GetCPluginInfo(void){
	return &cpinfo;
}


#ifdef __cplusplus
extern "C"
#endif
void CALLBACK OldMode(HWND hWnd){
	HWND hTab = PropSheet_GetTabControl(hWnd);
	int nTab = TabCtrl_GetItemCount(hTab);
	BOOL fFound = FALSE;
	int i;
	for (i = 0; i < nTab; i++){
		TCHAR szBuf[32];
		TCITEM tci;
		tci.mask = TCIF_TEXT;
		tci.pszText = szBuf;
		tci.cchTextMax = 32;
		if (TabCtrl_GetItem(hTab, i, &tci)){
			if (lstrcmp(tci.pszText, "�~�j�p�l��") == 0){
				fFound = TRUE;
			}
		}
	}
	if (!fFound){
		HPROPSHEETPAGE hPage = CreateConfigPage();
		if (hPage){
			PropSheet_AddPage(hWnd, hPage);
		}
	}
}


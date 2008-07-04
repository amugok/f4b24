#define WINVER		0x0400	// 98以降
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400	// IE4以降
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/src/oplugin.h"
#include "../cplugin.h"
#include "output.rh"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(linker, "/EXPORT:GetCPluginInfo=_GetCPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

// ウィンドウをサブクラス化、プロシージャハンドルをウィンドウに関連付ける
#define SET_SUBCLASS(hWnd, Proc) \
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Proc))

void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}

void HFree(LPVOID pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}

/* 出力プラグインリスト */
static struct OUTPUT_PLUGIN_NODE {
	struct OUTPUT_PLUGIN_NODE *pNext;
	OUTPUT_PLUGIN_INFO *pInfo;
	HMODULE hDll;
} *pTop = NULL;

/* 出力プラグインを開放 */
static void FreeOutputPlugins(){
	struct OUTPUT_PLUGIN_NODE *pList = pTop;
	while (pList) {
		struct OUTPUT_PLUGIN_NODE *pNext = pList->pNext;
		FreeLibrary(pList->hDll);
		HFree(pList);
		pList = pNext;
	}
	pTop = NULL;
}

/* 出力プラグインを登録 */
static BOOL CALLBACK RegisterOutputPlugin(HMODULE hPlugin){
	FARPROC lpfnProc = GetProcAddress(hPlugin, "GetOPluginInfo");
	if (lpfnProc){
		struct OUTPUT_PLUGIN_NODE *pNewNode = (struct OUTPUT_PLUGIN_NODE *)HAlloc(sizeof(struct OUTPUT_PLUGIN_NODE));
		if (pNewNode) {
			pNewNode->pInfo = ((GetOPluginInfoFunc)lpfnProc)();
			pNewNode->pNext = NULL;
			pNewNode->hDll = hPlugin;
			if (pNewNode->pInfo){
				if (pTop) {
					struct OUTPUT_PLUGIN_NODE *pList;
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

static BOOL m_fIsUnicode = FALSE;
static HMODULE m_hDLL = 0;
static int m_nDefualtDeviceId = 0;

#define WA_MAX_SIZE MAX_PATH
#define WAIsUnicode m_fIsUnicode
#include "../../../fittle/src/wastr.h"


static struct {
	int nOutputDevice;		// 出力デバイス
	int nOut32bit;				// 32bit(float)で出力する
	int nFadeOut;				// 停止時にフェードアウトする
	int nReplayGainMode;		// ReplayGainの適用方法
	int nReplayGainMixer;		// 音量増幅方法
	int nReplayGainPreAmp;		// PreAmp(%)
	int nReplayGainPostAmp;		// PostAmp(%)
} m_cfg;				// 設定構造体

static DWORD CALLBACK GetConfigPageCount(void);
static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, LPSTR pszConfigPath, int nConfigPathSize);

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


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		m_hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	} else if (fdwReason == DLL_PROCESS_DETACH){
		FreeOutputPlugins();
	}
	return TRUE;
}

static void PostF4B24Message(WPARAM wp, LPARAM lp){
	HWND hFittle = FindWindow(TEXT("Fittle"), NULL);
	if (hFittle && IsWindow(hFittle)){
		PostMessage(hFittle, WM_F4B24_IPC, wp, lp);
	}
}

static LRESULT SendF4B24Message(WPARAM wp, LPARAM lp){
	HWND hFittle = FindWindow(TEXT("Fittle"), NULL);
	if (hFittle && IsWindow(hFittle)){
		return SendMessage(hFittle, WM_F4B24_IPC, wp, lp);
	}
	return 0;
}

static void ApplyFittle(){
	PostF4B24Message(WM_F4B24_IPC_APPLY_CONFIG, 0);
}


// グローバルな設定を読み込む
static void LoadConfig(){
	int i;
	CHAR szSec[10];

	WASetIniFile(NULL, "fittle.ini");

	// 音声出力デバイス
	m_cfg.nOutputDevice = WAGetIniInt("Main", "OutputDevice", 0);
	// 32bit(float)で出力する
	m_cfg.nOut32bit = WAGetIniInt("Main", "Out32bit", 0);
	// 停止時にフェードアウトする
	m_cfg.nFadeOut = WAGetIniInt("Main", "FadeOut", 1);
	// ReplayGainの適用方法
	m_cfg.nReplayGainMode = WAGetIniInt("ReplayGain", "Mode", 5);
	// 音量増幅方法
	m_cfg.nReplayGainMixer = WAGetIniInt("ReplayGain", "Mixer", 1);
	// PreAmp(%)
	m_cfg.nReplayGainPreAmp = WAGetIniInt("ReplayGain", "PreAmp", 100);
	// PostAmp(%)
	m_cfg.nReplayGainPostAmp = WAGetIniInt("ReplayGain", "PostAmp", 100);

}

// 設定を保存
static void SaveConfig(){
	int i;
	CHAR szSec[10];

	WASetIniFile(NULL, "fittle.ini");

	WASetIniInt("Main", "OutputDevice", m_cfg.nOutputDevice);
	WASetIniInt("Main", "Out32bit", m_cfg.nOut32bit);
	WASetIniInt("Main", "FadeOut", m_cfg.nFadeOut);
	WASetIniInt("ReplayGain", "Mode", m_cfg.nReplayGainMode);
	WASetIniInt("ReplayGain", "Mixer", m_cfg.nReplayGainMixer);
	WASetIniInt("ReplayGain", "PreAmp", m_cfg.nReplayGainPreAmp);
	WASetIniInt("ReplayGain", "PostAmp", m_cfg.nReplayGainPostAmp);

	WAFlushIni();
}

static BOOL OutputCheckChanged(HWND hDlg){
	int nSelectDeviceId = SendDlgItemMessage(hDlg, IDC_COMBO5, CB_GETITEMDATA, (WPARAM)SendDlgItemMessage(hDlg, IDC_COMBO5, CB_GETCURSEL, (WPARAM)0, (LPARAM)0), (LPARAM)0);
	if (nSelectDeviceId == -1) nSelectDeviceId = 0;
	if (nSelectDeviceId != m_cfg.nOutputDevice){
		if (m_cfg.nOutputDevice != 0 || m_nDefualtDeviceId != nSelectDeviceId)
			return TRUE;
	}
	if (m_cfg.nOut32bit != (int)SendDlgItemMessage(hDlg, IDC_CHECK13, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nFadeOut != (int)SendDlgItemMessage(hDlg, IDC_CHECK14, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nReplayGainMode != SendDlgItemMessage(hDlg, IDC_COMBO2, CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) return TRUE;
	if (m_cfg.nReplayGainMixer != SendDlgItemMessage(hDlg, IDC_COMBO3, CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) return TRUE;
	if (m_cfg.nReplayGainPreAmp != SendDlgItemMessage(hDlg, IDC_SPIN2, UDM_GETPOS, (WPARAM)0, (LPARAM)0)) return TRUE;
	if (m_cfg.nReplayGainPostAmp != SendDlgItemMessage(hDlg, IDC_SPIN1, UDM_GETPOS, (WPARAM)0, (LPARAM)0)) return TRUE;
	return FALSE;
}

static void UpdatePreAmp(HWND hDlg){
	switch (SendDlgItemMessage(hDlg, IDC_COMBO2, CB_GETCURSEL, (WPARAM)0, (LPARAM)0))
	{
	case 0:
	case 2:
	case 4:
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT3), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_SPIN2), FALSE);
		break;
	default:
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT3), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_SPIN2), TRUE);
		break;
	}
}

static BOOL CALLBACK OutputSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	int i;
	switch(msg)
	{
		case WM_INITDIALOG:

			LoadConfig();
		

			if (SendF4B24Message(WM_F4B24_IPC_GET_CAPABLE, WM_F4B24_IPC_GET_CAPABLE_LP_FLOATOUTPUT) == WM_F4B24_IPC_GET_CAPABLE_RET_NOT_SUPPORTED){
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK13), FALSE);
			}
			SendDlgItemMessage(hDlg, IDC_CHECK13, BM_SETCHECK, (WPARAM)m_cfg.nOut32bit, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK14, BM_SETCHECK, (WPARAM)m_cfg.nFadeOut, 0);

			SendDlgItemMessage(hDlg, IDC_COMBO2, CB_INSERTSTRING, (WPARAM)0, (LPARAM)TEXT("off"));
			SendDlgItemMessage(hDlg, IDC_COMBO2, CB_INSERTSTRING, (WPARAM)1, (LPARAM)TEXT("album gain"));
			SendDlgItemMessage(hDlg, IDC_COMBO2, CB_INSERTSTRING, (WPARAM)2, (LPARAM)TEXT("album peak"));
			SendDlgItemMessage(hDlg, IDC_COMBO2, CB_INSERTSTRING, (WPARAM)3, (LPARAM)TEXT("track gain"));
			SendDlgItemMessage(hDlg, IDC_COMBO2, CB_INSERTSTRING, (WPARAM)4, (LPARAM)TEXT("track peak"));
			SendDlgItemMessage(hDlg, IDC_COMBO2, CB_INSERTSTRING, (WPARAM)5, (LPARAM)TEXT("album gain limited by peak"));
			SendDlgItemMessage(hDlg, IDC_COMBO2, CB_INSERTSTRING, (WPARAM)6, (LPARAM)TEXT("track gain limited by peak"));
			SendDlgItemMessage(hDlg, IDC_COMBO2, CB_SETCURSEL, (WPARAM)m_cfg.nReplayGainMode, (LPARAM)0);

			SendDlgItemMessage(hDlg, IDC_COMBO3, CB_INSERTSTRING, (WPARAM)0, (LPARAM)TEXT("内蔵"));
			SendDlgItemMessage(hDlg, IDC_COMBO3, CB_INSERTSTRING, (WPARAM)1, (LPARAM)TEXT("増幅のみ内蔵"));
			SendDlgItemMessage(hDlg, IDC_COMBO3, CB_INSERTSTRING, (WPARAM)2, (LPARAM)TEXT("BASS(増幅不可)"));
			SendDlgItemMessage(hDlg, IDC_COMBO3, CB_SETCURSEL, (WPARAM)m_cfg.nReplayGainMixer, (LPARAM)0);

			SendDlgItemMessage(hDlg, IDC_SPIN1, UDM_SETRANGE, (WPARAM)0, (LPARAM)MAKELONG(999, 1));
			SendDlgItemMessage(hDlg, IDC_SPIN1, UDM_SETPOS, (WPARAM)0, (LPARAM)MAKELONG(m_cfg.nReplayGainPostAmp, 0));
			SendDlgItemMessage(hDlg, IDC_SPIN2, UDM_SETRANGE, (WPARAM)0, (LPARAM)MAKELONG(999, 1));
			SendDlgItemMessage(hDlg, IDC_SPIN2, UDM_SETPOS, (WPARAM)0, (LPARAM)MAKELONG(m_cfg.nReplayGainPreAmp, 0));

			UpdatePreAmp(hDlg);

			if (pTop){
				struct OUTPUT_PLUGIN_NODE *pList = pTop;
				int nSelextIndex = -1;
				int nDefaultIndex = -1;
				DWORD dwDefaultDevice = 0xffffffff;
				do {
					int i;
					OUTPUT_PLUGIN_INFO *pPlugin = pList->pInfo;
					DWORD dwDefaultId = pPlugin->GetDefaultDeviceID();
					if (dwDefaultDevice > dwDefaultId) dwDefaultDevice = dwDefaultId;
					int n = pPlugin->GetDeviceCount();
					for (i = 0; i < n; i++) {
						CHAR name[100];
						DWORD dwId = pPlugin->GetDeviceID(i);
						if (pPlugin->GetDeviceNameA(dwId, name, 100)) {
							int nIndex = SendDlgItemMessageA(hDlg, IDC_COMBO5, CB_ADDSTRING, (WPARAM)0, (LPARAM)name);
							if (nIndex != CB_ERR && nIndex != CB_ERRSPACE) {
								SendDlgItemMessage(hDlg, IDC_COMBO5, CB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)dwId);
								if (dwId == dwDefaultDevice) {
									nDefaultIndex = nIndex;
									m_nDefualtDeviceId = dwId;
								}
								if (m_cfg.nOutputDevice != 0 && dwId == m_cfg.nOutputDevice) {
									nSelextIndex = nIndex;
								}
							}
						}
					}
					pList = pList->pNext;
				} while (pList);
				SendDlgItemMessage(hDlg, IDC_COMBO5, CB_SETCURSEL, (WPARAM)(nSelextIndex >= 0) ? nSelextIndex : (nDefaultIndex >= 0) ? nDefaultIndex : 0, (LPARAM)0);
			}

			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				int nSelectDeviceId = SendDlgItemMessage(hDlg, IDC_COMBO5, CB_GETITEMDATA, (WPARAM)SendDlgItemMessage(hDlg, IDC_COMBO5, CB_GETCURSEL, (WPARAM)0, (LPARAM)0), (LPARAM)0);
				if (nSelectDeviceId == -1) nSelectDeviceId = 0;
				if (nSelectDeviceId != m_nDefualtDeviceId)
					m_cfg.nOutputDevice = nSelectDeviceId;
				else
					m_cfg.nOutputDevice = 0;
				m_cfg.nOut32bit = (int)SendDlgItemMessage(hDlg, IDC_CHECK13, BM_GETCHECK, 0, 0);
				m_cfg.nFadeOut = (int)SendDlgItemMessage(hDlg, IDC_CHECK14, BM_GETCHECK, 0, 0);
				m_cfg.nReplayGainMode = (int)SendDlgItemMessage(hDlg, IDC_COMBO2, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				m_cfg.nReplayGainMixer = (int)SendDlgItemMessage(hDlg, IDC_COMBO3, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				m_cfg.nReplayGainPostAmp = (int)SendDlgItemMessage(hDlg, IDC_SPIN1, UDM_GETPOS, (WPARAM)0, (LPARAM)0);
				m_cfg.nReplayGainPreAmp = (int)SendDlgItemMessage(hDlg, IDC_SPIN2, UDM_GETPOS, (WPARAM)0, (LPARAM)0);
				SaveConfig();
				ApplyFittle();
				return TRUE;
			}
			return FALSE;

		case WM_COMMAND:
			if(LOWORD(wp)==IDC_BUTTON1){
				PostF4B24Message(WM_F4B24_IPC_INVOKE_OUTPUT_PLUGIN_SETUP, 0);
				return TRUE;
			}
			if(LOWORD(wp)==IDC_COMBO2){
				UpdatePreAmp(hDlg);
			}
			if (OutputCheckChanged(hDlg))
				PropSheet_Changed(GetParent(hDlg) , hDlg);
			else
				PropSheet_UnChanged(GetParent(hDlg) , hDlg);
			return TRUE;

		default:
			return FALSE;
	}
}


static DWORD CALLBACK GetConfigPageCount(void){
	return 1;
}

#ifndef PROPSHEETPAGE_V1
#define PROPSHEETPAGEA_V1 PROPSHEETPAGEA
#define PROPSHEETPAGEW_V1 PROPSHEETPAGEW
#endif
static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, LPSTR pszConfigPath, int nConfigPathSize){
	union {
		PROPSHEETPAGEA_V1 A;
		PROPSHEETPAGEW_V1 W;
	} psp;
	WASTR dlgtemp;
	LPCSTR lpszTemplate, lpszPath;
	DLGPROC lpfnDlgProc = 0;
	m_fIsUnicode = ((GetVersion() & 0x80000000) == 0);
	if (!pTop) WAEnumPlugins(NULL, "", "*.fop", RegisterOutputPlugin);
	if (nLevel == 1){
		if (nIndex == 0){
			lpszTemplate = "OUTPUT_SHEET";
			lpfnDlgProc = (DLGPROC)OutputSheetProc;
			lpszPath = "fittle/output";
		}
	}
	if (!lpfnDlgProc) return NULL;
	lstrcpynA(pszConfigPath, lpszPath, nConfigPathSize);
	WAstrcpyA(&dlgtemp, lpszTemplate);;
	if (WAIsUnicode) {
		psp.W.dwSize = sizeof (PROPSHEETPAGEW_V1);
		psp.W.dwFlags = PSP_DEFAULT;
		psp.W.hInstance = m_hDLL;
		psp.W.pszTemplate = dlgtemp.W;
		psp.W.pfnDlgProc = (DLGPROC)lpfnDlgProc;
		return CreatePropertySheetPageW(&psp.W);
	} else {
		psp.A.dwSize = sizeof (PROPSHEETPAGEA_V1);
		psp.A.dwFlags = PSP_DEFAULT;
		psp.A.hInstance = m_hDLL;
		psp.A.pszTemplate = dlgtemp.A;
		psp.A.pfnDlgProc = (DLGPROC)lpfnDlgProc;
		return CreatePropertySheetPageA(&psp.A);
	}
}

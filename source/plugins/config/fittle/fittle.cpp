#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/plugin.h"
#include "../cplugin.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
//#pragma comment(lib,"gdi32.lib")
//#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(linker, "/EXPORT:GetCPluginInfo=_GetCPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

#define WM_USER_MINIPANEL (WM_USER + 88)

static HMODULE hDLL = 0;

static DWORD CALLBACK GetConfigPageCount(void);
static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel);

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

// 実行ファイルのパスを取得
void GetModuleParentDir(char *szParPath)
{
	char szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98以降
	*PathFindFileName(szParPath) = '\0';
	return;
}

// 整数型で設定ファイル書き込み
static int WritePrivateProfileInt(char *szAppName, char *szKeyName, int nData, char *szINIPath)
{
	char szTemp[100];

	wsprintf(szTemp, "%d", nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}





static BOOL OpenFilerPath(char *szPath, HWND hWnd, LPCSTR pszMsg){
	OPENFILENAME ofn;
	char szFile[MAX_FITTLE_PATH] = {0};
	char szFileTitle[MAX_FITTLE_PATH] = {0};

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "実行ファイル(*.exe)\0*.exe\0\0";
	ofn.lpstrFile = szFile;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.lpstrInitialDir = szPath;
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = MAX_FITTLE_PATH;
	ofn.nMaxFileTitle = MAX_FITTLE_PATH;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "exe";
	ofn.lpstrTitle = pszMsg;

	if(GetOpenFileName(&ofn)==0) return FALSE;
	lstrcpyn(szPath, szFile, MAX_FITTLE_PATH);
	return TRUE;
}


static void ShowSettingDialog(HWND hWnd, int nStart){
	PROPSHEETPAGE psp;
	PROPSHEETHEADER psh;
	HPROPSHEETPAGE hPsp[6];

	UnRegHotKey(hWnd);
	ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = m_hInst;

	psp.pszTemplate = "GENERAL_SHEET";
	psp.pfnDlgProc = (DLGPROC)GeneralSheetProc;
	hPsp[0] = CreatePropertySheetPage(&psp);

	psp.pszTemplate = "PATH_SHEET";
	psp.pfnDlgProc = (DLGPROC)PathSheetProc;
	hPsp[1] = CreatePropertySheetPage(&psp);

	psp.pszTemplate = "CONTROL_SHEET";
	psp.pfnDlgProc = (DLGPROC)ControlSheetProc;
	hPsp[2] = CreatePropertySheetPage(&psp);

	psp.pszTemplate = "TASKTRAY_SHEET";
	psp.pfnDlgProc = (DLGPROC)TaskTraySheetProc;
	hPsp[3] = CreatePropertySheetPage(&psp);

	psp.pszTemplate = "HOTKEY_SHEET";
	psp.pfnDlgProc = (DLGPROC)HotKeySheetProc;
	hPsp[4] = CreatePropertySheetPage(&psp);

	psp.pszTemplate = "ABOUT_SHEET";
	psp.pfnDlgProc = (DLGPROC)AboutSheetProc;
	hPsp[5] = CreatePropertySheetPage(&psp);

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_DEFAULT| PSH_NOAPPLYNOW | 0x02000000/*PSH_NOCONTEXTHELP*/;
	psh.hwndParent = hWnd;
	psh.pszCaption = "設定";
	psh.nPages = 6;
	psh.nStartPage = nStart;
	psh.phpage = hPsp;
	PropertySheet(&psh);
	RegHotKey(hWnd);
	return;
}


static BOOL CALLBACK GeneralSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg)
	{
		case WM_INITDIALOG:
			int i;
			char szBuff[3];
			SendDlgItemMessage(hDlg, IDC_CHECK12, BM_SETCHECK, (WPARAM)g_cfg.nExistCheck, 0);
			/*if(!g_cfg.nExistCheck){
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK1), FALSE);
			}*/
			SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)g_cfg.nTimeInList, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, (WPARAM)g_cfg.nTagReverse, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, (WPARAM)g_cfg.nResume, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK10, BM_SETCHECK, (WPARAM)g_cfg.nResPosFlag, 0);
			if(!g_cfg.nResume){
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK10), FALSE);
			}
			SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, (WPARAM)g_cfg.nHighTask, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, (WPARAM)g_cfg.nCloseMin, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, (WPARAM)g_cfg.nZipSearch, 0);
			for(i=1;i<=60;i++){
				wsprintf(szBuff, "%d", i);
				SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, (WPARAM)0, (LPARAM)szBuff);
			}
			SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)g_cfg.nSeekAmount-1, (LPARAM)0);

			DWORD floatable; // floating-point channel support? 0 = no, else yes
			floatable = BASS_StreamCreate(44100, 1, BASS_SAMPLE_FLOAT, NULL, 0); // try creating FP stream
			if (floatable){
				BASS_StreamFree(floatable); // floating-point channels are supported!
			}else{
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK13), FALSE);
			}
			SendDlgItemMessage(hDlg, IDC_CHECK13, BM_SETCHECK, (WPARAM)g_cfg.nOut32bit, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK14, BM_SETCHECK, (WPARAM)g_cfg.nFadeOut, 0);
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				g_cfg.nExistCheck = (int)SendDlgItemMessage(hDlg, IDC_CHECK12, BM_GETCHECK, 0, 0);
				g_cfg.nTimeInList = (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
				g_cfg.nTagReverse = (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0);
				g_cfg.nResume = (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0);
				g_cfg.nResPosFlag = (int)SendDlgItemMessage(hDlg, IDC_CHECK10, BM_GETCHECK, 0, 0);
				g_cfg.nHighTask = (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0);
				if(g_cfg.nHighTask){
					SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);		
				}else{
					SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);	
				}
				g_cfg.nCloseMin = (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0);
				g_cfg.nZipSearch = (int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);

				g_cfg.nSeekAmount = GetDlgItemInt(hDlg, IDC_COMBO1, NULL, FALSE);
				g_cfg.nOut32bit = (int)SendDlgItemMessage(hDlg, IDC_CHECK13, BM_GETCHECK, 0, 0);
				g_cfg.nFadeOut = (int)SendDlgItemMessage(hDlg, IDC_CHECK14, BM_GETCHECK, 0, 0);
				return TRUE;
			}
			return FALSE;

		case WM_COMMAND:
			if(HIWORD(wp)==BN_CLICKED){
				if(LOWORD(wp)==IDC_CHECK3){
					if(SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0)){
						EnableWindow(GetDlgItem(hDlg, IDC_CHECK10), TRUE);
					}else{
						EnableWindow(GetDlgItem(hDlg, IDC_CHECK10), FALSE);
					}
				}/*else if(LOWORD(wp)==IDC_CHECK12){
					if(SendDlgItemMessage(hDlg, IDC_CHECK12, BM_GETCHECK, 0, 0)){
						EnableWindow(GetDlgItem(hDlg, IDC_CHECK1), TRUE);
					}else{
						EnableWindow(GetDlgItem(hDlg, IDC_CHECK1), FALSE);
					}
				}*/
			}
			return FALSE;

		default:
			return FALSE;
	}
}

static BOOL CALLBACK PathSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	char szBuff[MAX_FITTLE_PATH];

	switch(msg)
	{
		case WM_INITDIALOG:
			int i;

			SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), g_cfg.szStartPath);
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT3), g_cfg.szFilerPath);
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT4), g_cfg.szToolPath);
			szBuff[0] = '\0';
			for(i=0;i<MAX_EXT_COUNT;i++){
				if(g_cfg.szTypeList[i][0]=='\0') break;
				lstrcat(szBuff, g_cfg.szTypeList[i]);
				lstrcat(szBuff, " ");
			}
			SetWindowText(GetDlgItem(hDlg, IDC_STATIC4), szBuff);

			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				GetDlgItemText(hDlg, IDC_EDIT1, g_cfg.szStartPath, MAX_FITTLE_PATH);
				GetDlgItemText(hDlg, IDC_EDIT3, g_cfg.szFilerPath, MAX_FITTLE_PATH);
				GetDlgItemText(hDlg, IDC_EDIT4, g_cfg.szToolPath, MAX_FITTLE_PATH);
				return TRUE;
			}
			return FALSE;
		
		case WM_COMMAND:
			if(LOWORD(wp)==IDC_BUTTON1){
				BROWSEINFO bi;
				LPITEMIDLIST pidl;

				ZeroMemory(&bi, sizeof(BROWSEINFO));
				bi.hwndOwner = GetParent(hDlg);
				bi.lpszTitle = "起動時に開くフォルダを選択してください";
				pidl = SHBrowseForFolder(&bi);
				if(pidl){
					SHGetPathFromIDList(pidl, szBuff);
					CoTaskMemFree(pidl);
					SetDlgItemText(hDlg, IDC_EDIT1, szBuff);
				}
				return TRUE;
			}else if(LOWORD(wp)==IDC_BUTTON2){
				// TODO 一緒にする
				GetWindowText(GetDlgItem(hDlg, IDC_EDIT3), szBuff, MAX_FITTLE_PATH);
				if(OpenFilerPath(szBuff, hDlg, "ファイラを選択してください")){
					SetWindowText(GetDlgItem(hDlg, IDC_EDIT3), szBuff);
				}
			}else if(LOWORD(wp)==IDC_BUTTON3){
				GetWindowText(GetDlgItem(hDlg, IDC_EDIT4), szBuff, MAX_FITTLE_PATH);
				if(OpenFilerPath(szBuff, hDlg, "外部ツールを選択してください")){
					SetWindowText(GetDlgItem(hDlg, IDC_EDIT4), szBuff);
				}
			}
			return FALSE;

		default:
			return FALSE;

	}
}

static BOOL CALLBACK ControlSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	static int s_nFontHeight;
	static int s_nFontStyle;
	static char s_szFontName[32];
	static int s_nBkColor;
	static int s_nTextColor;
	static int s_nPlayTxtCol;
	static int s_nPlayBkCol;
	int i;

	switch(msg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)g_cfg.nTreeIcon, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, (WPARAM)g_cfg.nHideShow, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, (WPARAM)g_cfg.nAllSub, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, (WPARAM)g_cfg.nPathTip, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, (WPARAM)g_cfg.nGridLine, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, (WPARAM)g_cfg.nShowHeader, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK7, BM_SETCHECK, (WPARAM)g_cfg.nTabHide, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK8, BM_SETCHECK, (WPARAM)g_cfg.nTabBottom, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK9, BM_SETCHECK, (WPARAM)g_cfg.nSingleExpand, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK11, BM_SETCHECK, (WPARAM)g_cfg.nTabMulti, 0);
			SendDlgItemMessage(hDlg, IDC_RADIO1 + g_cfg.nPlayView, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			s_nFontHeight = g_cfg.nFontHeight;
			s_nFontStyle = g_cfg.nFontStyle;
			lstrcpyn(s_szFontName, g_cfg.szFontName, 32);
			s_nTextColor = g_cfg.nTextColor;
			s_nBkColor = g_cfg.nBkColor;
			s_nPlayTxtCol = g_cfg.nPlayTxtCol;
			s_nPlayBkCol = g_cfg.nPlayBkCol;
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				g_cfg.nTreeIcon = (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
				if(g_cfg.nTreeIcon) RefreshComboIcon(m_hCombo);
				InitTreeIconIndex(m_hCombo, m_hTree, (BOOL)g_cfg.nTreeIcon);
				g_cfg.nHideShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0);
				g_cfg.nAllSub = (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0);
				g_cfg.nPathTip = (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0);
				g_cfg.nGridLine = (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0);
				g_cfg.nShowHeader = (int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);
				g_cfg.nTabHide = (int)SendDlgItemMessage(hDlg, IDC_CHECK7, BM_GETCHECK, 0, 0);
				g_cfg.nTabBottom = (int)SendDlgItemMessage(hDlg, IDC_CHECK8, BM_GETCHECK, 0, 0);
				g_cfg.nSingleExpand = (int)SendDlgItemMessage(hDlg, IDC_CHECK9, BM_GETCHECK, 0, 0);	
				g_cfg.nTabMulti = (int)SendDlgItemMessage(hDlg, IDC_CHECK11, BM_GETCHECK, 0, 0);	
				for(i=0;i<4;i++){
					if(SendDlgItemMessage(hDlg, IDC_RADIO1+i, BM_GETCHECK, 0, 0)){
						g_cfg.nPlayView = i;
					}
				}
				g_cfg.nTextColor = s_nTextColor;
				g_cfg.nBkColor = s_nBkColor;
				g_cfg.nPlayTxtCol = s_nPlayTxtCol;
				g_cfg.nPlayBkCol = s_nPlayBkCol;
				SetUIColor();
				lstrcpyn(g_cfg.szFontName, s_szFontName, 32);
				g_cfg.nFontHeight = s_nFontHeight;
				g_cfg.nFontStyle = s_nFontStyle;
				SetUIFont();
				return TRUE;
			}
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDC_BUTTON2:
					CHOOSEFONT cf;
					LOGFONT lf;
					HDC hDC;

					hDC = GetDC(m_hTree);
					ZeroMemory(&lf, sizeof(LOGFONT));
					lstrcpyn(lf.lfFaceName, s_szFontName, 32);
					lf.lfHeight = -MulDiv(s_nFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
					lf.lfItalic = (g_cfg.nFontStyle&ITALIC_FONTTYPE?TRUE:FALSE);
					lf.lfWeight = (g_cfg.nFontStyle&BOLD_FONTTYPE?FW_BOLD:0);

					ReleaseDC(m_hTree, hDC);

					ZeroMemory(&cf, sizeof(CHOOSEFONT));
					lf.lfCharSet = SHIFTJIS_CHARSET;
					cf.lStructSize = sizeof(CHOOSEFONT);
					cf.hwndOwner = hDlg;
					cf.lpLogFont = &lf;
					cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
					cf.nFontType = SCREEN_FONTTYPE;
					if(ChooseFont(&cf)){
						lstrcpyn(s_szFontName, lf.lfFaceName, 32);
						s_nFontStyle = cf.nFontType;
						s_nFontHeight = cf.iPointSize / 10;
					}
					break;

				case IDC_BUTTON3:
				case IDC_BUTTON4:
				case IDC_BUTTON6:
				case IDC_BUTTON7:
					CHOOSECOLOR cc;
					COLORREF cr;
					DWORD dwCustColors[16];
					
					cr = (COLORREF)0x000000;
					ZeroMemory(&cc, sizeof(CHOOSECOLOR));
					ZeroMemory(dwCustColors, sizeof(DWORD)*16);
					cc.lStructSize = sizeof(CHOOSECOLOR);
					cc.hwndOwner = hDlg;
					cc.lpCustColors = dwCustColors;
					cc.Flags = CC_RGBINIT;
					cc.rgbResult = cr;
					if(ChooseColor(&cc)){
						cr = cc.rgbResult;
						switch(LOWORD(wp)){
							case IDC_BUTTON3:
								s_nBkColor = (int)cr;
								break;
							case IDC_BUTTON4:
                                s_nTextColor = (int)cr;
								break;
							case IDC_BUTTON6:
								s_nPlayTxtCol = (int)cr;
								break;
							case IDC_BUTTON7:
								s_nPlayBkCol = (int)cr;
								break;
						}
					}
					break;

				case IDC_BUTTON5:	// 標準に戻す
					s_nBkColor = (int)GetSysColor(COLOR_WINDOW);
					s_nTextColor = (int)GetSysColor(COLOR_WINDOWTEXT);
					s_nPlayTxtCol = (int)RGB(0xFF, 0, 0);
					s_nPlayBkCol = (int)RGB(230, 234, 238);
					s_szFontName[0] = '\0';
					break;

				default:
					break;
			}
			break;

		default:
			return FALSE;

	}
	return FALSE;
}

static BOOL CALLBACK HotKeySheetProc(HWND hDlg, UINT msg, WPARAM /*wp*/, LPARAM lp){
	switch(msg)
	{
		case WM_INITDIALOG:
			int i;
			for(i=0;i<HOTKEY_COUNT;i++){
				SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_SETRULES, (WPARAM)HKCOMB_NONE, MAKELPARAM(HOTKEYF_ALT, 0));
				SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_SETHOTKEY, (WPARAM)g_cfg.nHotKey[i], 0);
			}
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				for(i=0;i<HOTKEY_COUNT;i++){
					g_cfg.nHotKey[i] = (int)SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_GETHOTKEY, 0, 0);
				}
				return TRUE;
			}
			return FALSE;

		default:
			return FALSE;
	}
}

// エディットボックスのプロシージャ
static LRESULT CALLBACK ClickableURLProc(HWND hSC, UINT msg, WPARAM wp, LPARAM lp){
	char szURL[MAX_FITTLE_PATH] = {0};

	switch(msg){
		case WM_PAINT:
			HDC hdc;
			LOGFONT lf;
			HFONT hFont;
			COLORREF cr;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hSC, &ps);
			ZeroMemory(&lf, sizeof(LOGFONT));
			lstrcpy(lf.lfFaceName , "ＭＳ Ｐゴシック");
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfUnderline = TRUE;
			lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			hFont = CreateFontIndirect(&lf);
			SelectObject(hdc, (HGDIOBJ)hFont);
			cr = GetSysColor(COLOR_MENU);
			SetBkColor(hdc, cr);
			SetTextColor(hdc, RGB(0, 0, 255));
			SetBkMode(hdc, TRANSPARENT);
			GetWindowText(hSC, szURL, MAX_FITTLE_PATH);
			TextOut(hdc, 0, 0, szURL, lstrlen(szURL));
			DeleteObject(hFont);
			EndPaint(hSC, &ps);
			return 0;

		case WM_LBUTTONUP:
			GetWindowText(hSC, szURL, MAX_FITTLE_PATH);
			ShellExecute(HWND_DESKTOP, "open", szURL, NULL, NULL, SW_SHOWNORMAL);
			return 0;

		case WM_SETCURSOR:
			HCURSOR hCursor;
			hCursor = LoadCursor(NULL, MAKEINTRESOURCE(32649));
            SetCursor(hCursor);
			return 0;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hSC, GWLP_USERDATA), hSC, msg, wp, lp);
}


static BOOL CALLBACK AboutSheetProc(HWND hDlg, UINT msg, WPARAM /*wp*/, LPARAM lp){
	static HICON hIcon = NULL;
	switch(msg)
	{
		case WM_INITDIALOG:
			// DLLアイコン設定
			SHFILEINFO shfi;
			SHGetFileInfo(".dll", FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(SHFILEINFO),
				SHGFI_USEFILEATTRIBUTES | SHGFI_ICON);
			hIcon = shfi.hIcon;
			SendDlgItemMessage(hDlg, IDC_STATIC2, STM_SETICON, (LPARAM)hIcon, (LPARAM)0);
			SET_SUBCLASS(GetDlgItem(hDlg, IDC_STATIC3), ClickableURLProc);
			SET_SUBCLASS(GetDlgItem(hDlg, IDC_STATIC4), ClickableURLProc);
			SetDlgItemText(hDlg, IDC_STATIC0, FITTLE_VERSION);
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				DestroyIcon(hIcon);			
				return TRUE;
			}
			return FALSE;

		default:
			return FALSE;
	}
}

static BOOL CALLBACK TaskTraySheetProc(HWND hDlg, UINT msg, WPARAM /*wp*/, LPARAM lp){
	int i;
	switch(msg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg, IDC_RADIO1 + g_cfg.nTrayOpt, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_RADIO7 + g_cfg.nInfoTip, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			for(i=0;i<6;i++){
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)0, (LPARAM)"なし");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)1, (LPARAM)"再生");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)2, (LPARAM)"一時停止");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)3, (LPARAM)"停止");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)4, (LPARAM)"前の曲");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)5, (LPARAM)"次の曲");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)6, (LPARAM)"ウィンドウ表示/非表示");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)7, (LPARAM)"終了");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)8, (LPARAM)"メニュー表示");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)9, (LPARAM)"再生/一時停止");
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_SETCURSEL, (WPARAM)g_cfg.nTrayClick[i], (LPARAM)0);
			}
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				if(SendDlgItemMessage(hDlg, IDC_RADIO1, BM_GETCHECK, 0, 0)){
					if(m_bTrayFlag){
						Shell_NotifyIcon(NIM_DELETE, &m_ni);
						m_bTrayFlag = FALSE;
					}
					g_cfg.nTrayOpt = 0;
				}else if(SendDlgItemMessage(hDlg, IDC_RADIO2, BM_GETCHECK, 0, 0)){
					if(m_bTrayFlag){
						Shell_NotifyIcon(NIM_DELETE, &m_ni);
						m_bTrayFlag = FALSE;
					}
					g_cfg.nTrayOpt = 1;
				}else{
					if(g_cfg.nTrayOpt!=2)
						SetTaskTray(GetParent(m_hStatus));
					g_cfg.nTrayOpt = 2;
				}
				if(SendDlgItemMessage(hDlg, IDC_RADIO7, BM_GETCHECK, 0, 0)){
					g_cfg.nInfoTip = 0;
				}else if(SendDlgItemMessage(hDlg, IDC_RADIO8, BM_GETCHECK, 0, 0)){
					g_cfg.nInfoTip = 1;
				}else{
					g_cfg.nInfoTip = 2;
				}
				for(int i=0;i<6;i++){
					g_cfg.nTrayClick[i] = SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				}
				return TRUE;
			}
			return FALSE;

		default:
			return FALSE;
	}
}



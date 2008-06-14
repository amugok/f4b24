#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/f4b24.h"
#include "../../../fittle/src/plugin.h"
#include "../cplugin.h"
#include "fittle.rh"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"shlwapi.lib")
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

static HMODULE hDLL = 0;
static struct CONFIG g_cfg;				// 設定構造体

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
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
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

// 実行ファイルのパスを取得
void GetModuleParentDir(LPTSTR szParPath){
	TCHAR szPath[MAX_FITTLE_PATH];

	GetModuleFileName(NULL, szPath, MAX_FITTLE_PATH);
	GetLongPathName(szPath, szParPath, MAX_FITTLE_PATH); // 98以降
	*PathFindFileName(szParPath) = '\0';
	return;
}

// 整数型で設定ファイル書き込み
static int WritePrivateProfileInt(LPTSTR szAppName, LPTSTR szKeyName, int nData, LPTSTR szINIPath){
	TCHAR szTemp[100];

	wsprintf(szTemp, TEXT("%d"), nData);
	return WritePrivateProfileString(szAppName, szKeyName, szTemp, szINIPath);
}

// グローバルな設定を読み込む
static void LoadConfig(){
	int i;
	TCHAR szSec[10];

	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("fittle.ini"));

	// コントロールカラー
	g_cfg.nBkColor = (int)GetPrivateProfileInt(TEXT("Color"), TEXT("BkColor"), (int)GetSysColor(COLOR_WINDOW), m_szINIPath);
	g_cfg.nTextColor = (int)GetPrivateProfileInt(TEXT("Color"), TEXT("TextColor"), (int)GetSysColor(COLOR_WINDOWTEXT), m_szINIPath);
	g_cfg.nPlayTxtCol = (int)GetPrivateProfileInt(TEXT("Color"), TEXT("PlayTxtCol"), (int)RGB(0xFF, 0, 0), m_szINIPath);
	g_cfg.nPlayBkCol = (int)GetPrivateProfileInt(TEXT("Color"), TEXT("PlayBkCol"), (int)RGB(230, 234, 238), m_szINIPath);
	// 表示方法
	g_cfg.nPlayView = GetPrivateProfileInt(TEXT("Color"), TEXT("PlayView"), 1, m_szINIPath);

	// システムの優先度の設定
	g_cfg.nHighTask = GetPrivateProfileInt(TEXT("Main"), TEXT("Priority"), 0, m_szINIPath);
	// グリッドライン
	g_cfg.nGridLine = GetPrivateProfileInt(TEXT("Main"), TEXT("GridLine"), 1, m_szINIPath);
	g_cfg.nSingleExpand = GetPrivateProfileInt(TEXT("Main"), TEXT("SingleExp"), 0, m_szINIPath);
	// 存在確認
	g_cfg.nExistCheck = GetPrivateProfileInt(TEXT("Main"), TEXT("ExistCheck"), 0, m_szINIPath);
	// プレイリストで更新日時を取得する
	g_cfg.nTimeInList = GetPrivateProfileInt(TEXT("Main"), TEXT("TimeInList"), 0, m_szINIPath);
	// システムイメージリストを結合
	g_cfg.nTreeIcon = GetPrivateProfileInt(TEXT("Main"), TEXT("TreeIcon"), TRUE, m_szINIPath);
	// タスクトレイに収納のチェック
	g_cfg.nTrayOpt = GetPrivateProfileInt(TEXT("Main"), TEXT("Tray"), 1, m_szINIPath);
	// ツリー表示設定
	g_cfg.nHideShow = GetPrivateProfileInt(TEXT("Main"), TEXT("HideShow"), 0, m_szINIPath);
	// タブを下に表示する
	g_cfg.nTabBottom = GetPrivateProfileInt(TEXT("Main"), TEXT("TabBottom"), 0, m_szINIPath);
	// 多段で表示する
	g_cfg.nTabMulti = GetPrivateProfileInt(TEXT("Main"), TEXT("TabMulti"), 1, m_szINIPath);
	// すべてのフォルダが～
	g_cfg.nAllSub = GetPrivateProfileInt(TEXT("Main"), TEXT("AllSub"), 0, m_szINIPath);
	// ツールチップでフルパスを表示
	g_cfg.nPathTip = GetPrivateProfileInt(TEXT("Main"), TEXT("PathTip"), 1, m_szINIPath);
	// 曲名お知らせ機能
	g_cfg.nInfoTip = GetPrivateProfileInt(TEXT("Main"), TEXT("Info"), 1, m_szINIPath);
	// タグを反転
	g_cfg.nTagReverse = GetPrivateProfileInt(TEXT("Main"), TEXT("TagReverse"), 0, m_szINIPath);
	// ヘッダコントロールを表示する
	g_cfg.nShowHeader = GetPrivateProfileInt(TEXT("Main"), TEXT("ShowHeader"), 1, m_szINIPath);
	// シーク量
	g_cfg.nSeekAmount = GetPrivateProfileInt(TEXT("Main"), TEXT("SeekAmount"), 5, m_szINIPath);
	// 音量変化量(隠し設定)
	g_cfg.nVolAmount = GetPrivateProfileInt(TEXT("Main"), TEXT("VolAmount"), 5, m_szINIPath);
	// 終了時に再生していた曲を起動時にも再生する
	g_cfg.nResume = GetPrivateProfileInt(TEXT("Main"), TEXT("Resume"), 0, m_szINIPath);
	// 終了時の再生位置も記録復元する
	g_cfg.nResPosFlag = GetPrivateProfileInt(TEXT("Main"), TEXT("ResPosFlag"), 0, m_szINIPath);
	// 閉じるボタンで最小化する
	g_cfg.nCloseMin = GetPrivateProfileInt(TEXT("Main"), TEXT("CloseMin"), 0, m_szINIPath);
	// サブフォルダを検索で圧縮ファイルも検索する
	g_cfg.nZipSearch = GetPrivateProfileInt(TEXT("Main"), TEXT("ZipSearch"), 0, m_szINIPath);
	// タブが一つの時はタブを隠す
	g_cfg.nTabHide = GetPrivateProfileInt(TEXT("Main"), TEXT("TabHide"), 0, m_szINIPath);
	// 32bit(float)で出力する
	g_cfg.nOut32bit = GetPrivateProfileInt(TEXT("Main"), TEXT("Out32bit"), 0, m_szINIPath);
	// 停止時にフェードアウトする
	g_cfg.nFadeOut = GetPrivateProfileInt(TEXT("Main"), TEXT("FadeOut"), 1, m_szINIPath);
	// スタートアップフォルダ読み込み
	GetPrivateProfileString(TEXT("Main"), TEXT("StartPath"), TEXT(""), g_cfg.szStartPath, MAX_FITTLE_PATH, m_szINIPath);
	// ファイラのパス
	GetPrivateProfileString(TEXT("Main"), TEXT("FilerPath"), TEXT(""), g_cfg.szFilerPath, MAX_FITTLE_PATH, m_szINIPath);

	// ホットキーの設定
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintf(szSec, TEXT("HotKey%d"), i);
		g_cfg.nHotKey[i] = GetPrivateProfileInt(TEXT("HotKey"), szSec, 0, m_szINIPath);
	}

	// クリック時の動作
	g_cfg.nTrayClick[0] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click0"), 6, m_szINIPath);
	g_cfg.nTrayClick[1] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click1"), 0, m_szINIPath);
	g_cfg.nTrayClick[2] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click2"), 8, m_szINIPath);
	g_cfg.nTrayClick[3] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click3"), 0, m_szINIPath);
	g_cfg.nTrayClick[4] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click4"), 5, m_szINIPath);
	g_cfg.nTrayClick[5] = GetPrivateProfileInt(TEXT("TaskTray"), TEXT("Click5"), 0, m_szINIPath);

	// フォント設定読み込み
	GetPrivateProfileString(TEXT("Font"), TEXT("FontName"), TEXT(""), g_cfg.szFontName, 32, m_szINIPath);	// フォント名""がデフォルトの印
	g_cfg.nFontHeight = GetPrivateProfileInt(TEXT("Font"), TEXT("Height"), 10, m_szINIPath);
	g_cfg.nFontStyle = GetPrivateProfileInt(TEXT("Font"), TEXT("Style"), 0, m_szINIPath);

	// 外部ツール
	GetPrivateProfileString(TEXT("Tool"), TEXT("Path0"), TEXT(""), g_cfg.szToolPath, MAX_FITTLE_PATH, m_szINIPath);
	return;
}

// 設定を保存
static void SaveConfig(){
	int i;
	TCHAR szSec[10];

	TCHAR m_szINIPath[MAX_FITTLE_PATH];	// INIファイルのパス

	// INIファイルの位置を取得
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, TEXT("fittle.ini"));

	WritePrivateProfileInt(TEXT("Color"), TEXT("BkColor"), g_cfg.nBkColor, m_szINIPath);
	WritePrivateProfileInt(TEXT("Color"), TEXT("TextColor"), g_cfg.nTextColor, m_szINIPath);
	WritePrivateProfileInt(TEXT("Color"), TEXT("PlayTxtCol"), g_cfg.nPlayTxtCol, m_szINIPath);
	WritePrivateProfileInt(TEXT("Color"), TEXT("PlayBkCol"), g_cfg.nPlayBkCol, m_szINIPath);
	WritePrivateProfileInt(TEXT("Color"), TEXT("PlayView"), g_cfg.nPlayView, m_szINIPath);

	WritePrivateProfileInt(TEXT("Main"), TEXT("Priority"), g_cfg.nHighTask, m_szINIPath); //システム優先度
	WritePrivateProfileInt(TEXT("Main"), TEXT("GridLine"), g_cfg.nGridLine, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("SingleExp"), g_cfg.nSingleExpand, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("ExistCheck"), g_cfg.nExistCheck, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("TimeInList"), g_cfg.nTimeInList, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("TreeIcon"), g_cfg.nTreeIcon, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("Tray"), g_cfg.nTrayOpt, m_szINIPath); //タスクトレイモード
	WritePrivateProfileInt(TEXT("Main"), TEXT("HideShow"), g_cfg.nHideShow, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("TabBottom"), g_cfg.nTabBottom, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("TabMulti"), g_cfg.nTabMulti, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("AllSub"), g_cfg.nAllSub, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("PathTip"), g_cfg.nPathTip, m_szINIPath); // ツールチップでフルパスを表示
	WritePrivateProfileInt(TEXT("Main"), TEXT("Info"), g_cfg.nInfoTip, m_szINIPath); //曲名お知らせ
	WritePrivateProfileInt(TEXT("Main"), TEXT("TagReverse"), g_cfg.nTagReverse, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("ShowHeader"), g_cfg.nShowHeader, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("SeekAmount"), g_cfg.nSeekAmount, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("VolAmount"), g_cfg.nVolAmount, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("Resume"), g_cfg.nResume, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("ResPosFlag"), g_cfg.nResPosFlag, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("CloseMin"), g_cfg.nCloseMin, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("ZipSearch"), g_cfg.nZipSearch, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("TabHide"), g_cfg.nTabHide, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("Out32bit"), g_cfg.nOut32bit, m_szINIPath);
	WritePrivateProfileInt(TEXT("Main"), TEXT("FadeOut"), g_cfg.nFadeOut, m_szINIPath);

	WritePrivateProfileString(TEXT("Main"), TEXT("StartPath"), g_cfg.szStartPath, m_szINIPath);
	WritePrivateProfileString(TEXT("Main"), TEXT("FilerPath"), g_cfg.szFilerPath, m_szINIPath);

	// ホットキーを保存
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintf(szSec, TEXT("HotKey%d"), i);
		WritePrivateProfileInt(TEXT("HotKey"), szSec, g_cfg.nHotKey[i], m_szINIPath); //ホットキー
	}

	// タスクトレイを保存
	for(i=0;i<6;i++){
		wsprintf(szSec, TEXT("Click%d"), i);
		WritePrivateProfileInt(TEXT("TaskTray"), szSec, g_cfg.nTrayClick[i], m_szINIPath); //ホットキー
	}

	WritePrivateProfileInt(TEXT("Font"), TEXT("Height"), g_cfg.nFontHeight, m_szINIPath);
	WritePrivateProfileInt(TEXT("Font"), TEXT("Style"), g_cfg.nFontStyle, m_szINIPath);

	WritePrivateProfileString(TEXT("Font"), TEXT("FontName"), g_cfg.szFontName, m_szINIPath);


	// 外部ツール保存
	WritePrivateProfileString(TEXT("Tool"), TEXT("Path0"), g_cfg.szToolPath, m_szINIPath);

	WritePrivateProfileString(NULL, NULL, NULL, m_szINIPath);
	return;
}

static BOOL OpenFilerPath(LPTSTR szPath, HWND hWnd, LPCTSTR pszMsg){
	OPENFILENAME ofn;
	TCHAR szFile[MAX_FITTLE_PATH] = {0};
	TCHAR szFileTitle[MAX_FITTLE_PATH] = {0};

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = TEXT("実行ファイル(*.exe)\0*.exe\0\0");
	ofn.lpstrFile = szFile;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.lpstrInitialDir = szPath;
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = MAX_FITTLE_PATH;
	ofn.nMaxFileTitle = MAX_FITTLE_PATH;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpstrDefExt = TEXT("exe");
	ofn.lpstrTitle = pszMsg;

	if(GetOpenFileName(&ofn)==0) return FALSE;
	lstrcpyn(szPath, szFile, MAX_FITTLE_PATH);
	return TRUE;
}

static BOOL GeneralCheckChanged(HWND hDlg){
	if (g_cfg.nExistCheck != (int)SendDlgItemMessage(hDlg, IDC_CHECK12, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nTimeInList != (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nTagReverse != (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nResume != (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nResPosFlag != (int)SendDlgItemMessage(hDlg, IDC_CHECK10, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nHighTask != (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nCloseMin != (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nZipSearch != (int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nSeekAmount != GetDlgItemInt(hDlg, IDC_COMBO1, NULL, FALSE)) return TRUE;
	if (g_cfg.nOut32bit != (int)SendDlgItemMessage(hDlg, IDC_CHECK13, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nFadeOut != (int)SendDlgItemMessage(hDlg, IDC_CHECK14, BM_GETCHECK, 0, 0)) return TRUE;
	return FALSE;
}

static BOOL CALLBACK GeneralSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg)
	{
		case WM_INITDIALOG:
			int i;
			TCHAR szBuff[3];

			LoadConfig();
		
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
				wsprintf(szBuff, TEXT("%d"), i);
				SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, (WPARAM)0, (LPARAM)szBuff);
			}
			SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)g_cfg.nSeekAmount-1, (LPARAM)0);

			if (SendF4B24Message(WM_F4B24_IPC_GET_CAPABLE, WM_F4B24_IPC_GET_CAPABLE_LP_FLOATOUTPUT) == WM_F4B24_IPC_GET_CAPABLE_RET_NOT_SUPPORTED){
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

				g_cfg.nCloseMin = (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0);
				g_cfg.nZipSearch = (int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);

				g_cfg.nSeekAmount = GetDlgItemInt(hDlg, IDC_COMBO1, NULL, FALSE);
				g_cfg.nOut32bit = (int)SendDlgItemMessage(hDlg, IDC_CHECK13, BM_GETCHECK, 0, 0);
				g_cfg.nFadeOut = (int)SendDlgItemMessage(hDlg, IDC_CHECK14, BM_GETCHECK, 0, 0);
				SaveConfig();
				ApplyFittle();
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
			if (GeneralCheckChanged(hDlg))
				PropSheet_Changed(GetParent(hDlg) , hDlg);
			else
				PropSheet_UnChanged(GetParent(hDlg) , hDlg);
			return TRUE;

		default:
			return FALSE;
	}
}

static BOOL PathCheckChanged(HWND hDlg){
	TCHAR szBuff[MAX_FITTLE_PATH];
	GetDlgItemText(hDlg, IDC_EDIT1, szBuff, MAX_FITTLE_PATH);
	if (lstrcmp(g_cfg.szStartPath, szBuff) != 0) return TRUE;
	GetDlgItemText(hDlg, IDC_EDIT3, szBuff, MAX_FITTLE_PATH);
	if (lstrcmp(g_cfg.szFilerPath, szBuff) != 0) return TRUE;
	GetDlgItemText(hDlg, IDC_EDIT4, szBuff, MAX_FITTLE_PATH);
	if (lstrcmp(g_cfg.szToolPath, szBuff) != 0) return TRUE;
	return FALSE;
}

static BOOL CALLBACK PathSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	TCHAR szBuff[MAX_FITTLE_PATH];
	switch(msg)
	{
		case WM_INITDIALOG:
			LoadConfig();

			SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), g_cfg.szStartPath);
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT3), g_cfg.szFilerPath);
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT4), g_cfg.szToolPath);
			SetDlgItemText(hDlg, IDC_STATIC4, TEXT(""));
			PostF4B24Message(WM_F4B24_IPC_GET_SUPPORT_LIST, (LPARAM)GetDlgItem(hDlg, IDC_STATIC4));

			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				GetDlgItemText(hDlg, IDC_EDIT1, g_cfg.szStartPath, MAX_FITTLE_PATH);
				GetDlgItemText(hDlg, IDC_EDIT3, g_cfg.szFilerPath, MAX_FITTLE_PATH);
				GetDlgItemText(hDlg, IDC_EDIT4, g_cfg.szToolPath, MAX_FITTLE_PATH);
				SaveConfig();
				ApplyFittle();
				return TRUE;
			}
			return FALSE;
		
		case WM_COMMAND:
			if(LOWORD(wp)==IDC_BUTTON1){
				BROWSEINFO bi;
				LPITEMIDLIST pidl;

				ZeroMemory(&bi, sizeof(BROWSEINFO));
				bi.hwndOwner = GetParent(hDlg);
				bi.lpszTitle = TEXT("起動時に開くフォルダを選択してください");
				pidl = SHBrowseForFolder(&bi);
				if(pidl){
					SHGetPathFromIDList(pidl, szBuff);
					CoTaskMemFree(pidl);
					SetDlgItemText(hDlg, IDC_EDIT1, szBuff);
				}
			}else if(LOWORD(wp)==IDC_BUTTON2){
				// TODO 一緒にする
				GetWindowText(GetDlgItem(hDlg, IDC_EDIT3), szBuff, MAX_FITTLE_PATH);
				if(OpenFilerPath(szBuff, hDlg, TEXT("ファイラを選択してください"))){
					SetWindowText(GetDlgItem(hDlg, IDC_EDIT3), szBuff);
				}
			}else if(LOWORD(wp)==IDC_BUTTON3){
				GetWindowText(GetDlgItem(hDlg, IDC_EDIT4), szBuff, MAX_FITTLE_PATH);
				if(OpenFilerPath(szBuff, hDlg, TEXT("外部ツールを選択してください"))){
					SetWindowText(GetDlgItem(hDlg, IDC_EDIT4), szBuff);
				}
			}

			if (PathCheckChanged(hDlg))
				PropSheet_Changed(GetParent(hDlg) , hDlg);
			else
				PropSheet_UnChanged(GetParent(hDlg) , hDlg);

			return TRUE;

		default:
			return FALSE;

	}
}

static struct ControlSheetWork {
	int nFontHeight;
	int nFontStyle;
	TCHAR szFontName[LF_FACESIZE];
	int nBkColor;
	int nTextColor;
	int nPlayTxtCol;
	int nPlayBkCol;
} m_csw;

static int Get_nPlayView(HWND hDlg){
	for(int i=0;i<4;i++){
		if(SendDlgItemMessage(hDlg, IDC_RADIO1+i, BM_GETCHECK, 0, 0)){
			return i;
		}
	}
	return 1;
}

static BOOL ControlCheckChanged(HWND hDlg){
	if (g_cfg.nTreeIcon !=(int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nHideShow !=(int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nAllSub !=(int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nPathTip !=(int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nGridLine !=(int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nShowHeader !=(int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nTabHide !=(int)SendDlgItemMessage(hDlg, IDC_CHECK7, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nTabBottom !=(int)SendDlgItemMessage(hDlg, IDC_CHECK8, BM_GETCHECK, 0, 0)) return TRUE;
	if (g_cfg.nSingleExpand !=(int)SendDlgItemMessage(hDlg, IDC_CHECK9, BM_GETCHECK, 0, 0)) return TRUE;	
	if (g_cfg.nTabMulti !=(int)SendDlgItemMessage(hDlg, IDC_CHECK11, BM_GETCHECK, 0, 0)) return TRUE;	
	if (g_cfg.nPlayView !=Get_nPlayView(hDlg)) return TRUE;
	if (g_cfg.nTextColor !=m_csw.nTextColor) return TRUE;
	if (g_cfg.nBkColor !=m_csw.nBkColor) return TRUE;
	if (g_cfg.nPlayTxtCol !=m_csw.nPlayTxtCol) return TRUE;
	if (g_cfg.nPlayBkCol !=m_csw.nPlayBkCol) return TRUE;
	if (lstrcmp(g_cfg.szFontName, m_csw.szFontName) != 0) return TRUE;
	if (g_cfg.nFontHeight !=m_csw.nFontHeight) return TRUE;
	if (g_cfg.nFontStyle !=m_csw.nFontStyle) return TRUE;
	return FALSE;
}

static BOOL CALLBACK ControlSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	int i;

	switch(msg)
	{
		case WM_INITDIALOG:

			LoadConfig();

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
			m_csw.nFontHeight = g_cfg.nFontHeight;
			m_csw.nFontStyle = g_cfg.nFontStyle;
			lstrcpyn(m_csw.szFontName, g_cfg.szFontName, LF_FACESIZE);
			m_csw.nTextColor = g_cfg.nTextColor;
			m_csw.nBkColor = g_cfg.nBkColor;
			m_csw.nPlayTxtCol = g_cfg.nPlayTxtCol;
			m_csw.nPlayBkCol = g_cfg.nPlayBkCol;
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				g_cfg.nTreeIcon = (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
				g_cfg.nHideShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0);
				g_cfg.nAllSub = (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0);
				g_cfg.nPathTip = (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0);
				g_cfg.nGridLine = (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0);
				g_cfg.nShowHeader = (int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);
				g_cfg.nTabHide = (int)SendDlgItemMessage(hDlg, IDC_CHECK7, BM_GETCHECK, 0, 0);
				g_cfg.nTabBottom = (int)SendDlgItemMessage(hDlg, IDC_CHECK8, BM_GETCHECK, 0, 0);
				g_cfg.nSingleExpand = (int)SendDlgItemMessage(hDlg, IDC_CHECK9, BM_GETCHECK, 0, 0);	
				g_cfg.nTabMulti = (int)SendDlgItemMessage(hDlg, IDC_CHECK11, BM_GETCHECK, 0, 0);	
				g_cfg.nPlayView = Get_nPlayView(hDlg);
				g_cfg.nTextColor = m_csw.nTextColor;
				g_cfg.nBkColor = m_csw.nBkColor;
				g_cfg.nPlayTxtCol = m_csw.nPlayTxtCol;
				g_cfg.nPlayBkCol = m_csw.nPlayBkCol;
				lstrcpyn(g_cfg.szFontName, m_csw.szFontName, LF_FACESIZE);
				g_cfg.nFontHeight = m_csw.nFontHeight;
				g_cfg.nFontStyle = m_csw.nFontStyle;
				SaveConfig();
				ApplyFittle();
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

					hDC = GetDC(hDlg);
					ZeroMemory(&lf, sizeof(LOGFONT));
					lstrcpyn(lf.lfFaceName, m_csw.szFontName, LF_FACESIZE);
					lf.lfHeight = -MulDiv(m_csw.nFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
					lf.lfItalic = (g_cfg.nFontStyle&ITALIC_FONTTYPE?TRUE:FALSE);
					lf.lfWeight = (g_cfg.nFontStyle&BOLD_FONTTYPE?FW_BOLD:0);
					ReleaseDC(hDlg, hDC);

					ZeroMemory(&cf, sizeof(CHOOSEFONT));
					lf.lfCharSet = SHIFTJIS_CHARSET;
					cf.lStructSize = sizeof(CHOOSEFONT);
					cf.hwndOwner = hDlg;
					cf.lpLogFont = &lf;
					cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
					cf.nFontType = SCREEN_FONTTYPE;
					if(ChooseFont(&cf)){
						lstrcpyn(m_csw.szFontName, lf.lfFaceName, LF_FACESIZE);
						m_csw.nFontStyle = cf.nFontType;
						m_csw.nFontHeight = cf.iPointSize / 10;
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
								m_csw.nBkColor = (int)cr;
								break;
							case IDC_BUTTON4:
								m_csw.nTextColor = (int)cr;
								break;
							case IDC_BUTTON6:
								m_csw.nPlayTxtCol = (int)cr;
								break;
							case IDC_BUTTON7:
								m_csw.nPlayBkCol = (int)cr;
								break;
						}
					}
					break;

				case IDC_BUTTON5:	// 標準に戻す
					m_csw.nBkColor = (int)GetSysColor(COLOR_WINDOW);
					m_csw.nTextColor = (int)GetSysColor(COLOR_WINDOWTEXT);
					m_csw.nPlayTxtCol = (int)RGB(0xFF, 0, 0);
					m_csw.nPlayBkCol = (int)RGB(230, 234, 238);
					m_csw.szFontName[0] = '\0';
					break;

				default:
					break;
			}

			if (ControlCheckChanged(hDlg))
				PropSheet_Changed(GetParent(hDlg) , hDlg);
			else
				PropSheet_UnChanged(GetParent(hDlg) , hDlg);
			return TRUE;

		default:
			return FALSE;

	}
	return FALSE;
}

static BOOL HotKeyCheckChanged(HWND hDlg){
	for(int i=0;i<HOTKEY_COUNT;i++){
		if (g_cfg.nHotKey[i] != (int)SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_GETHOTKEY, 0, 0)) return TRUE;
	}
	return FALSE;
}

static BOOL CALLBACK HotKeySheetProc(HWND hDlg, UINT msg, WPARAM /*wp*/, LPARAM lp){
	switch(msg)
	{
		case WM_INITDIALOG:
			int i;

			LoadConfig();

			for(i=0;i<HOTKEY_COUNT;i++){
				SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_SETRULES, (WPARAM)HKCOMB_NONE, MAKELPARAM(HOTKEYF_ALT, 0));
				SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_SETHOTKEY, (WPARAM)g_cfg.nHotKey[i], 0);
			}
			return TRUE;

		case WM_COMMAND:
			if (HotKeyCheckChanged(hDlg))
				PropSheet_Changed(GetParent(hDlg) , hDlg);
			else
				PropSheet_UnChanged(GetParent(hDlg) , hDlg);
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				for(i=0;i<HOTKEY_COUNT;i++){
					g_cfg.nHotKey[i] = (int)SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_GETHOTKEY, 0, 0);
				}
				SaveConfig();
				ApplyFittle();
				return TRUE;
			}
			return FALSE;

		default:
			return FALSE;
	}
}

// エディットボックスのプロシージャ
static LRESULT CALLBACK ClickableURLProc(HWND hSC, UINT msg, WPARAM wp, LPARAM lp){
	TCHAR szURL[MAX_FITTLE_PATH] = {0};

	switch(msg){
		case WM_PAINT:
			HDC hdc;
			LOGFONT lf;
			HFONT hFont;
			HFONT hOldFont;
			COLORREF cr;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hSC, &ps);
			ZeroMemory(&lf, sizeof(LOGFONT));
			lstrcpy(lf.lfFaceName , TEXT("ＭＳ Ｐゴシック"));
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfUnderline = TRUE;
			lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			hFont = CreateFontIndirect(&lf);
			hOldFont = (HFONT)SelectObject(hdc, (HGDIOBJ)hFont);
			cr = GetSysColor(COLOR_MENU);
			SetBkColor(hdc, cr);
			SetTextColor(hdc, RGB(0, 0, 255));
			SetBkMode(hdc, TRANSPARENT);
			GetWindowText(hSC, szURL, MAX_FITTLE_PATH);
			TextOut(hdc, 0, 0, szURL, lstrlen(szURL));
			SelectObject(hdc, (HGDIOBJ)hOldFont);
			DeleteObject(hFont);
			EndPaint(hSC, &ps);
			return 0;

		case WM_LBUTTONUP:
			GetWindowText(hSC, szURL, MAX_FITTLE_PATH);
			ShellExecute(HWND_DESKTOP, TEXT("open"), szURL, NULL, NULL, SW_SHOWNORMAL);
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
			SHGetFileInfo(TEXT("*.dll"), FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(SHFILEINFO),
				SHGFI_USEFILEATTRIBUTES | SHGFI_ICON);
			hIcon = shfi.hIcon;
			SendDlgItemMessage(hDlg, IDC_STATIC2, STM_SETICON, (LPARAM)hIcon, (LPARAM)0);
			SET_SUBCLASS(GetDlgItem(hDlg, IDC_STATIC3), ClickableURLProc);
			SET_SUBCLASS(GetDlgItem(hDlg, IDC_STATIC4), ClickableURLProc);
			SET_SUBCLASS(GetDlgItem(hDlg, IDC_STATIC7), ClickableURLProc);

			SendDlgItemMessage(hDlg, IDC_STATIC1, STM_SETICON, (LPARAM)LoadIcon(GetModuleHandle(NULL), TEXT("MYICON")), (LPARAM)0);
			PostF4B24Message(WM_F4B24_IPC_GET_VERSION_STRING, (LPARAM)GetDlgItem(hDlg, IDC_STATIC0));

			SendDlgItemMessage(hDlg, IDC_STATIC6, STM_SETICON, (LPARAM)LoadIcon(GetModuleHandle(NULL), TEXT("MYICON")), (LPARAM)0);
			PostF4B24Message(WM_F4B24_IPC_GET_VERSION_STRING2, (LPARAM)GetDlgItem(hDlg, IDC_STATIC5));
			return TRUE;

		case WM_CLOSE:
			if (hIcon){
				DestroyIcon(hIcon);
				hIcon = 0;
			}
			return FALSE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				return TRUE;
			}
			return FALSE;

		default:
			return FALSE;
	}
}

static int Get_nTrayOpt(HWND hDlg){
	if(SendDlgItemMessage(hDlg, IDC_RADIO1, BM_GETCHECK, 0, 0)){
		return 0;
	}else if(SendDlgItemMessage(hDlg, IDC_RADIO2, BM_GETCHECK, 0, 0)){
		return 1;
	}else{
		return 2;
	}
}

static int Get_nInfoTip(HWND hDlg){
	if(SendDlgItemMessage(hDlg, IDC_RADIO7, BM_GETCHECK, 0, 0)){
		return 0;
	}else if(SendDlgItemMessage(hDlg, IDC_RADIO8, BM_GETCHECK, 0, 0)){
		return 1;
	}else{
		return 2;
	}
}

static BOOL TaskTrayCheckChanged(HWND hDlg){
	if (g_cfg.nTrayOpt != Get_nTrayOpt(hDlg)) return TRUE;
	if (g_cfg.nInfoTip != Get_nInfoTip(hDlg)) return TRUE;
	for(int i=0;i<6;i++){
		if (g_cfg.nTrayClick[i] != SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) return TRUE;
	}
	return FALSE;
}

static BOOL CALLBACK TaskTraySheetProc(HWND hDlg, UINT msg, WPARAM /*wp*/, LPARAM lp){
	int i;
	switch(msg){
		case WM_INITDIALOG:

			LoadConfig();

			SendDlgItemMessage(hDlg, IDC_RADIO1 + g_cfg.nTrayOpt, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_RADIO7 + g_cfg.nInfoTip, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			for(i=0;i<6;i++){
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)0, (LPARAM)TEXT("なし"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)1, (LPARAM)TEXT("再生"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)2, (LPARAM)TEXT("一時停止"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)3, (LPARAM)TEXT("停止"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)4, (LPARAM)TEXT("前の曲"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)5, (LPARAM)TEXT("次の曲"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)6, (LPARAM)TEXT("ウィンドウ表示/非表示"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)7, (LPARAM)TEXT("終了"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)8, (LPARAM)TEXT("メニュー表示"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_INSERTSTRING, (WPARAM)9, (LPARAM)TEXT("再生/一時停止"));
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_SETCURSEL, (WPARAM)g_cfg.nTrayClick[i], (LPARAM)0);
			}
			return TRUE;

		case WM_COMMAND:
			if (TaskTrayCheckChanged(hDlg))
				PropSheet_Changed(GetParent(hDlg) , hDlg);
			else
				PropSheet_UnChanged(GetParent(hDlg) , hDlg);
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				g_cfg.nTrayOpt = Get_nTrayOpt(hDlg);
				g_cfg.nInfoTip = Get_nInfoTip(hDlg);
				for(int i=0;i<6;i++){
					g_cfg.nTrayClick[i] = SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				}
				SaveConfig();
				ApplyFittle();
				return TRUE;
			}
			return FALSE;

		default:
			return FALSE;
	}
}

static DWORD CALLBACK GetConfigPageCount(void){
	return 5 + 1;
}

static HPROPSHEETPAGE CALLBACK GetConfigPage(int nIndex, int nLevel, LPSTR pszConfigPath, int nConfigPathSize){
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof (PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = hDLL;
	if (nLevel == 0){
		if (nIndex == 0){
			psp.pszTemplate = TEXT("GENERAL_SHEET");
			psp.pfnDlgProc = (DLGPROC)GeneralSheetProc;
			lstrcpynA(pszConfigPath, "fittle/general", nConfigPathSize);
			return CreatePropertySheetPage(&psp);
		} else if (nIndex == 1){
			psp.pszTemplate = TEXT("PATH_SHEET");
			psp.pfnDlgProc = (DLGPROC)PathSheetProc;
			lstrcpynA(pszConfigPath, "fittle/path", nConfigPathSize);
			return CreatePropertySheetPage(&psp);
		} else if (nIndex == 2){
			psp.pszTemplate = TEXT("CONTROL_SHEET");
			psp.pfnDlgProc = (DLGPROC)ControlSheetProc;
			lstrcpynA(pszConfigPath, "fittle/control", nConfigPathSize);
			return CreatePropertySheetPage(&psp);
		} else if (nIndex == 3){
			psp.pszTemplate = TEXT("TASKTRAY_SHEET");
			psp.pfnDlgProc = (DLGPROC)TaskTraySheetProc;
			lstrcpynA(pszConfigPath, "fittle/tasktray", nConfigPathSize);
			return CreatePropertySheetPage(&psp);
		} else if (nIndex == 4){
			psp.pszTemplate = TEXT("HOTKEY_SHEET");
			psp.pfnDlgProc = (DLGPROC)HotKeySheetProc;
			lstrcpynA(pszConfigPath, "fittle/hotkey", nConfigPathSize);
			return CreatePropertySheetPage(&psp);
		}
	} else if (nLevel == 3){
		if (nIndex == 0){
			psp.pszTemplate = TEXT("ABOUT_SHEET");
			psp.pfnDlgProc = (DLGPROC)AboutSheetProc;
			lstrcpynA(pszConfigPath, "fittle/about", nConfigPathSize);
			return CreatePropertySheetPage(&psp);
		}
	}
	return 0;
}

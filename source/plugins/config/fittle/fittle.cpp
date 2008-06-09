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




// グローバルな設定を読み込む
static void LoadState(){
	// ミニパネル表示状態
	g_cfg.nMiniPanelEnd = (int)GetPrivateProfileInt("MiniPanel", "End", 0, m_szINIPath);
	// ツリーの幅を設定
	g_cfg.nTreeWidth = GetPrivateProfileInt("Main", "TreeWidth", 200, m_szINIPath);
	//g_cfg.nTWidthSub = GetPrivateProfileInt("Main", "TWidthSub", 200, m_szINIPath);
	g_cfg.nTreeState = GetPrivateProfileInt("Main", "TreeState", TREE_SHOW, m_szINIPath);
	// ステータスバー表示非表示
	g_cfg.nShowStatus =  GetPrivateProfileInt("Main", "ShowStatus", 1, m_szINIPath);
	// 終了時の再生位置も記録復元する
	g_cfg.nResPos = GetPrivateProfileInt("Main", "ResPos", 0, m_szINIPath);
	// しおりをルートとして扱う
	g_cfg.nBMRoot = GetPrivateProfileInt("BookMark", "BMRoot", 1, m_szINIPath);
	// しおりをフルパスで表示
	g_cfg.nBMFullPath = GetPrivateProfileInt("BookMark", "BMFullPath", 1, m_szINIPath);
	// 最後に再生していたファイル
	GetPrivateProfileString("Main", "LastFile", "", g_cfg.szLastFile, MAX_FITTLE_PATH, m_szINIPath);

	m_hFont = NULL;
	m_bFLoat = (BOOL)g_cfg.nOut32bit;
}



// グローバルな設定を読み込む
static void LoadConfig(){
	int i;
	char szSec[10];

	// システムの優先度の設定
	g_cfg.nHighTask = GetPrivateProfileInt("Main", "Priority", 0, m_szINIPath);
	// コントロールカラー
	g_cfg.nBkColor = (int)GetPrivateProfileInt("Color", "BkColor", (int)GetSysColor(COLOR_WINDOW), m_szINIPath);
	g_cfg.nTextColor = (int)GetPrivateProfileInt("Color", "TextColor", (int)GetSysColor(COLOR_WINDOWTEXT), m_szINIPath);
	g_cfg.nPlayTxtCol = (int)GetPrivateProfileInt("Color", "PlayTxtCol", (int)RGB(0xFF, 0, 0), m_szINIPath);
	g_cfg.nPlayBkCol = (int)GetPrivateProfileInt("Color", "PlayBkCol", (int)RGB(230, 234, 238), m_szINIPath);

	// グリッドライン
	g_cfg.nGridLine = GetPrivateProfileInt("Main", "GridLine", 1, m_szINIPath);
	g_cfg.nSingleExpand = GetPrivateProfileInt("Main", "SingleExp", 0, m_szINIPath);
	// 存在確認
	g_cfg.nExistCheck = GetPrivateProfileInt("Main", "ExistCheck", 0, m_szINIPath);
	// プレイリストで更新日時を取得する
	g_cfg.nTimeInList = GetPrivateProfileInt("Main", "TimeInList", 0, m_szINIPath);
	// システムイメージリストを結合
	g_cfg.nTreeIcon = GetPrivateProfileInt("Main", "TreeIcon", TRUE, m_szINIPath);
	// タスクトレイに収納のチェック
	g_cfg.nTrayOpt = GetPrivateProfileInt("Main", "Tray", 1, m_szINIPath);
	// ツリー表示設定
	g_cfg.nHideShow = GetPrivateProfileInt("Main", "HideShow", 0, m_szINIPath);
	// タブを下に表示する
	g_cfg.nTabBottom = GetPrivateProfileInt("Main", "TabBottom", 0, m_szINIPath);
	// 多段で表示する
	g_cfg.nTabMulti = GetPrivateProfileInt("Main", "TabMulti", 1, m_szINIPath);
	// すべてのフォルダが～
	g_cfg.nAllSub = GetPrivateProfileInt("Main", "AllSub", 0, m_szINIPath);
	// ツールチップでフルパスを表示
	g_cfg.nPathTip = GetPrivateProfileInt("Main", "PathTip", 1, m_szINIPath);
	// 曲名お知らせ機能
	g_cfg.nInfoTip = GetPrivateProfileInt("Main", "Info", 1, m_szINIPath);
	// 表示方法
	g_cfg.nPlayView = GetPrivateProfileInt("Color", "PlayView", 1, m_szINIPath);
	// タグを反転
	g_cfg.nTagReverse = GetPrivateProfileInt("Main", "TagReverse", 0, m_szINIPath);
	// ヘッダコントロールを表示する
	g_cfg.nShowHeader = GetPrivateProfileInt("Main", "ShowHeader", 1, m_szINIPath);
	// シーク量
	g_cfg.nSeekAmount = GetPrivateProfileInt("Main", "SeekAmount", 5, m_szINIPath);
	// 音量変化量(隠し設定)
	g_cfg.nVolAmount = GetPrivateProfileInt("Main", "VolAmount", 5, m_szINIPath);
	// 終了時に再生していた曲を起動時にも再生する
	g_cfg.nResume = GetPrivateProfileInt("Main", "Resume", 0, m_szINIPath);
	// 終了時の再生位置も記録復元する
	g_cfg.nResPosFlag = GetPrivateProfileInt("Main", "ResPosFlag", 0, m_szINIPath);
	// 閉じるボタンで最小化する
	g_cfg.nCloseMin = GetPrivateProfileInt("Main", "CloseMin", 0, m_szINIPath);
	// サブフォルダを検索で圧縮ファイルも検索する
	g_cfg.nZipSearch = GetPrivateProfileInt("Main", "ZipSearch", 0, m_szINIPath);
	// タブが一つの時はタブを隠す
	g_cfg.nTabHide = GetPrivateProfileInt("Main", "TabHide", 0, m_szINIPath);
	// 32bit(float)で出力する
	g_cfg.nOut32bit = GetPrivateProfileInt("Main", "Out32bit", 0, m_szINIPath);
	// 停止時にフェードアウトする
	g_cfg.nFadeOut = GetPrivateProfileInt("Main", "FadeOut", 1, m_szINIPath);
	// スタートアップフォルダ読み込み
	GetPrivateProfileString("Main", "StartPath", "", g_cfg.szStartPath, MAX_FITTLE_PATH, m_szINIPath);
	// ホットキーの設定
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintf(szSec, "HotKey%d", i);
		g_cfg.nHotKey[i] = GetPrivateProfileInt("HotKey", szSec, 0, m_szINIPath);
	}
	// クリック時の動作
	g_cfg.nTrayClick[0] = GetPrivateProfileInt("TaskTray", "Click0", 6, m_szINIPath);
	g_cfg.nTrayClick[1] = GetPrivateProfileInt("TaskTray", "Click1", 0, m_szINIPath);
	g_cfg.nTrayClick[2] = GetPrivateProfileInt("TaskTray", "Click2", 8, m_szINIPath);
	g_cfg.nTrayClick[3] = GetPrivateProfileInt("TaskTray", "Click3", 0, m_szINIPath);
	g_cfg.nTrayClick[4] = GetPrivateProfileInt("TaskTray", "Click4", 5, m_szINIPath);
	g_cfg.nTrayClick[5] = GetPrivateProfileInt("TaskTray", "Click5", 0, m_szINIPath);
	// フォント設定読み込み
	GetPrivateProfileString("Font", "FontName", "", g_cfg.szFontName, 32, m_szINIPath);	// フォント名""がデフォルトの印
	g_cfg.nFontHeight = GetPrivateProfileInt("Font", "Height", 10, m_szINIPath);
	g_cfg.nFontStyle = GetPrivateProfileInt("Font", "Style", 0, m_szINIPath);
	// ファイラのパス
	GetPrivateProfileString("Main", "FilerPath", "", g_cfg.szFilerPath, MAX_FITTLE_PATH, m_szINIPath);
	// 外部ツール
	GetPrivateProfileString("Tool", "Path0", "", g_cfg.szToolPath, MAX_FITTLE_PATH, m_szINIPath);
	return;
}

// 設定を保存
static void SaveConfig(HWND hWnd){
	int i;
	char szLastPath[MAX_FITTLE_PATH];
	char szSec[10];
	WINDOWPLACEMENT wpl;
	REBARBANDINFO rbbi;

	wpl.length = sizeof(WINDOWPLACEMENT);

	lstrcpyn(szLastPath, m_szTreePath, MAX_FITTLE_PATH);
	// コンパクトモードを考慮しながらウィンドウサイズを保存
	GetWindowPlacement(hWnd, &wpl);
	WritePrivateProfileInt("Main", "Maximized", (wpl.showCmd==SW_SHOWMAXIMIZED || wpl.flags & WPF_RESTORETOMAXIMIZED), m_szINIPath);	// 最大化
	WritePrivateProfileInt("Main", "Top", wpl.rcNormalPosition.top, m_szINIPath); //ウィンドウ位置Top
	WritePrivateProfileInt("Main", "Left", wpl.rcNormalPosition.left, m_szINIPath); //ウィンドウ位置Left
	WritePrivateProfileInt("Main", "Height", wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top, m_szINIPath); //ウィンドウ位置Height
	WritePrivateProfileInt("Main", "Width", wpl.rcNormalPosition.right - wpl.rcNormalPosition.left, m_szINIPath); //ウィンドウ位置Width
	ShowWindow(hWnd, SW_HIDE);	// 終了を高速化して見せるために非表示
	WritePrivateProfileInt("Main", "Mode", m_nPlayMode, m_szINIPath); //プレイモード
	WritePrivateProfileInt("Main", "Repeat", m_nRepeatFlag, m_szINIPath); //リピートモード
	WritePrivateProfileInt("Main", "Volumes", (int)SendMessage(m_hVolume, TBM_GETPOS, 0, 0), m_szINIPath); //ボリューム
	WritePrivateProfileInt("Main", "Tray", g_cfg.nTrayOpt, m_szINIPath); //タスクトレイモード
	WritePrivateProfileInt("Main", "Info", g_cfg.nInfoTip, m_szINIPath); //曲名お知らせ
	WritePrivateProfileInt("Main", "PathTip", g_cfg.nPathTip, m_szINIPath); // ツールチップでフルパスを表示
	WritePrivateProfileInt("Main", "Priority", g_cfg.nHighTask, m_szINIPath); //システム優先度
	WritePrivateProfileInt("Main", "TreeWidth", g_cfg.nTreeWidth, m_szINIPath); //ツリーの幅
	WritePrivateProfileInt("Main", "TreeState", (g_cfg.nTreeState==TREE_SHOW), m_szINIPath);
	WritePrivateProfileInt("Main", "ShowStatus", g_cfg.nShowStatus, m_szINIPath);
	WritePrivateProfileInt("Main", "TabIndex", (m_nPlayTab==-1?TabCtrl_GetCurSel(m_hTab):m_nPlayTab), m_szINIPath);	//TabのIndex
	WritePrivateProfileInt("Main", "MainMenu", (GetMenu(hWnd)?1:0), m_szINIPath);
	WritePrivateProfileInt("Main", "TreeIcon", g_cfg.nTreeIcon, m_szINIPath);
	WritePrivateProfileInt("Main", "HideShow", g_cfg.nHideShow, m_szINIPath);
	WritePrivateProfileInt("Main", "AllSub", g_cfg.nAllSub, m_szINIPath);
	WritePrivateProfileInt("Main", "ExistCheck", g_cfg.nExistCheck, m_szINIPath);
	WritePrivateProfileInt("Main", "TimeInList", g_cfg.nTimeInList, m_szINIPath);
	WritePrivateProfileInt("Main", "GridLine", g_cfg.nGridLine, m_szINIPath);
	WritePrivateProfileInt("Main", "SingleExp", g_cfg.nSingleExpand, m_szINIPath);
	WritePrivateProfileInt("Main", "TagReverse", g_cfg.nTagReverse, m_szINIPath);
	WritePrivateProfileInt("Main", "SeekAmount", g_cfg.nSeekAmount, m_szINIPath);
	WritePrivateProfileInt("Main", "VolAmount", g_cfg.nVolAmount, m_szINIPath);
	WritePrivateProfileInt("Main", "TabHide", g_cfg.nTabHide, m_szINIPath);
	WritePrivateProfileInt("Main", "ShowHeader", g_cfg.nShowHeader, m_szINIPath);
	WritePrivateProfileInt("Main", "Resume", g_cfg.nResume, m_szINIPath);
	WritePrivateProfileInt("Main", "ResPosFlag", g_cfg.nResPosFlag, m_szINIPath);
	WritePrivateProfileInt("Main", "ResPos", g_cfg.nResPos, m_szINIPath);
	WritePrivateProfileInt("Main", "CloseMin", g_cfg.nCloseMin, m_szINIPath);
	WritePrivateProfileInt("Main", "ZipSearch", g_cfg.nZipSearch, m_szINIPath);
	WritePrivateProfileInt("Main", "TabBottom", g_cfg.nTabBottom, m_szINIPath);
	WritePrivateProfileInt("Main", "TabMulti", g_cfg.nTabMulti, m_szINIPath);
	WritePrivateProfileInt("Main", "Out32bit", g_cfg.nOut32bit, m_szINIPath);
	WritePrivateProfileInt("Main", "FadeOut", g_cfg.nFadeOut, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "End", g_cfg.nMiniPanelEnd, m_szINIPath);
	WritePrivateProfileInt("Font", "Height", g_cfg.nFontHeight, m_szINIPath);
	WritePrivateProfileInt("Font", "Style", g_cfg.nFontStyle, m_szINIPath);
	WritePrivateProfileInt("Color", "PlayView", g_cfg.nPlayView, m_szINIPath);
	WritePrivateProfileInt("Color", "TextColor", g_cfg.nTextColor, m_szINIPath);
	WritePrivateProfileInt("Color", "BkColor", g_cfg.nBkColor, m_szINIPath);
	WritePrivateProfileInt("Color", "PlayTxtCol", g_cfg.nPlayTxtCol, m_szINIPath);
	WritePrivateProfileInt("Color", "PlayBkCol", g_cfg.nPlayBkCol, m_szINIPath);
	WritePrivateProfileInt("Column", "Width0", ListView_GetColumnWidth(GetCurListTab(m_hTab)->hList, 0), m_szINIPath);
	WritePrivateProfileInt("Column", "Width1", ListView_GetColumnWidth(GetCurListTab(m_hTab)->hList, 1), m_szINIPath);
	WritePrivateProfileInt("Column", "Width2", ListView_GetColumnWidth(GetCurListTab(m_hTab)->hList, 2), m_szINIPath);
	WritePrivateProfileInt("Column", "Width3", ListView_GetColumnWidth(GetCurListTab(m_hTab)->hList, 3), m_szINIPath);
	WritePrivateProfileInt("Column", "Sort", GetCurListTab(m_hTab)->nSortState, m_szINIPath);
	WritePrivateProfileString("Main", "LastPath", szLastPath, m_szINIPath); //ラストパス
	WritePrivateProfileString("Main", "StartPath", g_cfg.szStartPath, m_szINIPath);
	WritePrivateProfileString("Main", "FilerPath", g_cfg.szFilerPath, m_szINIPath);
	WritePrivateProfileString("Main", "LastFile", g_cfg.szLastFile, m_szINIPath);
	WritePrivateProfileInt("BookMark", "BMRoot", g_cfg.nBMRoot, m_szINIPath);
	WritePrivateProfileInt("BookMark", "BMFullPath", g_cfg.nBMFullPath, m_szINIPath);
	SaveBookMark(m_szINIPath);	// しおりの保存
	WritePrivateProfileString("Font", "FontName", g_cfg.szFontName, m_szINIPath);
	// ホットキーを保存
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintf(szSec, "HotKey%d", i);
		WritePrivateProfileInt("HotKey", szSec, g_cfg.nHotKey[i], m_szINIPath); //ホットキー
	}
	// タスクトレイを保存
	for(i=0;i<6;i++){
		wsprintf(szSec, "Click%d", i);
		WritePrivateProfileInt("TaskTray", szSec, g_cfg.nTrayClick[i], m_szINIPath); //ホットキー
	}
	//　レバーの状態を保存
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE | RBBIM_ID;
	for(i=0;i<BAND_COUNT;i++){
		SendMessage(GetDlgItem(hWnd, ID_REBAR), RB_GETBANDINFO, i, (WPARAM)&rbbi);
		wsprintf(szSec, "fStyle%d", i);
		WritePrivateProfileInt("Rebar2", szSec, rbbi.fStyle, m_szINIPath);
		wsprintf(szSec, "cx%d", i);
		WritePrivateProfileInt("Rebar2", szSec, rbbi.cx, m_szINIPath);
		wsprintf(szSec, "wID%d", i);
		WritePrivateProfileInt("Rebar2", szSec, rbbi.wID, m_szINIPath);
	}
	// 外部ツール保存
	WritePrivateProfileString("Tool", "Path0", g_cfg.szToolPath, m_szINIPath);
	return;
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



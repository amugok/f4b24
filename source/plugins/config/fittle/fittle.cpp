#include "../../../fittle/src/fittle.h"
#include "../../../fittle/src/f4b24.h"
#include "../cplugin.h"
#include "fittle.rh"

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

static HMODULE m_hDLL = 0;

#include "../../../fittle/src/wastr.h"
#include "../../../fittle/src/wastr.cpp"

static struct CONFIG m_cfg;				// 設定構造体

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

	WASetIniFile(NULL, "Fittle.ini");

	// コントロールカラー
	m_cfg.nBkColor = WAGetIniInt("Color", "BkColor", (int)GetSysColor(COLOR_WINDOW));
	m_cfg.nTextColor = WAGetIniInt("Color", "TextColor", (int)GetSysColor(COLOR_WINDOWTEXT));
	m_cfg.nPlayTxtCol = WAGetIniInt("Color", "PlayTxtCol", (int)RGB(0xFF, 0, 0));
	m_cfg.nPlayBkCol = WAGetIniInt("Color", "PlayBkCol", (int)RGB(230, 234, 238));
	// 表示方法
	m_cfg.nPlayView = WAGetIniInt("Color", "PlayView", 1);

	// システムの優先度の設定
	m_cfg.nHighTask = WAGetIniInt("Main", "Priority", 0);
	// グリッドライン
	m_cfg.nGridLine = WAGetIniInt("Main", "GridLine", 1);
	m_cfg.nSingleExpand = WAGetIniInt("Main", "SingleExp", 0);
	// 存在確認
	m_cfg.nExistCheck = WAGetIniInt("Main", "ExistCheck", 0);
	// プレイリストで更新日時を取得する
	m_cfg.nTimeInList = WAGetIniInt("Main", "TimeInList", 0);
	// システムイメージリストを結合
	m_cfg.nTreeIcon = WAGetIniInt("Main", "TreeIcon", TRUE);
	// タスクトレイに収納のチェック
	m_cfg.nTrayOpt = WAGetIniInt("Main", "Tray", 1);
	// ツリー表示設定
	m_cfg.nHideShow = WAGetIniInt("Main", "HideShow", 0);
	// タブを下に表示する
	m_cfg.nTabBottom = WAGetIniInt("Main", "TabBottom", 0);
	// 多段で表示する
	m_cfg.nTabMulti = WAGetIniInt("Main", "TabMulti", 1);
	// すべてのフォルダが～
	m_cfg.nAllSub = WAGetIniInt("Main", "AllSub", 0);
	// ツールチップでフルパスを表示
	m_cfg.nPathTip = WAGetIniInt("Main", "PathTip", 1);
	// 曲名お知らせ機能
	m_cfg.nInfoTip = WAGetIniInt("Main", "Info", 1);
	// タグを反転
	m_cfg.nTagReverse = WAGetIniInt("Main", "TagReverse", 0);
	// ヘッダコントロールを表示する
	m_cfg.nShowHeader = WAGetIniInt("Main", "ShowHeader", 1);
	// シーク量
	m_cfg.nSeekAmount = WAGetIniInt("Main", "SeekAmount", 5);
	// シーク時にポーズを解除する
	m_cfg.nRestartOnSeek = WAGetIniInt("Main", "RestartOnSeek", 1);
	// 音量変化量(隠し設定)
	m_cfg.nVolAmount = WAGetIniInt("Main", "VolAmount", 5);
	// 終了時に再生していた曲を起動時にも再生する
	m_cfg.nResume = WAGetIniInt("Main", "Resume", 0);
	// 終了時の再生位置も記録復元する
	m_cfg.nResPosFlag = WAGetIniInt("Main", "ResPosFlag", 0);
	// 終了時に再生していた曲を起動時に選択する
	m_cfg.nSelLastPlayed = WAGetIniInt("Main", "SelLastPlayed", 1);
	// 閉じるボタンで最小化する
	m_cfg.nCloseMin = WAGetIniInt("Main", "CloseMin", 0);
	// サブフォルダを検索で圧縮ファイルも検索する
	m_cfg.nZipSearch = WAGetIniInt("Main", "ZipSearch", 0);
	// タブが一つの時はタブを隠す
	m_cfg.nTabHide = WAGetIniInt("Main", "TabHide", 0);
	// スタートアップフォルダ読み込み
	WAGetIniStr("Main", "StartPath", &m_cfg.szStartPath);
	// ファイラのパス
	WAGetIniStr("Main", "FilerPath", &m_cfg.szFilerPath);

	// ホットキーの設定
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintfA(szSec, "HotKey%d", i);
		m_cfg.nHotKey[i] = WAGetIniInt("HotKey", szSec, 0);
	}

	// クリック時の動作
	for(i=0;i<6;i++){
		wsprintfA(szSec, "Click%d", i);
		m_cfg.nTrayClick[i] = WAGetIniInt("TaskTray", szSec, "\x6\x0\x8\x0\x5\x0"[i]); //ホットキー
	}

	// フォント設定読み込み
	WAGetIniStr("Font", "FontName", &m_cfg.szFontName);	// フォント名""がデフォルトの印
	m_cfg.nFontHeight = WAGetIniInt("Font", "Height", 10);
	m_cfg.nFontStyle = WAGetIniInt("Font", "Style", 0);

	// 外部ツール
	WAGetIniStr("Tool", "Path0", &m_cfg.szToolPath);
}

// 設定を保存
static void SaveConfig(){
	int i;
	CHAR szSec[10];

	WASetIniFile(NULL, "Fittle.ini");

	WASetIniInt("Color", "BkColor", m_cfg.nBkColor);
	WASetIniInt("Color", "TextColor", m_cfg.nTextColor);
	WASetIniInt("Color", "PlayTxtCol", m_cfg.nPlayTxtCol);
	WASetIniInt("Color", "PlayBkCol", m_cfg.nPlayBkCol);
	WASetIniInt("Color", "PlayView", m_cfg.nPlayView);

	WASetIniInt("Main", "Priority", m_cfg.nHighTask); //システム優先度
	WASetIniInt("Main", "GridLine", m_cfg.nGridLine);
	WASetIniInt("Main", "SingleExp", m_cfg.nSingleExpand);
	WASetIniInt("Main", "ExistCheck", m_cfg.nExistCheck);
	WASetIniInt("Main", "TimeInList", m_cfg.nTimeInList);
	WASetIniInt("Main", "TreeIcon", m_cfg.nTreeIcon);
	WASetIniInt("Main", "Tray", m_cfg.nTrayOpt); //タスクトレイモード
	WASetIniInt("Main", "HideShow", m_cfg.nHideShow);
	WASetIniInt("Main", "TabBottom", m_cfg.nTabBottom);
	WASetIniInt("Main", "TabMulti", m_cfg.nTabMulti);
	WASetIniInt("Main", "AllSub", m_cfg.nAllSub);
	WASetIniInt("Main", "PathTip", m_cfg.nPathTip); // ツールチップでフルパスを表示
	WASetIniInt("Main", "Info", m_cfg.nInfoTip); //曲名お知らせ
	WASetIniInt("Main", "TagReverse", m_cfg.nTagReverse);
	WASetIniInt("Main", "ShowHeader", m_cfg.nShowHeader);
	WASetIniInt("Main", "SeekAmount", m_cfg.nSeekAmount);
	WASetIniInt("Main", "RestartOnSeek", m_cfg.nRestartOnSeek);
	WASetIniInt("Main", "VolAmount", m_cfg.nVolAmount);
	WASetIniInt("Main", "Resume", m_cfg.nResume);
	WASetIniInt("Main", "ResPosFlag", m_cfg.nResPosFlag);
	WASetIniInt("Main", "SelLastPlayed", m_cfg.nSelLastPlayed);
	WASetIniInt("Main", "CloseMin", m_cfg.nCloseMin);
	WASetIniInt("Main", "ZipSearch", m_cfg.nZipSearch);
	WASetIniInt("Main", "TabHide", m_cfg.nTabHide);

	WASetIniStr("Main", "StartPath", &m_cfg.szStartPath);
	WASetIniStr("Main", "FilerPath", &m_cfg.szFilerPath);

	// ホットキーを保存
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintfA(szSec, "HotKey%d", i);
		WASetIniInt("HotKey", szSec, m_cfg.nHotKey[i]); //ホットキー
	}

	// タスクトレイを保存
	for(i=0;i<6;i++){
		wsprintfA(szSec, "Click%d", i);
		WASetIniInt("TaskTray", szSec, m_cfg.nTrayClick[i]); //ホットキー
	}

	WASetIniInt("Font", "Height", m_cfg.nFontHeight);
	WASetIniInt("Font", "Style", m_cfg.nFontStyle);

	WASetIniStr("Font", "FontName", &m_cfg.szFontName);

	// 外部ツール保存
	WASetIniStr("Tool", "Path0", &m_cfg.szToolPath);

	WAFlushIni();
}

static BOOL GeneralCheckChanged(HWND hDlg){
	if (m_cfg.nExistCheck != (int)SendDlgItemMessage(hDlg, IDC_CHECK12, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nTimeInList != (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nTagReverse != (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nResume != (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nResPosFlag != (int)SendDlgItemMessage(hDlg, IDC_CHECK10, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nSelLastPlayed != (int)SendDlgItemMessage(hDlg, IDC_CHECK14, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nHighTask != (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nCloseMin != (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nZipSearch != (int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nSeekAmount != GetDlgItemInt(hDlg, IDC_COMBO1, NULL, FALSE)) return TRUE;
	if (m_cfg.nRestartOnSeek != (int)SendDlgItemMessage(hDlg, IDC_CHECK13, BM_GETCHECK, 0, 0)) return TRUE;
	return FALSE;
}

static BOOL CALLBACK GeneralSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	int i;
	switch(msg)
	{
		case WM_INITDIALOG:

			LoadConfig();
		
			SendDlgItemMessage(hDlg, IDC_CHECK12, BM_SETCHECK, (WPARAM)m_cfg.nExistCheck, 0);
			/*if(!m_cfg.nExistCheck){
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK1), FALSE);
			}*/
			SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)m_cfg.nTimeInList, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, (WPARAM)m_cfg.nTagReverse, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, (WPARAM)m_cfg.nResume, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK10, BM_SETCHECK, (WPARAM)m_cfg.nResPosFlag, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK14, BM_SETCHECK, (WPARAM)m_cfg.nSelLastPlayed, 0);
			if(!m_cfg.nResume){
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK10), FALSE);
			}else{
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK14), FALSE);
			}
			SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, (WPARAM)m_cfg.nHighTask, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, (WPARAM)m_cfg.nCloseMin, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, (WPARAM)m_cfg.nZipSearch, 0);
			for(i=1;i<=60;i++){
				CHAR szBuff[3];
				wsprintfA(szBuff, "%d", i);
				SendDlgItemMessageA(hDlg, IDC_COMBO1, CB_ADDSTRING, (WPARAM)0, (LPARAM)szBuff);
			}
			SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)m_cfg.nSeekAmount-1, (LPARAM)0);
			SendDlgItemMessage(hDlg, IDC_CHECK13, BM_SETCHECK, (WPARAM)m_cfg.nRestartOnSeek, 0);

			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				m_cfg.nExistCheck = (int)SendDlgItemMessage(hDlg, IDC_CHECK12, BM_GETCHECK, 0, 0);
				m_cfg.nTimeInList = (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
				m_cfg.nTagReverse = (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0);
				m_cfg.nResume = (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0);
				m_cfg.nResPosFlag = (int)SendDlgItemMessage(hDlg, IDC_CHECK10, BM_GETCHECK, 0, 0);
				m_cfg.nSelLastPlayed = (int)SendDlgItemMessage(hDlg, IDC_CHECK14, BM_GETCHECK, 0, 0);
				m_cfg.nHighTask = (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0);

				m_cfg.nCloseMin = (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0);
				m_cfg.nZipSearch = (int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);

				m_cfg.nSeekAmount = GetDlgItemInt(hDlg, IDC_COMBO1, NULL, FALSE);
				m_cfg.nRestartOnSeek = (int)SendDlgItemMessage(hDlg, IDC_CHECK13, BM_GETCHECK, 0, 0);

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
						EnableWindow(GetDlgItem(hDlg, IDC_CHECK14), FALSE);
					}else{
						EnableWindow(GetDlgItem(hDlg, IDC_CHECK10), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_CHECK14), TRUE);
					}
				}
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
	WASTR buf;
	WAGetDlgItemText(hDlg, IDC_EDIT1, &buf);
	if (WAstrcmp(&m_cfg.szStartPath, &buf) != 0) return TRUE;
	WAGetDlgItemText(hDlg, IDC_EDIT3, &buf);
	if (WAstrcmp(&m_cfg.szFilerPath, &buf) != 0) return TRUE;
	WAGetDlgItemText(hDlg, IDC_EDIT4, &buf);
	if (WAstrcmp(&m_cfg.szToolPath, &buf) != 0) return TRUE;
	return FALSE;
}

static BOOL CALLBACK PathSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	WASTR buf;
	switch(msg)
	{
		case WM_INITDIALOG:
			LoadConfig();

			WASetDlgItemText(hDlg, IDC_EDIT1, &m_cfg.szStartPath);
			WASetDlgItemText(hDlg, IDC_EDIT3, &m_cfg.szFilerPath);
			WASetDlgItemText(hDlg, IDC_EDIT4, &m_cfg.szToolPath);
			SetDlgItemText(hDlg, IDC_EDIT5, TEXT(""));
			PostF4B24Message(WM_F4B24_IPC_GET_SUPPORT_LIST, (LPARAM)GetDlgItem(hDlg, IDC_EDIT5));

			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				WAGetDlgItemText(hDlg, IDC_EDIT1, &m_cfg.szStartPath);
				WAGetDlgItemText(hDlg, IDC_EDIT3, &m_cfg.szFilerPath);
				WAGetDlgItemText(hDlg, IDC_EDIT4, &m_cfg.szToolPath);
				SaveConfig();
				ApplyFittle();
				return TRUE;
			}
			return FALSE;
		
		case WM_COMMAND:
			if(LOWORD(wp)==IDC_BUTTON1){
				WAGetDlgItemText(hDlg, IDC_EDIT1, &buf);
				if(WABrowseForFolder(&buf, hDlg, "起動時に開くフォルダを選択してください")){
					WASetDlgItemText(hDlg, IDC_EDIT1, &buf);
				}
			}else if(LOWORD(wp)==IDC_BUTTON2){
				// TODO 一緒にする
				WAGetDlgItemText(hDlg, IDC_EDIT3, &buf);
				if(WAOpenFilerPath(&buf, hDlg, "ファイラを選択してください", "実行ファイル(*.exe);*.exe;", "exe")){
					WASetDlgItemText(hDlg, IDC_EDIT3, &buf);
				}
			}else if(LOWORD(wp)==IDC_BUTTON3){
				WAGetDlgItemText(hDlg, IDC_EDIT4, &buf);
				if(WAOpenFilerPath(&buf, hDlg, "外部ツールを選択してください", "実行ファイル(*.exe);*.exe;", "exe")){
					WASetDlgItemText(hDlg, IDC_EDIT4, &buf);
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
	WASTR szFontName;
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
	if (m_cfg.nTreeIcon !=(int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nHideShow !=(int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nAllSub !=(int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nPathTip !=(int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nGridLine !=(int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nShowHeader !=(int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nTabHide !=(int)SendDlgItemMessage(hDlg, IDC_CHECK7, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nTabBottom !=(int)SendDlgItemMessage(hDlg, IDC_CHECK8, BM_GETCHECK, 0, 0)) return TRUE;
	if (m_cfg.nSingleExpand !=(int)SendDlgItemMessage(hDlg, IDC_CHECK9, BM_GETCHECK, 0, 0)) return TRUE;	
	if (m_cfg.nTabMulti !=(int)SendDlgItemMessage(hDlg, IDC_CHECK11, BM_GETCHECK, 0, 0)) return TRUE;	
	if (m_cfg.nPlayView !=Get_nPlayView(hDlg)) return TRUE;
	if (m_cfg.nTextColor !=m_csw.nTextColor) return TRUE;
	if (m_cfg.nBkColor !=m_csw.nBkColor) return TRUE;
	if (m_cfg.nPlayTxtCol !=m_csw.nPlayTxtCol) return TRUE;
	if (m_cfg.nPlayBkCol !=m_csw.nPlayBkCol) return TRUE;
	if (WAstrcmp(&m_cfg.szFontName, &m_csw.szFontName) != 0) return TRUE;
	if (m_cfg.nFontHeight !=m_csw.nFontHeight) return TRUE;
	if (m_cfg.nFontStyle !=m_csw.nFontStyle) return TRUE;
	return FALSE;
}

static BOOL CALLBACK ControlSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp){
	int i;

	switch(msg)
	{
		case WM_INITDIALOG:

			LoadConfig();

			SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)m_cfg.nTreeIcon, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, (WPARAM)m_cfg.nHideShow, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, (WPARAM)m_cfg.nAllSub, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, (WPARAM)m_cfg.nPathTip, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, (WPARAM)m_cfg.nGridLine, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, (WPARAM)m_cfg.nShowHeader, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK7, BM_SETCHECK, (WPARAM)m_cfg.nTabHide, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK8, BM_SETCHECK, (WPARAM)m_cfg.nTabBottom, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK9, BM_SETCHECK, (WPARAM)m_cfg.nSingleExpand, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK11, BM_SETCHECK, (WPARAM)m_cfg.nTabMulti, 0);
			SendDlgItemMessage(hDlg, IDC_RADIO1 + m_cfg.nPlayView, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			m_csw.nFontHeight = m_cfg.nFontHeight;
			m_csw.nFontStyle = m_cfg.nFontStyle;
			WAstrcpy(&m_csw.szFontName, &m_cfg.szFontName);
			m_csw.nTextColor = m_cfg.nTextColor;
			m_csw.nBkColor = m_cfg.nBkColor;
			m_csw.nPlayTxtCol = m_cfg.nPlayTxtCol;
			m_csw.nPlayBkCol = m_cfg.nPlayBkCol;
			return TRUE;

		case WM_NOTIFY:
			if(((NMHDR *)lp)->code==PSN_APPLY){
				m_cfg.nTreeIcon = (int)SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
				m_cfg.nHideShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0);
				m_cfg.nAllSub = (int)SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0);
				m_cfg.nPathTip = (int)SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0);
				m_cfg.nGridLine = (int)SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0);
				m_cfg.nShowHeader = (int)SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);
				m_cfg.nTabHide = (int)SendDlgItemMessage(hDlg, IDC_CHECK7, BM_GETCHECK, 0, 0);
				m_cfg.nTabBottom = (int)SendDlgItemMessage(hDlg, IDC_CHECK8, BM_GETCHECK, 0, 0);
				m_cfg.nSingleExpand = (int)SendDlgItemMessage(hDlg, IDC_CHECK9, BM_GETCHECK, 0, 0);	
				m_cfg.nTabMulti = (int)SendDlgItemMessage(hDlg, IDC_CHECK11, BM_GETCHECK, 0, 0);	
				m_cfg.nPlayView = Get_nPlayView(hDlg);
				m_cfg.nTextColor = m_csw.nTextColor;
				m_cfg.nBkColor = m_csw.nBkColor;
				m_cfg.nPlayTxtCol = m_csw.nPlayTxtCol;
				m_cfg.nPlayBkCol = m_csw.nPlayBkCol;
				WAstrcpy(&m_cfg.szFontName, &m_csw.szFontName);
				m_cfg.nFontHeight = m_csw.nFontHeight;
				m_cfg.nFontStyle = m_csw.nFontStyle;
				SaveConfig();
				ApplyFittle();
				return TRUE;
			}
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDC_BUTTON2:
					WAChooseFont(&m_csw.szFontName, &m_csw.nFontHeight, &m_csw.nFontStyle, hDlg);
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
					WAstrcpyA(&m_csw.szFontName, "");
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
		if (m_cfg.nHotKey[i] != (int)SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_GETHOTKEY, 0, 0)) return TRUE;
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
				SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_SETHOTKEY, (WPARAM)m_cfg.nHotKey[i], 0);
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
					m_cfg.nHotKey[i] = (int)SendDlgItemMessage(hDlg, IDC_HOTKEY1 + i, HKM_GETHOTKEY, 0, 0);
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
	if (m_cfg.nTrayOpt != Get_nTrayOpt(hDlg)) return TRUE;
	if (m_cfg.nInfoTip != Get_nInfoTip(hDlg)) return TRUE;
	for(int i=0;i<6;i++){
		if (m_cfg.nTrayClick[i] != SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) return TRUE;
	}
	return FALSE;
}

static BOOL CALLBACK TaskTraySheetProc(HWND hDlg, UINT msg, WPARAM /*wp*/, LPARAM lp){
	int i;
	switch(msg){
		case WM_INITDIALOG:

			LoadConfig();

			SendDlgItemMessage(hDlg, IDC_RADIO1 + m_cfg.nTrayOpt, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_RADIO7 + m_cfg.nInfoTip, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
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
				SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_SETCURSEL, (WPARAM)m_cfg.nTrayClick[i], (LPARAM)0);
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
				m_cfg.nTrayOpt = Get_nTrayOpt(hDlg);
				m_cfg.nInfoTip = Get_nInfoTip(hDlg);
				for(int i=0;i<6;i++){
					m_cfg.nTrayClick[i] = SendDlgItemMessage(hDlg, IDC_COMBO1+i, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
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
	WASTR dlgtemp;
	LPCSTR lpszTemplate, lpszPath;
	DLGPROC lpfnDlgProc = 0;
	WAIsUnicode();
	if (nLevel == 0){
		if (nIndex == 0){
			lpszTemplate = "GENERAL_SHEET";
			lpfnDlgProc = (DLGPROC)GeneralSheetProc;
			lpszPath = "fittle/general";
		} else if (nIndex == 1){
			lpszTemplate = "PATH_SHEET";
			lpfnDlgProc = (DLGPROC)PathSheetProc;
			lpszPath = "fittle/path";
		} else if (nIndex == 2){
			lpszTemplate = "CONTROL_SHEET";
			lpfnDlgProc = (DLGPROC)ControlSheetProc;
			lpszPath = "fittle/control";
		} else if (nIndex == 3){
			lpszTemplate = "TASKTRAY_SHEET";
			lpfnDlgProc = (DLGPROC)TaskTraySheetProc;
			lpszPath = "fittle/tasktray";
		} else if (nIndex == 4){
			lpszTemplate = "HOTKEY_SHEET";
			lpfnDlgProc = (DLGPROC)HotKeySheetProc;
			lpszPath = "fittle/hotkey";
		}
	} else if (nLevel == 3){
		if (nIndex == 0){
			lpszTemplate = "ABOUT_SHEET";
			lpfnDlgProc = (DLGPROC)AboutSheetProc;
			lpszPath = "fittle/about";
		}
	}
	if (!lpfnDlgProc) return NULL;
	lstrcpynA(pszConfigPath, lpszPath, nConfigPathSize);
	return WACreatePropertySheetPage(m_hDLL, lpszTemplate, lpfnDlgProc);
}

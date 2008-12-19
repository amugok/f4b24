#include <windows.h>
#include <shlwapi.h>
#include "../../../fittle/resource/resource.h"
#include "../../../fittle/src/plugin.h"
#include "lyrics.rh"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

static HMODULE hDLL = 0;

BOOL OnInit();
void OnQuit();
void OnTrackChange();
void OnStatusChange();
void OnConfig();
#define IDM_LYRICS 50000

FITTLE_PLUGIN_INFO fpi = {
	PDK_VER,
	OnInit,
	OnQuit,
	OnTrackChange,
	OnStatusChange,
	OnConfig,
	NULL,
	NULL
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH){
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if (fdwReason == DLL_PROCESS_DETACH){
	}
	return TRUE;
}

WNDPROC g_hOldProc;
HWND g_hEdit;
HANDLE g_hThread = 0;
HWND g_hwndMain = 0;
char g_szIniPath[MAX_PATH];
HFONT   g_hFont = NULL;

//エディットのスクロールバーを表示するかどうか
BOOL g_bHSCROLL = TRUE;
BOOL g_bVSCROLL = TRUE;

// サブクラス化したウィンドウプロシージャ
LRESULT CALLBACK WndProcSub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_COMMAND:
			if(LOWORD(wp)==IDM_LYRICS)
			{
				if( IsWindowVisible( g_hwndMain ) )
					ShowWindow( g_hwndMain, SW_HIDE );
				else
					ShowWindow( g_hwndMain, SW_SHOW );
				return 0;
			}
			break;
	}
	return CallWindowProc(g_hOldProc, hWnd, msg, wp, lp);
}


/* エントリポント */
int WINAPI DllEntryPoint(HINSTANCE hInstance , DWORD fdwReason , PVOID pvReserved) {
    return TRUE;
}

//エディットを作る
BOOL CreateEdit( HWND hWnd )
{
	RECT rc;
	GetClientRect(hWnd, &rc);

	g_hEdit = CreateWindow(
		"EDIT",
		NULL,
		WS_CHILD | WS_VISIBLE |
		ES_WANTRETURN | ES_MULTILINE | ES_NOHIDESEL |ES_READONLY |
		( g_bVSCROLL ? WS_VSCROLL : 0 ) | ( g_bHSCROLL ? WS_HSCROLL : 0 ),
		0, 0,
		rc.right, rc.bottom,
		hWnd,
		(HMENU)0,
		fpi.hDllInst,
		NULL);

	SendMessage( g_hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE );
	
	return TRUE;
}

//設定を変更した際にエディットを作り直す
BOOL ReCreateEdit( HWND hWnd )
{
	int len = GetWindowTextLength( g_hEdit );
	LPSTR lpbuff = (LPSTR)HeapAlloc( GetProcessHeap(), 0, len + 1 );

	if( lpbuff == NULL )
		return FALSE;

	GetWindowText( g_hEdit, lpbuff, len + 1 );

	DestroyWindow( g_hEdit );

	CreateEdit( hWnd );

	SetWindowText( g_hEdit, lpbuff );

	return TRUE;
}

//ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd , UINT msg , WPARAM wp , LPARAM lp) {
	static HMENU hMenu;
	switch (msg) {
	case WM_CREATE:
		{
			hMenu = GetSubMenu( GetMenu( hWnd ), 0 );

			//フォントの設定
			LOGFONT lg = { 0 };
			if( GetPrivateProfileStruct( "Setting", "LogFont", &lg, sizeof LOGFONT, g_szIniPath) )
				g_hFont = CreateFontIndirect( &lg );

			//スクロールバーの設定を読み出し
			g_bVSCROLL = GetPrivateProfileInt( "Setting", "VSCROLL", TRUE, g_szIniPath );
			if( g_bVSCROLL )
				CheckMenuItem( hMenu, ID_VSCROLL, MF_CHECKED | MF_BYCOMMAND );

			g_bHSCROLL = GetPrivateProfileInt( "Setting", "HSCROLL", TRUE, g_szIniPath );
			if( !g_bHSCROLL )
				CheckMenuItem( hMenu, ID_HSCROLL, MF_CHECKED | MF_BYCOMMAND );

			CreateEdit( hWnd );
		}
		return 0;
	case WM_COMMAND:
		if( (HWND)lp == g_hEdit )
			if( HIWORD( wp ) == EN_SETFOCUS )
				HideCaret( g_hEdit ); //エディットのキャレットを消してみる

		switch( LOWORD( wp ) )
		{
		case ID_FONT:
			{
				//フォントの設定
				LOGFONT lg = { 0 };
				CHOOSEFONT cf = { sizeof (CHOOSEFONT) };
				cf.hwndOwner = hWnd;
				cf.lpLogFont = &lg;
				cf.Flags = CF_EFFECTS | CF_SCREENFONTS;

				if (ChooseFont(&cf))
				{
					if( g_hFont != NULL )
						DeleteObject( g_hFont );
					g_hFont = CreateFontIndirect( &lg );
					SendMessage( g_hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE );
					WritePrivateProfileStruct( "Setting", "LogFont", &lg, sizeof LOGFONT, g_szIniPath );
				}
			}
			break;
		case ID_VSCROLL:
			//たてのスクロールバー
			g_bVSCROLL = !g_bVSCROLL;
			CheckMenuItem( hMenu, ID_VSCROLL, ( g_bVSCROLL ? MF_CHECKED : MF_UNCHECKED ) | MF_BYCOMMAND );
			ReCreateEdit( hWnd );

			break;
		case ID_HSCROLL:
			//横のスクロールバー
			g_bHSCROLL = !g_bHSCROLL;
			CheckMenuItem( hMenu, ID_HSCROLL, ( g_bHSCROLL ? MF_UNCHECKED : MF_CHECKED ) | MF_BYCOMMAND );
			ReCreateEdit( hWnd );

			break;
		}
		return 0;
	case WM_CLOSE:
		ShowWindow( hWnd, SW_HIDE );
		return 0;
	case WM_SIZE:
		MoveWindow( g_hEdit, 0, 0, LOWORD( lp ), HIWORD( lp ), TRUE );
		return 0;
	case WM_DESTROY:

		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd , msg , wp , lp);
}

int WINAPI Main(/*HINSTANCE hInstance , HINSTANCE hPrevInstance ,
		 PSTR pCmdLine , int nCmdShow */) {
	HWND hwnd;
	MSG msg;
	WNDCLASS winc;


	winc.style		= CS_HREDRAW | CS_VREDRAW;
	winc.lpfnWndProc	= WndProc;
	winc.cbClsExtra	= winc.cbWndExtra	= 0;
	winc.hInstance		= fpi.hDllInst;
	winc.hIcon		= NULL;
	winc.hCursor		= LoadCursor(NULL , IDC_ARROW);
	winc.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	winc.lpszMenuName	= NULL;
	winc.lpszClassName	= TEXT("lyrics");

	if (!RegisterClass(&winc)) return 0;

	BOOL bVisible = GetPrivateProfileInt( "Setting", "Show", FALSE, g_szIniPath );

	hwnd = CreateWindow(
			TEXT("lyrics") , TEXT("lyrics") ,
			WS_OVERLAPPEDWINDOW ,
			100 , 100 , 200 , 200 , fpi.hParent , LoadMenu( fpi.hDllInst,MAKEINTRESOURCE( IDR_MENU1 ) ) ,
			fpi.hDllInst , NULL
	);

	//ウィンドウの位置を復元
	WINDOWPLACEMENT wpl;
	if( GetPrivateProfileStruct( "Setting", "WinPlacement", &wpl, sizeof( wpl ), g_szIniPath ) )
	{
		if( !bVisible )
			wpl.showCmd = SW_HIDE;

		SetWindowPlacement( hwnd, &wpl );
	}

	if ( hwnd == NULL ) return 0;

	g_hwndMain = hwnd;

	while (GetMessage(&msg , NULL , 0 , 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

static DWORD WINAPI ThreadFunc(LPVOID param)
{
	return Main();
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	//サブクラス化してメニューに追加
	g_hOldProc = (WNDPROC)SetWindowLong( fpi.hParent, GWL_WNDPROC, (LONG)WndProcSub );
	AppendMenu( GetMenu( fpi.hParent ), MF_STRING, IDM_LYRICS, "歌詞の表示" );

	int len = GetModuleFileName( fpi.hDllInst, g_szIniPath, MAX_PATH );
	PathRenameExtension( g_szIniPath, ".ini" );

	//ウィンドウを作成するスレッドを作成
	DWORD tid;
	if ( g_hThread ) return FALSE;
	g_hThread = CreateThread( NULL, 0, ThreadFunc, 0, 0, &tid );
	return g_hThread ? TRUE : FALSE;


	return TRUE;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	//ウィンドウ状態を保存
	WritePrivateProfileString( "Setting", "Show", IsWindowVisible( g_hwndMain ) ? "1" : "0", g_szIniPath );
	WINDOWPLACEMENT wpl = { sizeof(WINDOWPLACEMENT) };
	GetWindowPlacement( g_hwndMain, &wpl );
	WritePrivateProfileStruct( "Setting", "WinPlacement", &wpl, sizeof( wpl ), g_szIniPath );

	//設定を保存
	WritePrivateProfileString( "Setting", "VSCROLL", ( g_bVSCROLL ? "1" : "0" ), g_szIniPath );
	WritePrivateProfileString( "Setting", "HSCROLL", ( g_bHSCROLL ? "1" : "0" ), g_szIniPath );

	//スレッドを終了
	if ( g_hwndMain && IsWindow( g_hwndMain ) )
		PostMessage( g_hwndMain, WM_DESTROY, 0, 0 );
	if ( g_hThread )
	{
		WaitForSingleObject( g_hThread, INFINITE );
		CloseHandle( g_hThread );
		g_hThread = 0;
	}
	if( g_hFont != NULL )
		DeleteObject( g_hFont );
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	char szPath[MAX_PATH * 2 + 4];

	SendMessage( g_hEdit, WM_SETTEXT, 0, (LPARAM)"" );

	//歌詞ファイルを探す
	lstrcpy( szPath, (char *)SendMessage( fpi.hParent, WM_FITTLE, GET_PLAYING_PATH, 0 ) );

	//書庫内は探せない
	LPTSTR lpszArc = StrChr(szPath, '/');
	if (lpszArc) *lpszArc = 0;

	PathRenameExtension( szPath,".lrc" );
	if( !PathFileExists( szPath ) )
	{
		PathRenameExtension( szPath, ".txt" );
		if( !PathFileExists( szPath ) )
			return;
	}

	HANDLE hFile = CreateFile( szPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		char buf[2048 + 1];
		DWORD dwNumberOfBytesRead = 0;
		int fTop = 1;
		int fInTime = 0;
		char cMBCSLeadByte = 0;
		buf[sizeof(buf) - 1] = 0;
		while (ReadFile(hFile, buf + (cMBCSLeadByte != 0), sizeof(buf) - 1 - (cMBCSLeadByte != 0), &dwNumberOfBytesRead, 0) && dwNumberOfBytesRead > 0)
		{
			int iOutput = 0;
			int i;
			if (cMBCSLeadByte)
			{
				buf[0] = cMBCSLeadByte;
				dwNumberOfBytesRead++;
				cMBCSLeadByte = 0;
			}
			buf[dwNumberOfBytesRead] = 0;
			for (i = 0; i < dwNumberOfBytesRead; i++){
				if (IsDBCSLeadByte(buf[i]))
				{
					fTop = 0;
					if (++i == dwNumberOfBytesRead)
					{
						cMBCSLeadByte = buf[i - 1];
						buf[i - 1] = 0;
						break;
					}
				}
				else if (buf[i] == '\n')
				{
					if (!fInTime)
					{
						char cTop = buf[i + 1];
						buf[i + 1] = 0;
						SendMessage( g_hEdit, EM_REPLACESEL, FALSE, (LPARAM)(buf + iOutput) );
						buf[i + 1] = cTop;
					}
					iOutput = i + 1;
					fTop = 1;
					fInTime = 0;
				}
				else if (fTop && buf[i] == '[')
				{
					fTop = 0;
					fInTime = 1;
				}
				else if (fInTime && buf[i] == ']')
				{
					fInTime = 0;
					iOutput = i + 1;
				}
				else
				{
					fTop = 0;
				}
			}
			if (!fInTime)
			{
				SendMessage( g_hEdit, EM_REPLACESEL, FALSE, (LPARAM)(buf + iOutput) );
			}
		}
		CloseHandle(hFile);
	}
	SendMessage( g_hEdit, WM_VSCROLL, SB_TOP, 0 );

	return;
}

/* 再生状態が変わる時に呼ばれます */
void OnStatusChange(){
	switch( SendMessage(fpi.hParent, WM_FITTLE, GET_STATUS, 0) ){
	case 0: // 停止
		SendMessage( g_hEdit, WM_SETTEXT, 0, (LPARAM)"" );
		//case 1: // 再生
		//case 3: // 一時停止
	}
	return;
}

/* 設定画面を呼び出します（未実装）*/
void OnConfig(){
	return;
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
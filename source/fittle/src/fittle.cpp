/*
 * Fittle.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "fittle.h"
#include "func.h"
#include "tree.h"
#include "list.h"
#include "listtab.h"
#include "finddlg.h"
#include "archive.h"
#include "bass_tag.h"
#include "mt19937ar.h"
#include "plugins.h"
#include "dsp.h"
#include "cuesheet.h"
#include "plugin.h"

// �\�t�g���i�o�[�W�����A�b�v���ɖY�ꂸ�ɍX�V�j
#ifndef _DEBUG
#define FITTLE_VERSION "Fittle Ver.2.2.2 Preview 3 for bass 2.4"
#else
#define FITTLE_VERSION "Fittle Ver.2.2.2 <Debug>"
#endif

// ���C�u�����������N
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "../extra/bass24/bass.lib")

//--�}�N��--
// �S�Ă̑I����Ԃ�������A�w��C���f�b�N�X�̃A�C�e����I���A�\��
#define ListView_SingleSelect(hLV, nIndex) \
	ListView_SetItemState(hLV, -1, 0, (LVIS_SELECTED | LVIS_FOCUSED)); \
	ListView_SetItemState(hLV, nIndex, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED)); \
	ListView_EnsureVisible(hLV, nIndex, TRUE)

// �E�B���h�E���T�u�N���X���A�v���V�[�W���n���h�����E�B���h�E�Ɋ֘A�t����
#define SET_SUBCLASS(hWnd, Proc) \
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Proc))

// �Đ����̃��X�g�^�u�̃|�C���^���擾
#define GetPlayListTab(hTab) \
	GetListTab(hTab, (m_nPlayTab!=-1 ? m_nPlayTab : TabCtrl_GetCurSel(hTab)))

#define ODS(X) \
	OutputDebugString(X); OutputDebugString("\n");

enum {
	IDM_SETUP_NEXT = 32769,
	IDM_PLAY_NEXT
};
// --�񋓌^�̐錾--
enum {PM_LIST=0, PM_RANDOM, PM_SINGLE};	// �v���C���[�h
enum {GS_NOEND=0, GS_OK, GS_FAILED, GS_NEWFREQ};	// EventSync�̖߂�l�݂�����
enum {TREE_UNSHOWN=0, TREE_SHOW, TREE_HIDE};	// �c���[�̕\�����

//--�֐��̃v���g�^�C�v�錾--
// �v���V�[�W��
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK MyHookProc(int, WPARAM, LPARAM);
static LRESULT CALLBACK NewSliderProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewTabProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewTreeProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewSplitBarProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK GeneralSheetProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK PathSheetProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK ControlSheetProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK TaskTraySheetProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK AboutSheetProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK HotKeySheetProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK BookMarkDlgProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK AddURLDlgProc(HWND, UINT, WPARAM, LPARAM);
static DWORD CALLBACK MainStreamProc(HSTREAM, void *, DWORD, void *);
static void CALLBACK EventSync(HSYNC, DWORD, DWORD, void *);
// �R���g���[���֌W
static void LoadConfig();
static void SaveConfig(HWND);
static void DoTrayClickAction(HWND, int);
static void PopupTrayMenu(HWND);
static void PopupPlayModeMenu(HWND, NMTOOLBAR *);
static void ToggleWindowView(HWND);
static HWND CreateToolBar(HWND);
static void LoadBookMark(HMENU , char *);
static void SaveBookMark(char *);
static int AddBookMark(HMENU, HWND);
static void DrawBookMark(HMENU);
static int ShowToolTip(HWND, HWND, char *);
static int ControlPlayMode(HMENU, int);
static BOOL ToggleRepeatMode(HMENU);
static int SetTaskTray(HWND);
static int RegHotKey(HWND);
static int UnRegHotKey(HWND);
static void OnBeginDragList(HWND, POINT);
static void OnDragList(HWND, POINT);
static void OnEndDragList(HWND);
static void DrawTabFocus(HWND, int, BOOL);
static BOOL SetUIFont(void);
static void UpdateWindowSize(HWND);
static BOOL SetUIColor(void);
static void SetStatusbarIcon(LPSTR, BOOL);
static void ShowSettingDialog(HWND, int);
static LPSTR MallocAndConcatPath(LISTTAB *);
static void MyShellExecute(HWND, LPSTR, LPSTR, BOOL);
static int InitFileTypes();
static int SaveFileDialog(char *, char *);
static LONG GetToolbarTrueWidth(HWND);
// ���t����֌W
static BOOL SetChannelInfo(BOOL, struct FILEINFO *);
static BOOL _BASS_ChannelSetPosition(DWORD, int);
static void _BASS_ChannelSeekSecond(DWORD, float, int);
static BOOL PlayByUser(HWND, struct FILEINFO *);
static void OnChangeTrack();
static int FilePause();
static int StopOutputStream(HWND);
static struct FILEINFO *SelectNextFile(BOOL);
static void FreeChannelInfo(BOOL bFlag);

// �����o�ϐ��i��ԕҁj
static HINSTANCE m_hInst = NULL;	// �C���X�^���X
static NOTIFYICONDATA m_ni;			// �^�X�N�g���C�̃A�C�R���̃f�[�^
static int m_nPlayMode = PM_LIST;	// �v���C���[�h
static BOOL m_nRepeatFlag = FALSE;	// ���s�[�g�����ǂ���
static BOOL m_bTrayFlag = FALSE;	// �^�X�N�g���C�ɓ����Ă邩�ǂ���
static int m_nPlayTab = -1;			// �Đ����̃^�u�C���f�b�N�X
static struct FILEINFO *m_pNext = NULL;	// �Đ���
static int m_nGaplessState = GS_OK;	// �M���b�v���X�Đ��p�̃X�e�[�^�X
static char m_szINIPath[MAX_FITTLE_PATH];	// INI�t�@�C���̃p�X
static char m_szTime[100];			// �Đ�����
static char m_szTag[MAX_FITTLE_PATH];	// �^�O
static char m_szTreePath[MAX_FITTLE_PATH];	// �c���[�̃p�X
static BOOL m_bFLoat = FALSE;
static TAGINFO m_taginfo = {0};
static volatile BOOL m_bCueEnd = FALSE;
// �����o�ϐ��i�n���h���ҁj
static HWND m_hCombo = NULL;		// �R���{�{�b�N�X�n���h��
static HWND m_hTree = NULL;			// �c���[�r���[�n���h��
static HWND m_hTab = NULL;			// �^�u�R���g���[���n���h��
static HWND m_hToolBar = NULL;		// �c�[���o�[�n���h��
static HWND m_hStatus = NULL;		// �X�e�[�^�X�o�[�n���h��
static HWND m_hSeek = NULL;			// �V�[�N�o�[�n���h��
static HWND m_hVolume = NULL;		// �{�����[���o�[�n���h��
static HWND m_hTitleTip = NULL;		// �^�C�g���p�c�[���`�b�v�n���h��
static HWND m_hSliderTip = NULL;	// �X���C�_�p�c�[���`�b�v�n���h��
static HMENU m_hTrayMenu = NULL;	// �g���C���j���[�n���h��
static HIMAGELIST m_hDrgImg = NULL;	// �h���b�O�C���[�W
static HHOOK m_hHook = NULL;		// ���[�J���t�b�N�n���h��
static HFONT m_hBoldFont = NULL;	// �������t�H���g�n���h��
static HFONT m_hFont = NULL;		// �t�H���g�n���h��
static HSTREAM m_hChanOut = NULL;	// �X�g���[���n���h��
static HTREEITEM m_hHitTree = NULL;	// �c���[�̃q�b�g�A�C�e��
static HMENU m_hMainMenu = NULL;	// ���C�����j���[�n���h��
static CRITICAL_SECTION m_cs;


// �O���[�o���ϐ�
struct CONFIG g_cfg;				// �ݒ�\����
CHANNELINFO g_cInfo[2] = {0};		// �`�����l�����
BOOL g_bNow = FALSE;				// �A�N�e�B�u�ȃ`�����l��0 or 1

int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE /*hPrevInst*/, LPSTR /*lpsCmdLine*/, int nCmdShow){
	WNDCLASSEX wc;
	HWND hWnd;
	HACCEL hAccel;
	MSG msg;
	WINDOWPLACEMENT wpl = {0};
	char szClassName[] = "Fittle";	// �E�B���h�E�N���X

	DWORD dTime;
	dTime = GetTickCount();

#ifndef _DEBUG	// ���d�N���̖h�~
	HANDLE hMutex;
	HWND hFittle;
	COPYDATASTRUCT cds;
	int i;
	hMutex = CreateMutex(NULL, TRUE, "Mutex of Fittle");

	if(GetLastError()==ERROR_ALREADY_EXISTS){
		hFittle = FindWindow("Fittle", NULL);
		// �R�}���h���C��������Ζ{�̂ɑ��M
		if(__argc>1){
			// �R�}���h���C���I�v�V����
			if(__argv[1][0] == '/'){
				if(!lstrcmpi(__argv[1], "/play")){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
				}else if(!lstrcmpi(__argv[1], "/pause")){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_PAUSE, 0), 0);
				}else if(!lstrcmpi(__argv[1], "/stop")){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_STOP, 0), 0);
				}else if(!lstrcmpi(__argv[1], "/prev")){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_PREV, 0), 0);
				}else if(!lstrcmpi(__argv[1], "/next")){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
				}else if(!lstrcmpi(__argv[1], "/exit")){
					return SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_END, 0), 0);
				}else if(!lstrcmpi(__argv[1], "/add")){
					cds.dwData = 1;
					for(i=2;i<__argc;i++){
						cds.lpData = (void *)__argv[i];
						cds.cbData = lstrlen(__argv[i]) + 1;
						SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
					}
					if(__argc>2) return 0;
				}else if(!lstrcmpi(__argv[1], "/addplay")){
					cds.dwData = 1;
					for(i=2;i<__argc;i++){
						cds.lpData = (void *)__argv[i];
						cds.cbData = lstrlen(__argv[i]) + 1;
						SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
					}
					SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
					return 0;
				}
			}else{
				// �t�@�C���𓊂���
				cds.dwData = 0;
				cds.lpData = (void *)__argv[1];
				cds.cbData = lstrlen(__argv[1]) + 1;
				// �����񑗐M
				SendMessage(hFittle, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
				return 0;
			}
		}
		// �~�j�p�l������������I���
		if(SendMessage(hFittle, WM_USER, 0, 0))	return 0;

		if(!IsWindowVisible(hFittle) || IsIconic(hFittle)){
			SendMessage(hFittle, WM_COMMAND, MAKEWPARAM(IDM_TRAY_WINVIEW, 0), 0);
		}	
		SetForegroundWindow(hFittle);
		return 0;
	}
#endif

	// BASS�̃o�[�W�����m�F
	if(HIWORD(BASS_GetVersion())!=BASSVERSION){
		MessageBox(NULL, "bass.dll�̃o�[�W�������m�F���Ă��������B", "Fittle", MB_OK);
		return 0;
	}

	// INI�t�@�C���̈ʒu���擾
	GetModuleParentDir(m_szINIPath);
	lstrcat(m_szINIPath, "Fittle.ini");

	// �E�B���h�E�E�N���X�̓o�^
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_DBLCLKS; //CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc; //�v���V�[�W����
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInst = hCurInst; //�C���X�^���X
	wc.hIcon = (HICON)LoadImage(hCurInst, "MYICON", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hIconSm = (HICON)LoadImage(hCurInst, "MYICON", IMAGE_ICON, 16, 16, LR_SHARED);
	wc.hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,	0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = "MAINMENU";	//���j���[��
	wc.lpszClassName = (LPCSTR)szClassName;
	if(!RegisterClassEx(&wc)) return 0;

	LoadConfig();

	// �E�B���h�E�쐬
	hWnd = CreateWindow(szClassName,
			szClassName,			// �^�C�g���o�[�ɂ��̖��O���\������܂�
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,	// �E�B���h�E�̎��
			CW_USEDEFAULT,			// �w���W
			CW_USEDEFAULT,			// �x���W
			CW_USEDEFAULT,			// ��
			CW_USEDEFAULT,			// ����
			NULL,					// �e�E�B���h�E�̃n���h���A�e�����Ƃ���NULL
			NULL,					// ���j���[�n���h���A�N���X���j���[���g���Ƃ���NULL
			hCurInst,				// �C���X�^���X�n���h��
			NULL);
	if(!hWnd) return 0;

	// �ő剻���Č�
	if(nCmdShow==SW_SHOWNORMAL && GetPrivateProfileInt("Main", "Maximized", 0, m_szINIPath)){
		nCmdShow = SW_SHOWMAXIMIZED;			// �ő剻���
		//wpl.flags = WPF_RESTORETOMAXIMIZED;		// �ŏ������ꂽ�ő剻���
	}

	// �E�B���h�E�̈ʒu��ݒ�
	UpdateWindow(hWnd);
	wpl.length = sizeof(WINDOWPLACEMENT);
	wpl.showCmd = SW_HIDE;
	wpl.rcNormalPosition.left = (int)GetPrivateProfileInt("Main", "Left", (GetSystemMetrics(SM_CXSCREEN)-FITTLE_WIDTH)/2, m_szINIPath);
	wpl.rcNormalPosition.top = (int)GetPrivateProfileInt("Main", "Top", (GetSystemMetrics(SM_CYSCREEN)-FITTLE_HEIGHT)/2, m_szINIPath);
	wpl.rcNormalPosition.right = wpl.rcNormalPosition.left + (int)GetPrivateProfileInt("Main", "Width", FITTLE_WIDTH, m_szINIPath);
	wpl.rcNormalPosition.bottom = wpl.rcNormalPosition.top + (int)GetPrivateProfileInt("Main", "Height", FITTLE_HEIGHT, m_szINIPath);
	SetWindowPlacement(hWnd, &wpl);

	// �~�j�p�l���̕���
	if(g_cfg.nMiniPanelEnd){
		if(nCmdShow!=SW_SHOWNORMAL) ShowWindow(hWnd, nCmdShow);	// �ő剻���̃E�B���h�E��Ԃ�K��
		PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_MINIPANEL, 0), 0);
	}else{
		ShowWindow(hWnd, nCmdShow);	// �\��
	}

	// �A�N�Z�����[�^�[�L�[���擾
	hAccel = LoadAccelerators(hCurInst, "MYACCEL");

	char szBuff[100];
	GetWindowText(m_hStatus, szBuff, 100);
	if(lstrlen(szBuff)==0){
		wsprintf(szBuff, "Startup time: %d ms\n", GetTickCount() - dTime);
		SetWindowText(m_hStatus, szBuff);
	}

	//���b�Z�[�W���[�v
	while(GetMessage(&msg, NULL, 0, 0)){
		if(!TranslateAccelerator(hWnd, hAccel, &msg)){	// �ʏ���A�N�Z�����[�^�L�[�̗D��x���������Ă܂�
			HWND m_hMiniPanel = (HWND)SendMessage(hWnd, WM_FITTLE, GET_MINIPANEL, 0);
			if(!m_hMiniPanel || !IsDialogMessage(m_hMiniPanel, &msg)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	return (int)msg.wParam;
}

// �E�B���h�E�v���V�[�W��
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	static HWND s_hRebar = NULL;			// ���o�[�n���h��
	static HWND s_hSplitter = NULL;			// �X�v���b�^�n���h��
	static int s_nHitTab = -1;				// �^�u�̃q�b�g�A�C�e��
	static UINT s_uTaskbarRestart;			// �^�X�N�g���C�ĕ`�惁�b�Z�[�W�̂h�c
	static int s_nClickState = 0;			// �^�X�N�g���C�A�C�R���̃N���b�N���
	static BOOL s_bReviewAllow = TRUE;		// �c���[�̃Z���ω��Ō����������邩

	int i;
	int nFileIndex;
	char szLastPath[MAX_FITTLE_PATH] = {0};
	char szCmdParPath[MAX_FITTLE_PATH] = {0};
	char szSec[10] = "";
	MENUITEMINFO mii;

	switch(msg)
	{
		case WM_USER:
			return (LRESULT)(HWND)SendMessage(hWnd, WM_FITTLE, GET_MINIPANEL, 0);

		case WM_USER+1:
			OnChangeTrack();
			break;

		case WM_CREATE:
			// BASS������
			if(!BASS_Init(1, 44100, 0, hWnd, NULL)){
				MessageBox(hWnd, "BASS�̏������Ɏ��s���܂����B", "Fittle", MB_OK);
				BASS_Free();
				ExitProcess(1);
			}

			InitBassWaDsp(hWnd);
			
			// �D��x
			if(g_cfg.nHighTask){
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
			}

			// �����g���q�̌���
			GetModuleParentDir(szLastPath);
			InitArchive(szLastPath, hWnd);
			InitFileTypes();

			// �^�X�N�g���C�ĕ`��̃��b�Z�[�W��ۑ�
			s_uTaskbarRestart = RegisterWindowMessage("TaskbarCreated");

			// ���݂̃v���Z�XID�ŗ����W�F�l���[�^��������
			init_genrand((unsigned int)(GetTickCount()+GetCurrentProcessId()));

			// ���j���[�n���h����ۑ�
			m_hMainMenu = GetMenu(hWnd);

			// �R���g���[�����쐬
			INITCOMMONCONTROLSEX ic;
			ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
			ic.dwICC = ICC_USEREX_CLASSES | ICC_TREEVIEW_CLASSES
					 | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES
					 | ICC_COOL_CLASSES | ICC_TAB_CLASSES;
			InitCommonControlsEx(&ic);

			// �X�e�[�^�X�o�[�쐬
			m_hStatus = CreateStatusWindow(WS_CHILD | /*WS_VISIBLE |*/ SBARS_SIZEGRIP | CCS_BOTTOM | SBT_TOOLTIPS, "", hWnd, ID_STATUS);
			if(g_cfg.nShowStatus){
				g_cfg.nShowStatus = 0;
				PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_SHOWSTATUS, 0), 0);
			}

			// �R���{�{�b�N�X�쐬
			m_hCombo = CreateWindowEx(0,
				WC_COMBOBOXEX,
				NULL,
				WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,	// CBS_DROPDOWN
				0, 0, 0, 200,
				hWnd,
				(HMENU)ID_COMBO,
				m_hInst,
				NULL);

			// �c���[�쐬
			m_hTree = CreateWindowEx(WS_EX_CLIENTEDGE,
				WC_TREEVIEW,
				NULL,
				WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | (g_cfg.nSingleExpand?TVS_SINGLEEXPAND:0),
				0, 0, 0, 0,
				hWnd,
				(HMENU)ID_TREE,
				m_hInst,
				NULL);
			TreeView_SetBkColor(m_hTree, g_cfg.nBkColor);
			TreeView_SetTextColor(m_hTree, g_cfg.nTextColor);
			DragAcceptFiles(m_hTree, TRUE);

			// �^�u�R���g���[���쐬
			m_hTab = CreateWindow( 
				WC_TABCONTROL,
				NULL, 
				WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | (g_cfg.nTabMulti?TCS_MULTILINE:0) | (g_cfg.nTabBottom?TCS_BOTTOM:0),
				0, 0, 20, 20, 
				hWnd,
				(HMENU)ID_TAB,
				m_hInst,
				NULL);
			SendMessage(m_hTab, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
			DragAcceptFiles(m_hTab, TRUE);

			MakeNewTab(m_hTab, "�t�H���_", 0);

			// �v���C���X�g���X�g�ǂݍ���
			LoadPlaylists(m_hTab);

			//�X�v���b�g�E�B���h�E���쐬
			s_hSplitter = CreateWindow("static",
				NULL,
				SS_NOTIFY | WS_CHILD | WS_VISIBLE,
				0, 0, 0, 0,
				hWnd,
				NULL,
				m_hInst,
				NULL
			);

			//���o�[�R���g���[���̍쐬
			REBARINFO rbi;
			s_hRebar = CreateWindowEx(WS_EX_TOOLWINDOW,
				REBARCLASSNAME,
				NULL,
				WS_CHILD | WS_VISIBLE | WS_BORDER | /*WS_CLIPSIBLINGS | */WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_AUTOSIZE | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE,
				0, 0, 0, 0,
				hWnd, (HMENU)ID_REBAR, m_hInst, NULL);

			ZeroMemory(&rbi, sizeof(REBARINFO));
			rbi.cbSize = sizeof(REBARINFO);
			rbi.fMask = 0;
			rbi.himl = 0;
			PostMessage(s_hRebar, RB_SETBARINFO, 0, (LPARAM)&rbi);

			//�c�[���`�b�v�̍쐬
			m_hTitleTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				NULL, NULL, 
				m_hInst, NULL);
			m_hSliderTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				hWnd, NULL, 
				m_hInst, NULL);

			// �c�[���o�[�̍쐬
			m_hToolBar = CreateToolBar(s_hRebar);
			//RECT rc;	// �c�[���o�[�̉��̒������擾
			//SendMessage(m_hToolBar, TB_GETITEMRECT, SendMessage(m_hToolBar, TB_BUTTONCOUNT, 0, 0) - 1, (LPARAM)&rc);

			//�{�����[���o�[�̍쐬
			m_hVolume = CreateWindowEx(0,
				TRACKBAR_CLASS,
				NULL,
				WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_BOTH | TBS_TOOLTIPS | TBS_FIXEDLENGTH | WS_CLIPSIBLINGS,
				//rc.right, 1, 120, 20,
				0, 0, 0, 0,//120, 20,
				s_hRebar,
				(HMENU)ID_VOLUME,
				m_hInst,
				NULL); 
			PostMessage(m_hVolume, TBM_SETTHUMBLENGTH, (WPARAM)16, (LPARAM)0);
			PostMessage(m_hVolume, TBM_SETRANGEMAX, (WPARAM)FALSE, (LPARAM)SLIDER_DIVIDED); //���x�̐ݒ�

			//�V�[�N�o�[�̍쐬
			m_hSeek = CreateWindowEx(0,
				TRACKBAR_CLASS,
				NULL,
				WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_BOTTOM | WS_DISABLED | TBS_FIXEDLENGTH | WS_CLIPSIBLINGS,
				0, 0, 0, 0,//120, 20,
				s_hRebar,
				(HMENU)TD_SEEKBAR,
				m_hInst,
				NULL); 
			PostMessage(m_hSeek, TBM_SETTHUMBLENGTH, (WPARAM)16, (LPARAM)0);
			PostMessage(m_hSeek, TBM_SETRANGEMAX, (WPARAM)FALSE, (LPARAM)SLIDER_DIVIDED); //���x�̐ݒ�

			// ���o�[�R���g���[���̕���
			REBARBANDINFO rbbi;
			ZeroMemory(&rbbi, sizeof(REBARBANDINFO));
			rbbi.cbSize = sizeof(REBARBANDINFO);
			rbbi.fMask  = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_ID;
			rbbi.cyMinChild = 22;
			rbbi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
			for(i=0;i<BAND_COUNT;i++){
				wsprintf(szSec, "cx%d", i);
				rbbi.cx = GetPrivateProfileInt("Rebar2", szSec, 0, m_szINIPath);
				wsprintf(szSec, "wID%d", i);
				rbbi.wID = GetPrivateProfileInt("Rebar2", szSec, i, m_szINIPath);
				wsprintf(szSec, "fStyle%d", i);
				rbbi.fStyle = GetPrivateProfileInt("Rebar2", szSec, (RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS), m_szINIPath); //RBBS_BREAK
				
				switch(rbbi.wID){
					case 0:	// �c�[���o�[
						rbbi.hwndChild = m_hToolBar;
						rbbi.cxMinChild = GetToolbarTrueWidth(m_hToolBar);
						break;

					case 1:	// �{�����[���o�[
						rbbi.hwndChild = m_hVolume;
						rbbi.cxMinChild = 100;
						break;

					case 2:	// �V�[�N�o�[
						rbbi.hwndChild = m_hSeek;		
						rbbi.cxMinChild = 100;
						break;
				}
				SendMessage(s_hRebar, RB_INSERTBAND, (WPARAM)i, (LPARAM)&rbbi);

				// ��\�����̏���
				if(!(rbbi.fStyle & RBBS_HIDDEN)){
					CheckMenuItem(GetMenu(hWnd), IDM_SHOWMAIN+i, MF_BYCOMMAND | MF_CHECKED);
				}else{
					if(rbbi.wID==0)	ShowWindow(m_hToolBar, SW_HIDE);	// �c�[���o�[���\������Ă��܂��o�O�΍�
				}
			}

			// ���j���[�`�F�b�N
			if(rbbi.fStyle & RBBS_NOGRIPPER){
				CheckMenuItem(GetMenu(hWnd), IDM_FIXBAR, MF_BYCOMMAND | MF_CHECKED);
			}

			if(g_cfg.nTreeState==TREE_SHOW){
				CheckMenuItem(GetMenu(hWnd), IDM_TOGGLETREE, MF_BYCOMMAND | MF_CHECKED);
			}

			//�R���g���[���̃T�u�N���X��
			SET_SUBCLASS(m_hSeek, NewSliderProc);
			SET_SUBCLASS(m_hVolume, NewSliderProc);
			SET_SUBCLASS(m_hTab, NewTabProc);
			SET_SUBCLASS(m_hCombo, NewComboProc);
			SET_SUBCLASS(GetWindow(m_hCombo, GW_CHILD), NewEditProc);
			SET_SUBCLASS(m_hTree, NewTreeProc);
			SET_SUBCLASS(s_hSplitter, NewSplitBarProc);

			// TREE_INIT
			InitTreeIconIndex(m_hCombo, m_hTree, (BOOL)g_cfg.nTreeIcon);
			
			// �V�X�e�����j���[�̕ύX
			m_hTrayMenu = GetSubMenu(LoadMenu(m_hInst, "TRAYMENU"), 0);
			FillMemory(&mii, sizeof(mii), '\0');
			mii.fMask = MIIM_TYPE;
			mii.cbSize = sizeof(mii);
			mii.fType = MFT_SEPARATOR;
			InsertMenuItem(GetSystemMenu(hWnd, FALSE), 7, FALSE, &mii);

			mii.fMask = MIIM_TYPE | MIIM_ID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = "���j���[�̕\��(&V)\tCtrl+M";
			mii.wID = IDM_TOGGLEMENU;
			InsertMenuItem(GetSystemMenu(hWnd, FALSE), 8, FALSE, &mii);

			mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
			mii.fType = MFT_STRING;
			mii.dwTypeData = "&Fittle";
			mii.hSubMenu = m_hTrayMenu;
			InsertMenuItem(GetSystemMenu(hWnd, FALSE), 9, FALSE, &mii);

			DrawMenuBar(hWnd);

			// �v���C���[�h��ݒ肷��
			ControlPlayMode(GetMenu(hWnd), (int)GetPrivateProfileInt("Main", "Mode", PM_LIST, m_szINIPath));
			m_nRepeatFlag = GetPrivateProfileInt("Main", "Repeat", TRUE, m_szINIPath);
			if(m_nRepeatFlag){
				m_nRepeatFlag = FALSE;
				ToggleRepeatMode(GetMenu(hWnd));
			}

			// �E�B���h�E�^�C�g���̐ݒ�
			SetWindowText(hWnd, FITTLE_VERSION);
			lstrcpy(m_ni.szTip, FITTLE_VERSION);

			// �^�X�N�g���C
			if(g_cfg.nTrayOpt==2) SetTaskTray(hWnd);

			// �{�����[���̐ݒ�
			PostMessage(m_hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)GetPrivateProfileInt("Main", "Volumes", SLIDER_DIVIDED, m_szINIPath));

			// �O���[�o���z�b�g�L�[
			RegHotKey(hWnd);

			// ���[�J���t�b�N
			m_hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)MyHookProc, m_hInst, GetWindowThreadProcessId(hWnd, NULL));

			// �N���e�B�J���Z�N�V�����̍쐬
			InitializeCriticalSection(&m_cs);

			// ���[�U�C���^�[�t�F�C�X
			SetUIFont();

			// ������̓ǂݍ���
			LoadBookMark(GetSubMenu(GetMenu(hWnd), 3), m_szINIPath);

			// ���j���[�̔�\��
			if(!GetPrivateProfileInt("Main", "MainMenu", 1, m_szINIPath))
				PostMessage(hWnd, WM_COMMAND, IDM_TOGGLEMENU, 0);

			// �R���{�{�b�N�X�̏�����
			SetDrivesToCombo(m_hCombo);

			// �R�}���h���C���̏���
			BOOL bCmd;
			bCmd = FALSE;
			for(i=1;i<__argc;i++){
				if(bCmd) break;
				switch(GetParentDir(__argv[i], szCmdParPath)){
					case FOLDERS: // �t�H���_
					case LISTS:   // ���X�g
					case ARCHIVES:// �A�[�J�C�u
					case CUESHEETS:// CUE�V�[�g
						if(g_cfg.nTreeState==TREE_SHOW){
							MakeTreeFromPath(m_hTree, m_hCombo, szCmdParPath);
						}else{
							lstrcpyn(m_szTreePath, szCmdParPath, MAX_FITTLE_PATH);
							SendMessage(hWnd, WM_COMMAND, IDM_FILEREVIEW, 0);
						}
						bCmd = TRUE;
						break;

					case FILES: // �t�@�C��
						if(g_cfg.nTreeState==TREE_SHOW){
							MakeTreeFromPath(m_hTree, m_hCombo, szCmdParPath);
						}else{
							lstrcpyn(m_szTreePath, szCmdParPath, MAX_FITTLE_PATH);
							SendMessage(hWnd, WM_COMMAND, IDM_FILEREVIEW, 0);
						}
						GetLongPathName(__argv[i], szCmdParPath, MAX_FITTLE_PATH); // 98�ȍ~
						nFileIndex = GetIndexFromPath(GetListTab(m_hTab, 0)->pRoot, szCmdParPath);
						ListView_SingleSelect(GetListTab(m_hTab, 0)->hList, nFileIndex);
						bCmd = TRUE;
						break;
				}
			}

			// �v���O�C�����Ăяo��
			//GetModuleParentDir(szLastPath);
			InitPlugins(szLastPath, hWnd);

			if(bCmd){
				PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
			}else{
				// �R�}���h���C���Ȃ�
				// �����Oor���݂��Ȃ��Ȃ�Ō�̃p�X�ɂ���
				if(lstrlen(g_cfg.szStartPath)==0 || !FILE_EXIST(g_cfg.szStartPath)){
					GetPrivateProfileString("Main", "LastPath", "", szLastPath, MAX_FITTLE_PATH, m_szINIPath);
				}else{
					lstrcpyn(szLastPath, g_cfg.szStartPath, MAX_FITTLE_PATH);
				}
				if(FILE_EXIST(szLastPath)){
					if(g_cfg.nTreeState==TREE_SHOW){
						MakeTreeFromPath(m_hTree, m_hCombo, szLastPath);
					}else{
						lstrcpyn(m_szTreePath, szLastPath, MAX_FITTLE_PATH);
						SendMessage(hWnd, WM_COMMAND, IDM_FILEREVIEW, 0);
					}
				}else{
					TreeView_Select(m_hTree, MakeDriveNode(m_hCombo, m_hTree), TVGN_CARET);
				}
				// �^�u�̕���
				TabCtrl_SetCurFocus(m_hTab, m_nPlayTab = GetPrivateProfileInt("Main", "TabIndex", 0, m_szINIPath));

				// �Ō�ɍĐ����Ă����t�@�C�����Đ�
				if(g_cfg.nResume){
					if(m_nPlayMode==PM_RANDOM && !g_cfg.nResPosFlag){
						PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
					}else{
						nFileIndex = GetIndexFromPath(GetCurListTab(m_hTab)->pRoot, g_cfg.szLastFile);
						ListView_SingleSelect(GetCurListTab(m_hTab)->hList, nFileIndex);
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
						// �|�W�V����������
						if(g_cfg.nResPosFlag){
							BASS_ChannelStop(g_cInfo[g_bNow].hChan);
							BASS_ChannelSetPosition(g_cInfo[g_bNow].hChan, g_cfg.nResPos + g_cInfo[g_bNow].qStart, BASS_POS_BYTE);
							BASS_ChannelPlay(g_cInfo[g_bNow].hChan,  FALSE);
						}
					}
				}else if(GetKeyState(VK_SHIFT) < 0){
					// Shift�L�[��������Ă�����Đ�
					PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
				}
			}
			break;

		case WM_SETFOCUS:
			if(GetCurListTab(m_hTab)) SetFocus(GetCurListTab(m_hTab)->hList);
			break;

		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			if(BASS_ChannelIsActive(m_hChanOut)){
				g_cfg.nResPos = (int)(BASS_ChannelGetPosition(g_cInfo[g_bNow].hChan, BASS_POS_BYTE) - g_cInfo[g_bNow].qStart);
			}else{
				g_cfg.nResPos = 0;
			}
			StopOutputStream(hWnd);

			// �N���e�B�J���Z�N�V�������폜
			DeleteCriticalSection(&m_cs);

			SaveConfig(hWnd);
			SavePlaylists(m_hTab);

			if(m_bTrayFlag){
				Shell_NotifyIcon(NIM_DELETE, &m_ni);	// �^�X�N�g���C�ɓ����Ă������̏���
				m_bTrayFlag = FALSE;
			}

			QuitPlugins();	// �v���O�C���Ăяo��

			UnRegHotKey(hWnd);
			UnhookWindowsHookEx(m_hHook);
			FreeBassWaDsp();
			BASS_Free(); // Bass�̊J��
			
			PostQuitMessage(0);
			break;
			
		case WM_SIZE:
			RECT rcTab;
			RECT rcStBar;
			RECT rcCombo;
			int nRebarHeight;
			int sb_size[4];
			HDWP hdwp;

			//�ŏ����C�x���g�̏���
			if(wp==SIZE_MINIMIZED){
				switch(g_cfg.nTrayOpt)
				{
					case 1: // �ŏ�����
						ShowWindow(hWnd, SW_HIDE);
						SetTaskTray(hWnd);
						break;
					case 2:
						ShowWindow(hWnd, SW_HIDE);
						break;
				}
				break;
			}

			if(!IsWindowVisible(hWnd)){
				return 0;	// �����Ă��Ă΁`�΍�
			}

			//�X�e�[�^�X�o�[�̕����ύX
			sb_size[3] = LOWORD(lp);
			sb_size[2] = sb_size[3] - 18;
			sb_size[1] = sb_size[2] - 120;
			sb_size[0] = sb_size[1] - 100;
			SendMessage(m_hStatus, SB_SETPARTS, (WPARAM)3, (LPARAM)(LPINT)sb_size);
			
			SendMessage(m_hStatus, WM_SIZE, wp, lp);

			//SendMessage(m_hToolBar, WM_SIZE, wp, lp);
			SendMessage(s_hRebar, WM_SIZE, wp, lp);

			nRebarHeight = (int)SendMessage(s_hRebar, RB_GETBARHEIGHT, 0, 0); 
			if(g_cfg.nShowStatus){
				GetClientRect(m_hStatus, &rcStBar);
			}else{
				rcStBar.bottom = 0;
			}
			GetClientRect(m_hCombo, &rcCombo);
			rcCombo.bottom--;	// �����d�Ȃ�悤�ɒ���

			hdwp = BeginDeferWindowPos(4);

			hdwp = DeferWindowPos(hdwp, m_hCombo, HWND_TOP,
				SPLITTER_WIDTH,
				nRebarHeight + SPLITTER_WIDTH,
				(g_cfg.nTreeState==TREE_SHOW?g_cfg.nTreeWidth:0),
				HIWORD(lp)  - nRebarHeight - SPLITTER_WIDTH - rcCombo.bottom - rcStBar.bottom,
				SWP_NOZORDER);

			hdwp = DeferWindowPos(hdwp, m_hTree, m_hCombo, 
				SPLITTER_WIDTH,
				nRebarHeight + SPLITTER_WIDTH + rcCombo.bottom,
				(g_cfg.nTreeState==TREE_SHOW?g_cfg.nTreeWidth:0),
				HIWORD(lp)  - nRebarHeight - SPLITTER_WIDTH - rcCombo.bottom - rcStBar.bottom,
				SWP_NOZORDER);

			hdwp = DeferWindowPos(hdwp, s_hSplitter, HWND_TOP,
				(g_cfg.nTreeState==TREE_SHOW?SPLITTER_WIDTH + g_cfg.nTreeWidth:0),
				nRebarHeight + SPLITTER_WIDTH,
				SPLITTER_WIDTH,
				HIWORD(lp)  - nRebarHeight - SPLITTER_WIDTH - rcStBar.bottom,
				SWP_NOZORDER);

			hdwp = DeferWindowPos(hdwp, m_hTab, HWND_TOP,
				(g_cfg.nTreeState==TREE_SHOW?SPLITTER_WIDTH*2 + g_cfg.nTreeWidth:SPLITTER_WIDTH),
				nRebarHeight + SPLITTER_WIDTH,
				(g_cfg.nTreeState==TREE_SHOW?LOWORD(lp) - SPLITTER_WIDTH*3 - g_cfg.nTreeWidth:LOWORD(lp) - SPLITTER_WIDTH*2),
				HIWORD(lp)  - nRebarHeight - SPLITTER_WIDTH - rcStBar.bottom,
				SWP_NOZORDER);

			EndDeferWindowPos(hdwp);

			// ���X�g�r���[���T�C�Y
			hdwp = BeginDeferWindowPos(1);

			GetClientRect(m_hTab, &rcTab);
			if(!(g_cfg.nTabHide && TabCtrl_GetItemCount(m_hTab)==1)){
				if(rcTab.left<rcTab.right){
					TabCtrl_AdjustRect(m_hTab, FALSE, &rcTab);
				}
			}
			hdwp = DeferWindowPos(hdwp, GetCurListTab(m_hTab)->hList, HWND_TOP,
				rcTab.left,
				rcTab.top,
				rcTab.right - rcTab.left,
				rcTab.bottom - rcTab.top,
				SWP_SHOWWINDOW | SWP_NOZORDER);

			EndDeferWindowPos(hdwp);
			break;

		case WM_COPYDATA: // �������M
			COPYDATASTRUCT *pcds;
			char szParPath[MAX_FITTLE_PATH];

			pcds= (COPYDATASTRUCT *)lp;
			if(pcds->dwData==0){
				switch(GetParentDir((char *)(pcds->lpData), szParPath)){
					case FOLDERS:
					case LISTS:
					case ARCHIVES:
					case CUESHEETS:
						MakeTreeFromPath(m_hTree, m_hCombo, szParPath);
						m_nPlayTab = 0;
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
						return TRUE;

					case FILES:
						MakeTreeFromPath(m_hTree, m_hCombo, (char *)szParPath);
						GetLongPathName((char *)(pcds->lpData), szParPath, MAX_FITTLE_PATH); // 98�ȍ~
						nFileIndex = GetIndexFromPath(GetListTab(m_hTab, 0)->pRoot, szParPath);
						ListView_SingleSelect(GetCurListTab(m_hTab)->hList, nFileIndex);
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
						return TRUE;
				}
			}else{
				struct FILEINFO *pSub = NULL;
				char szTest[MAX_FITTLE_PATH];
				//lstrcpyn(szTest, (char*)pcds->lpData, MAX_FITTLE_PATH);	// ��x�o�b�t�@�ɃR�s�[���Ȃ��Ɨ�����
				GetLongPathName((LPSTR)pcds->lpData, szTest, MAX_FITTLE_PATH);
				SearchFiles(&pSub, szTest, TRUE);
				ListView_SetItemState(GetCurListTab(m_hTab)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
				InsertList(GetCurListTab(m_hTab), -1, pSub);
				ListView_EnsureVisible(GetCurListTab(m_hTab)->hList, ListView_GetItemCount(GetCurListTab(m_hTab)->hList)-1, TRUE);
				return TRUE;
			}

			break;

		case WM_COMMAND:
			char szNowDir[MAX_FITTLE_PATH];
			switch(LOWORD(wp))
			{
				case IDM_REVIEW: //�ŐV�̏��ɍX�V
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					MakeTreeFromPath(m_hTree, m_hCombo, szNowDir); 
					break;

				case IDM_FILEREVIEW:
					if(!s_bReviewAllow) break;
					GetListTab(m_hTab, 0)->nPlaying = -1;
					GetListTab(m_hTab, 0)->nStkPtr = -1;
					DeleteAllList(&(GetListTab(m_hTab, 0)->pRoot));
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					if(SearchFiles(&(GetListTab(m_hTab, 0)->pRoot), szNowDir, GetKeyState(VK_CONTROL) < 0)!=LISTS){
						MergeSort(&(GetListTab(m_hTab, 0)->pRoot), GetListTab(m_hTab, 0)->nSortState);
					}
					TraverseList(GetListTab(m_hTab, 0));
					TabCtrl_SetCurFocus(m_hTab, 0);
					break;

				case IDM_SAVE: //�v���C���X�g�ɕۑ�
					s_nHitTab = TabCtrl_GetCurSel(m_hTab);
				case IDM_TAB_SAVE:
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					if(IsPlayList(szNowDir) || IsArchive(szNowDir) || IsCueSheet(szNowDir)){
						*PathFindFileName(szNowDir) = '\0';
					}else{
						MyPathAddBackslash(szNowDir);
					}
					int nRet;
					nRet = SaveFileDialog(szNowDir, GetListTab(m_hTab, s_nHitTab)->szTitle);
					if(nRet){
						WriteM3UFile(GetListTab(m_hTab, s_nHitTab)->pRoot, szNowDir, nRet);
					}
					s_nHitTab = NULL;
					break;

				case IDM_END: //�I��
					SendMessage(hWnd, WM_CLOSE, 0, 0);
					break;

				case IDM_PLAY: //�Đ�
					int nLBIndex;

					nLBIndex = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, -1, LVNI_SELECTED);
					if(BASS_ChannelIsActive(m_hChanOut)==BASS_ACTIVE_PAUSED &&  nLBIndex==GetCurListTab(m_hTab)->nPlaying)
					{
						FilePause();	// �|�[�Y���Ȃ�ĊJ
					}else{ // ����ȊO�Ȃ�I���t�@�C�����Đ�
						if(nLBIndex!=-1){
							m_nPlayTab = TabCtrl_GetCurSel(m_hTab);
							PlayByUser(hWnd, GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, nLBIndex));
						}
					}
					break;

				case IDM_PLAYPAUSE:
 					switch(BASS_ChannelIsActive(m_hChanOut)){
						case BASS_ACTIVE_PLAYING:
						case BASS_ACTIVE_PAUSED:
							SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PAUSE, 0), 0);
							break;
						case BASS_ACTIVE_STOPPED:
							SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
							break;
					}
					break;

				case IDM_PAUSE: //�ꎞ��~
					FilePause();
					break;

				case IDM_STOP: //��~
					StopOutputStream(hWnd);
					break;

				case IDM_PREV: //�O�̋�
					struct FILEINFO *pPrev;
					int nPrev;

					switch(m_nPlayMode){
						case PM_LIST:
						case PM_SINGLE:
							if(GetPlayListTab(m_hTab)->nPlaying<=0){
								nPrev = ListView_GetItemCount(GetPlayListTab(m_hTab)->hList)-1;
							}else{
								nPrev = GetPlayListTab(m_hTab)->nPlaying-1;
							}
							pPrev = GetPtrFromIndex(GetPlayListTab(m_hTab)->pRoot, nPrev);
							break;

						case PM_RANDOM:
							if(GetStackPtr(GetPlayListTab(m_hTab))<=0){
								pPrev = NULL;
							}else{
								PopPrevious(GetPlayListTab(m_hTab));	// ���ݍĐ����̋Ȃ��폜
								pPrev = PopPrevious(GetPlayListTab(m_hTab));
							}
							break;

						default:
							pPrev = NULL;
							break;
					}
					if(pPrev){
						PlayByUser(hWnd, pPrev);
					}
					break;

				case IDM_NEXT: //���̋�
					struct FILEINFO *pNext;

					pNext = SelectNextFile(FALSE);
					if(pNext){
						PlayByUser(hWnd, pNext);
					}
					break;

				case IDM_PLAY_NEXT:
					EnterCriticalSection(&m_cs);

					// �t�@�C���؂�ւ�
					g_bNow = !g_bNow;
					// �J��
					FreeChannelInfo(!g_bNow);

					LeaveCriticalSection(&m_cs);

					PostMessage(GetParent(m_hStatus), WM_USER+1, 0, 0); 
					break;

				case IDM_SETUP_NEXT:
					m_pNext = SelectNextFile(TRUE);
					m_nGaplessState = GS_OK;

					if(m_pNext){
						// �I�[�v��
						if(!SetChannelInfo(!g_bNow, m_pNext)){
							m_nGaplessState = GS_FAILED;
						}else{
							BASS_CHANNELINFO info1,info2;
							// ��̃t�@�C���̎��g���A�`�����l�����A�r�b�g�����`�F�b�N
							BASS_ChannelGetInfo(g_cInfo[g_bNow].hChan, &info1);
							BASS_ChannelGetInfo(g_cInfo[!g_bNow].hChan, &info2);
							if(info1.freq!=info2.freq || info1.chans!=info2.chans || info1.flags!=info2.flags){
								m_nGaplessState = GS_NEWFREQ;
							}else{
								m_nGaplessState = GS_OK;
							}
						}
					}
					break;

				case IDM_SEEKFRONT:
					_BASS_ChannelSeekSecond(g_cInfo[g_bNow].hChan, (float)g_cfg.nSeekAmount, 1);
					break;

				case IDM_SEEKBACK:
					_BASS_ChannelSeekSecond(g_cInfo[g_bNow].hChan, (float)g_cfg.nSeekAmount, -1);
					break;

				case IDM_VOLUP: //���ʂ��グ��
					SendMessage(m_hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SendMessage(m_hVolume, TBM_GETPOS, 0 ,0) + g_cfg.nVolAmount * 10));
					break;

				case IDM_VOLDOWN: //���ʂ�������
					SendMessage(m_hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SendMessage(m_hVolume, TBM_GETPOS, 0 ,0) - g_cfg.nVolAmount * 10));
					break;

				case IDM_PM_LIST: //���X�g
					ControlPlayMode(m_hMainMenu, PM_LIST);
					break;

				case IDM_PM_RANDOM: //�����_��
					ControlPlayMode(m_hMainMenu, PM_RANDOM);
					break;

				case IDM_PM_SINGLE: //���s�[�g
					ControlPlayMode(m_hMainMenu, PM_SINGLE);
					break;

				case IDM_PM_TOGGLE:
					ControlPlayMode(m_hMainMenu, m_nPlayMode+1);
					break;

				case IDM_PM_REPEAT:
					ToggleRepeatMode(m_hMainMenu);
					break;

				case IDM_SUB: //�T�u�t�H���_������
					GetListTab(m_hTab, 0)->nPlaying = -1;
					GetListTab(m_hTab, 0)->nStkPtr = -1;
					DeleteAllList(&(GetListTab(m_hTab, 0)->pRoot));
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					SetCursor(LoadCursor(NULL, IDC_WAIT));  // �����v�J�[�\���ɂ���
					if(IsPlayList(szNowDir)){
						ReadM3UFile(&(GetListTab(m_hTab, 0)->pRoot), szNowDir);
					}else if(IsArchive(szNowDir)){
						ReadArchive(&(GetListTab(m_hTab, 0)->pRoot), szNowDir);
					}else if(IsCueSheet(szNowDir)){
						ReadCueSheet(&(GetListTab(m_hTab, 0)->pRoot), szNowDir);
					}else{
						SearchFolder(&(GetListTab(m_hTab, 0)->pRoot), szNowDir, TRUE);
						MergeSort(&(GetListTab(m_hTab, 0)->pRoot), GetListTab(m_hTab, 0)->nSortState);
					}
					TraverseList(GetListTab(m_hTab, 0));
					SetCursor(LoadCursor(NULL, IDC_ARROW)); // ���J�[�\���ɖ߂�
					TabCtrl_SetCurFocus(m_hTab, 0);
					break;

				case IDM_FIND:
					nFileIndex = (int)DialogBoxParam(m_hInst, "FIND_DIALOG", hWnd, FindDlgProc, (LPARAM)GetCurListTab(m_hTab)->pRoot);
					if(nFileIndex!=-1){
						ListView_SingleSelect(GetCurListTab(m_hTab)->hList, nFileIndex);
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
					}
					break;

				case IDM_TOGGLEMENU:
					if(GetMenu(hWnd)){
						SetMenu(hWnd, NULL);
					}else{
						SetMenu(hWnd, m_hMainMenu);
					}
					break;

				case IDM_MINIPANEL:
					break;

				case IDM_SHOWSTATUS:
					g_cfg.nShowStatus = g_cfg.nShowStatus?0:1;	// ���]
					CheckMenuItem(m_hMainMenu, LOWORD(wp), g_cfg.nShowStatus?MF_CHECKED:MF_UNCHECKED);
					ShowWindow(m_hStatus, g_cfg.nShowStatus?SW_SHOW:SW_HIDE);
					UpdateWindowSize(hWnd);
					break;

				case IDM_SHOWMAIN:	// ���C���c�[���o�[
				case IDM_SHOWVOL:	// �{�����[��
				case IDM_SHOWSEEK:	// �V�[�N�o�[

					ZeroMemory(&rbbi, sizeof(rbbi));
					rbbi.cbSize = sizeof(rbbi);
					rbbi.fMask  = RBBIM_STYLE;
					SendMessage(s_hRebar, RB_GETBANDINFO, (WPARAM)SendMessage(s_hRebar, RB_IDTOINDEX, (WPARAM)(LOWORD(wp)-IDM_SHOWMAIN), 0), (LPARAM)&rbbi);
					if(rbbi.fStyle & RBBS_HIDDEN){
						rbbi.fStyle &= ~RBBS_HIDDEN;
						CheckMenuItem(m_hMainMenu, LOWORD(wp), MF_CHECKED);
					}else{
						rbbi.fStyle |= RBBS_HIDDEN;
						CheckMenuItem(m_hMainMenu, LOWORD(wp), MF_UNCHECKED);
					}
					SendMessage(s_hRebar, RB_SETBANDINFO, (WPARAM)SendMessage(s_hRebar, RB_IDTOINDEX, (WPARAM)(LOWORD(wp)-IDM_SHOWMAIN), 0), (LPARAM)&rbbi);
					break;

				case IDM_FIXBAR:
					//REBARBANDINFO rbbi;
					ZeroMemory(&rbbi, sizeof(rbbi));
					rbbi.cbSize = sizeof(rbbi);
					rbbi.fMask  = RBBIM_STYLE | RBBIM_SIZE;
					rbbi.fStyle = 0;
					for(i=0;i<BAND_COUNT;i++){
						SendMessage(s_hRebar, RB_GETBANDINFO, i, (LPARAM)&rbbi);
						if(rbbi.fStyle & RBBS_NOGRIPPER){
							rbbi.fStyle |= RBBS_GRIPPERALWAYS;
							rbbi.fStyle &= ~RBBS_NOGRIPPER;
							rbbi.cx += 10;
						}else{
							rbbi.fStyle |= RBBS_NOGRIPPER;
							rbbi.fStyle &= ~RBBS_GRIPPERALWAYS;
							rbbi.cx -= 10;
						}
						SendMessage(s_hRebar, RB_SETBANDINFO, i, (LPARAM)&rbbi);
					}
					CheckMenuItem(m_hMainMenu, IDM_FIXBAR, MF_BYCOMMAND | (rbbi.fStyle & RBBS_NOGRIPPER)?MF_CHECKED:MF_UNCHECKED);
					break;

				case IDM_TOGGLETREE:
					switch(g_cfg.nTreeState){
						case TREE_UNSHOWN:
							s_bReviewAllow = FALSE;
							MakeTreeFromPath(m_hTree, m_hCombo, m_szTreePath);
							s_bReviewAllow = TRUE;
							CheckMenuItem(m_hMainMenu, IDM_TOGGLETREE, MF_BYCOMMAND | MF_CHECKED);
							g_cfg.nTreeState = TREE_SHOW;
							break;

						case TREE_SHOW:
							//EnableWindow(m_hTree, FALSE);
							CheckMenuItem(m_hMainMenu, IDM_TOGGLETREE, MF_BYCOMMAND | MF_UNCHECKED);
							g_cfg.nTreeState = TREE_HIDE;
							SetFocus(GetCurListTab(m_hTab)->hList);
							break;

						case TREE_HIDE:
							//EnableWindow(m_hTree, TRUE);
							s_bReviewAllow = FALSE;
							MakeTreeFromPath(m_hTree, m_hCombo, m_szTreePath);
							s_bReviewAllow = TRUE;
							CheckMenuItem(m_hMainMenu, IDM_TOGGLETREE, MF_BYCOMMAND | MF_CHECKED);
							g_cfg.nTreeState = TREE_SHOW;
							break;
					}
					UpdateWindowSize(hWnd);
					break;

				case IDM_SETTING:
					ShowSettingDialog(hWnd, 0);
					break;

				case IDM_BM_ADD: //������ɒǉ�
					if(AddBookMark(GetSubMenu(m_hMainMenu, 3), hWnd)!=-1){
						SetDrivesToCombo(m_hCombo);
					}
					break;

				case IDM_BM_ORG:
					DialogBox(m_hInst, "BOOKMARK_DIALOG", hWnd, (DLGPROC)BookMarkDlgProc);
					DrawBookMark(GetSubMenu(m_hMainMenu, 3));
					break;

				case IDM_BM_PLAYING:
					char *p;

					if(FILE_EXIST(g_cInfo[g_bNow].szFilePath)){
						GetParentDir(g_cInfo[g_bNow].szFilePath, szNowDir);
						MakeTreeFromPath(m_hTree, m_hCombo, szNowDir);
					}else if(IsArchivePath(g_cInfo[g_bNow].szFilePath) || IsCueSheet(g_cInfo[g_bNow].szFilePath)){
						p = StrStr(g_cInfo[g_bNow].szFilePath, "/");
						*p = '\0';
						lstrcpyn(szNowDir, g_cInfo[g_bNow].szFilePath, MAX_FITTLE_PATH);
						*p = '/';
						MakeTreeFromPath(m_hTree, m_hCombo, szNowDir);
					}
					break;

				case IDM_README:	// Readme.txt��\��
					GetModuleParentDir(szNowDir);
					lstrcat(szNowDir, "Readme.txt");
					ShellExecute(hWnd, NULL, szNowDir, NULL, NULL, SW_SHOWNORMAL);
					break;

				case IDM_VER:	// �o�[�W�������
					ShowSettingDialog(hWnd, 5);
					break;

				case IDM_LIST_MOVETOP:	// ��ԏ�Ɉړ�
					ChangeOrder(GetCurListTab(m_hTab), 0);
					break;

				case IDM_LIST_MOVEBOTTOM: //��ԉ��Ɉړ�
					ChangeOrder(GetCurListTab(m_hTab), ListView_GetItemCount(GetCurListTab(m_hTab)->hList) - 1);
					break;

				case IDM_LIST_URL:
					char szLabel[MAX_FITTLE_PATH];
					struct FILEINFO *pSub;

					szLabel[0] = '\0';
					pSub = NULL;
					if(DialogBoxParam(m_hInst, "TAB_NAME_DIALOG", hWnd, AddURLDlgProc, (LPARAM)szLabel)){
						SearchFiles(&pSub, szLabel, FALSE);
						ListView_SetItemState(GetCurListTab(m_hTab)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
						InsertList(GetCurListTab(m_hTab), -1, pSub);
						ListView_EnsureVisible(GetCurListTab(m_hTab)->hList, ListView_GetItemCount(GetCurListTab(m_hTab)->hList)-1, TRUE);
					}
					break;

				case IDM_LIST_NEW:	// �V�K�v���C���X�g
					lstrcpy(szLabel, "Default");
					if(DialogBoxParam(m_hInst, "TAB_NAME_DIALOG", hWnd, TabNameDlgProc, (LPARAM)szLabel)){
						MakeNewTab(m_hTab, szLabel, -1);
						SendToPlaylist(GetCurListTab(m_hTab), GetListTab(m_hTab, TabCtrl_GetItemCount(m_hTab)-1));
						TabCtrl_SetCurFocus(m_hTab, TabCtrl_GetItemCount(m_hTab)-1);
					}
					break;

				case IDM_LIST_ALL:	// �S�đI��
					ListView_SetItemState(GetCurListTab(m_hTab)->hList, -1, LVIS_SELECTED, LVIS_SELECTED);
					break;

				case IDM_LIST_DEL: //���X�g����폜
					DeleteFiles(GetCurListTab(m_hTab));
					break;

				case IDM_LIST_DELFILE:
					// ������
					SHFILEOPSTRUCT fops;
					char szMsg[MAX_FITTLE_PATH];
					int j;
					int q;
					char *pszTarget;
					BOOL bPlaying;
					bPlaying = FALSE;
					j = 0;
					q = 0;
					p = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(char)*2);

					// �p�X��A���Ȃ�
					i = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, -1, LVNI_SELECTED);
					while(i!=-1){
						pszTarget = GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, i)->szFilePath;
						if(i==GetCurListTab(m_hTab)->nPlaying) bPlaying = TRUE;	// �폜�t�@�C�������t��
						if(!IsURLPath(pszTarget) && !IsArchivePath(pszTarget) && !IsCueSheetPath(pszTarget)){	// �폜�ł��Ȃ��t�@�C���łȂ����
							j++;
							p = (char *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, p,
								HeapSize(GetProcessHeap(), 0, p) + (lstrlen(pszTarget) + 2) * sizeof(char));
							lstrcpy(p+q, pszTarget);
							q += lstrlen(pszTarget) + 1;
							*(p+q) = '\0';
						}
						i = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, i, LVNI_SELECTED);
					}
					if(j==1){
						// �t�@�C����I��
						wsprintf(szMsg, "%'%s%' �����ݔ��Ɉڂ��܂����H", p);
					}else if(j>1){
						// �����t�@�C���I��
						wsprintf(szMsg, "������ %d �̍��ڂ����ݔ��Ɉڂ��܂����H", j);
					}
					if(j>0 && MessageBox(hWnd, szMsg, "�t�@�C���̍폜�̊m�F", MB_YESNO | MB_ICONQUESTION)==IDYES){
						// ���ۂɍ폜
						fops.hwnd = hWnd;
						fops.wFunc = FO_DELETE;
						fops.pFrom = p;
						fops.pTo = NULL;
						fops.fFlags = FOF_FILESONLY | FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_MULTIDESTFILES;
						if(bPlaying) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_STOP, 0), 0);
						if(SHFileOperation(&fops)==0){
							if(bPlaying) PopPrevious(GetCurListTab(m_hTab));	// ��������Ƃ肠�����Ōゾ���폜
							DeleteFiles(GetCurListTab(m_hTab));
						}
						if(bPlaying) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
					}
					HeapFree(GetProcessHeap(), 0, p);
					break;

				case IDM_LIST_TOOL:
					if((nLBIndex = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, -1, LVNI_SELECTED))!=-1){
						if(lstrlen(g_cfg.szToolPath)!=0){
							LPSTR pszFiles;
							pszFiles = MallocAndConcatPath(GetCurListTab(m_hTab));
							MyShellExecute(hWnd, g_cfg.szToolPath, pszFiles, TRUE);
							HeapFree(GetProcessHeap(), 0, pszFiles);
						}else{
							ShowSettingDialog(hWnd, 1);
						}
					}
					break;

				case IDM_LIST_PROP:	// �v���p�e�B
					SHELLEXECUTEINFO sei;
					
					if((nLBIndex = ListView_GetNextItem(GetCurListTab(m_hTab)->hList, -1, LVNI_SELECTED))!=-1){
						ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
						sei.cbSize = sizeof(SHELLEXECUTEINFO);
						sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
						sei.lpFile = GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, nLBIndex)->szFilePath;
						sei.lpVerb = "properties";
						sei.hwnd = NULL;
						ShellExecuteEx(&sei);
					}
					break;

				case IDM_TREE_UP:
					HTREEITEM hParent;

					if(g_cfg.nTreeState==TREE_UNSHOWN){
						if(!PathIsRoot(m_szTreePath)){
							*PathFindFileName(m_szTreePath) = '\0';
							MakeTreeFromPath(m_hTree, m_hCombo, m_szTreePath);
						}
					}else{
						if(!m_hHitTree) m_hHitTree = TreeView_GetNextItem(m_hTree, TVI_ROOT, TVGN_CARET);
						hParent = TreeView_GetParent(m_hTree, m_hHitTree);
						if(hParent!=NULL){
							TreeView_Select(m_hTree, hParent, TVGN_CARET);
							MyScroll(m_hTree);
						}else{
							GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
							if(szNowDir[3]){
								PathRemoveFileSpec(szNowDir);
								MakeTreeFromPath(m_hTree, m_hCombo, szNowDir);
							}
						}
						m_hHitTree = NULL;
					}
					break;

				case IDM_TREE_SUB:
					TreeView_Select(m_hTree, m_hHitTree, TVGN_CARET);
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_SUB, 0);
					m_hHitTree = NULL;
					break;

				case IDM_TREE_ADD:
				//case IDM_TREE_SUBADD:
					pSub = NULL;

					GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					SearchFiles(&pSub, szNowDir, TRUE);
					ListView_SetItemState(GetCurListTab(m_hTab)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
					InsertList(GetCurListTab(m_hTab), -1, pSub);
					ListView_EnsureVisible(GetCurListTab(m_hTab)->hList, ListView_GetItemCount(GetCurListTab(m_hTab)->hList)-1, TRUE);
					m_hHitTree = NULL;
					break;

				case IDM_TREE_NEW:
					struct LISTTAB *pNew;

					GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					lstrcpyn(szLabel, PathFindFileName(szNowDir), MAX_FITTLE_PATH);
					if(IsPlayList(szNowDir) || IsArchive(szNowDir) || IsCueSheet(szNowDir)){
						// ".m3u"���폜
						PathRemoveExtension(szLabel);
					}
					if(DialogBoxParam(m_hInst, "TAB_NAME_DIALOG", hWnd, TabNameDlgProc, (LPARAM)szLabel)){
						pNew = MakeNewTab(m_hTab, szLabel, -1);
						SearchFiles(&(pNew->pRoot), szNowDir, TRUE);
						TraverseList(pNew);
						TabCtrl_SetCurFocus(m_hTab, TabCtrl_GetItemCount(m_hTab)-1);
						InvalidateRect(m_hTab, NULL, FALSE);
					}
					m_hHitTree = NULL;
					break;

				case IDM_EXPLORE:
				case IDM_TREE_EXPLORE:
					// �p�X�̎擾
					if(LOWORD(wp)==IDM_EXPLORE){
						lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					}else{
						GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					}
					// �O����
					if(IsPlayList(szNowDir) || IsArchive(szNowDir) || IsCueSheet(szNowDir)){
						*PathFindFileName(szNowDir) = '\0';
					}else{
						MyPathAddBackslash(szNowDir);
					}
					// �G�N�X�v���[���ɓ����鏈��
					MyShellExecute(hWnd, (g_cfg.szFilerPath[0]?g_cfg.szFilerPath:"explorer.exe"), szNowDir, FALSE);
					m_hHitTree = NULL;
					break;

				case IDM_TAB_NEW:
					lstrcpy(szLabel, "Default");
					if(DialogBoxParam(m_hInst, "TAB_NAME_DIALOG", hWnd, TabNameDlgProc, (LPARAM)szLabel)){
						MakeNewTab(m_hTab, szLabel, -1);
						TabCtrl_SetCurFocus(m_hTab, TabCtrl_GetItemCount(m_hTab)-1);
					}
					break;

				case IDM_TAB_RENAME:
					lstrcpyn(szLabel, GetListTab(m_hTab, s_nHitTab)->szTitle, MAX_FITTLE_PATH);
					if(DialogBoxParam(m_hInst, "TAB_NAME_DIALOG", hWnd, TabNameDlgProc, (LPARAM)szLabel)){
						RenameListTab(m_hTab, s_nHitTab, szLabel);
						InvalidateRect(m_hTab, NULL, FALSE);
					}
					break;

				case IDM_TAB_DEL:
					// �J�����g�^�u�������Ȃ獶���J�����g�ɂ���
					if(TabCtrl_GetCurSel(m_hTab)==s_nHitTab){
						TabCtrl_SetCurFocus(m_hTab, s_nHitTab-1);
					}
					// �Đ��^�u�������Ȃ�Đ��^�u���N���A
					if(s_nHitTab==m_nPlayTab){
						m_nPlayTab = -1;
					}
					RemoveListTab(m_hTab, s_nHitTab);
					break;

				case IDM_TAB_LEFT:	// ���ֈړ�
					MoveTab(m_hTab, s_nHitTab, -1);
					break;

				case IDM_TAB_RIGHT:	// �E�ֈړ�
					MoveTab(m_hTab, s_nHitTab, 1);
					break;

				case IDM_TAB_FOR_RIGHT:
					TabCtrl_SetCurFocus(m_hTab, (TabCtrl_GetCurSel(m_hTab)+1==TabCtrl_GetItemCount(m_hTab)?0:TabCtrl_GetCurSel(m_hTab)+1));
					break;

				case IDM_TAB_FOR_LEFT:
					TabCtrl_SetCurFocus(m_hTab, (TabCtrl_GetCurSel(m_hTab)?TabCtrl_GetCurSel(m_hTab)-1:TabCtrl_GetItemCount(m_hTab)-1));
					break;

				case IDM_TAB_SORT:
					SortListTab(GetCurListTab(m_hTab), 0);
					break;

				case IDM_TAB_NOEXIST:
					ListView_SetItemCountEx(GetCurListTab(m_hTab)->hList, LinkCheck(&(GetCurListTab(m_hTab)->pRoot)), 0);
					break;

				case IDM_TRAY_WINVIEW:
					ToggleWindowView(hWnd);
					break;

				case IDM_TOGGLEFOCUS:
					if(GetFocus()!=GetCurListTab(m_hTab)->hList){
						SetFocus(GetCurListTab(m_hTab)->hList);
					}else{
						SetFocus(m_hTree);
					}
					break;

				case ID_COMBO:
					if(HIWORD(wp)==CBN_SELCHANGE){
						TreeView_Select(m_hTree, MakeDriveNode(m_hCombo, m_hTree), TVGN_CARET);
					}
					break;

				default:
					if(IDM_SENDPL_FIRST<=LOWORD(wp) && LOWORD(wp)<IDM_BM_FIRST){
						//�v���C���X�g�ɑ���
						SendToPlaylist(GetCurListTab(m_hTab), GetListTab(m_hTab, LOWORD(wp) - IDM_SENDPL_FIRST));
					}else if(IDM_BM_FIRST<=LOWORD(wp) && LOWORD(wp)<IDM_BM_FIRST+MAX_BM_SIZE){
						// ������
						if(g_cfg.nTreeState==TREE_SHOW){
							MakeTreeFromPath(m_hTree, m_hCombo, g_cfg.szBMPath[LOWORD(wp)-IDM_BM_FIRST]);
						}else{
							lstrcpyn(m_szTreePath, g_cfg.szBMPath[LOWORD(wp)-IDM_BM_FIRST], MAX_FITTLE_PATH);
							SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FILEREVIEW, 0), 0);
						}
					}else{
						return DefWindowProc(hWnd, msg, wp, lp);
					}
					break;
			}
			break;

		case WM_SYSCOMMAND:
			switch (wp) {
				case IDM_PLAY: //�Đ�
				case IDM_PAUSE: //�ꎞ��~
				case IDM_PLAYPAUSE:
				case IDM_STOP: //��~
				case IDM_PREV: //�O�̋�
				case IDM_NEXT: //���̋�
				case IDM_PM_LIST: // ���X�g
				case IDM_PM_RANDOM: // �����_��
				case IDM_PM_SINGLE: // �V���O��
				case IDM_PM_REPEAT: // ���s�[�g
				case IDM_TOGGLEMENU:
				case IDM_END: // �I��
				case IDM_TRAY_WINVIEW:
					SendMessage(hWnd, WM_COMMAND, wp, 0);
					break;

				case SC_CLOSE:
					if(g_cfg.nCloseMin){
						ShowWindow(hWnd, SW_MINIMIZE);
						return 0;
					}

				default:
					return DefWindowProc(hWnd, msg, wp, lp);
			}
			break;

		case WM_CONTEXTMENU:
			HMENU hPopMenu;
			RECT rcItem;

			if((HWND)wp==hWnd){
				return DefWindowProc(hWnd, msg, wp, lp);		
			}else{
				switch(GetDlgCtrlID((HWND)wp))
				{
					case ID_TREE:
						TVHITTESTINFO tvhti;

						if(lp==-1){	// �L�[�{�[�h
							m_hHitTree = TreeView_GetNextItem(m_hTree, TVI_ROOT, TVGN_CARET);
							if(!m_hHitTree) break; 
							TreeView_GetItemRect(m_hTree, m_hHitTree, &rcItem, TRUE);
							MapWindowPoints(m_hTree, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
							lp = MAKELPARAM(rcItem.left, rcItem.bottom);
						}else{	// �}�E�X
							tvhti.pt.x = (short)LOWORD(lp);
							tvhti.pt.y = (short)HIWORD(lp);
							ScreenToClient(m_hTree, &tvhti.pt);
							TreeView_HitTest(m_hTree, &tvhti);
							m_hHitTree = tvhti.hItem;
						}
						TreeView_SelectDropTarget(m_hTree, m_hHitTree);
						hPopMenu = GetSubMenu(LoadMenu(m_hInst, "TREEMENU"), 0);
						TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, hWnd, NULL);
						DestroyMenu(hPopMenu);
						TreeView_SelectDropTarget(m_hTree, NULL);
						break;

					case ID_TAB:
						TCHITTESTINFO tchti;

						if(lp==-1){	// �L�[�{�[�h
							s_nHitTab = TabCtrl_GetCurSel(m_hTab);
							TabCtrl_GetItemRect(m_hTab, s_nHitTab, &rcItem);
							MapWindowPoints(m_hTab, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
							lp = MAKELPARAM(rcItem.left, rcItem.bottom);
						}else{	// �}�E�X
							tchti.pt.x = (short)LOWORD(lp);
							tchti.pt.y = (short)HIWORD(lp);
							tchti.flags = TCHT_NOWHERE;
							ScreenToClient(m_hTab, &tchti.pt);
							s_nHitTab = TabCtrl_HitTest(m_hTab, &tchti);
						}
						hPopMenu = GetSubMenu(LoadMenu(m_hInst, "TABMENU"), 0);
						// �O���[�\���̏���
						if(s_nHitTab<=0){
							EnableMenuItem(hPopMenu, IDM_TAB_DEL, MF_GRAYED | MF_DISABLED);
							EnableMenuItem(hPopMenu, IDM_TAB_RENAME, MF_GRAYED | MF_DISABLED);
						}
						if(s_nHitTab<=1) EnableMenuItem(hPopMenu, IDM_TAB_LEFT, MF_GRAYED | MF_DISABLED);
						if(s_nHitTab>=TabCtrl_GetItemCount(m_hTab)-1 || s_nHitTab<=0)
							EnableMenuItem(hPopMenu, IDM_TAB_RIGHT, MF_GRAYED | MF_DISABLED);
						TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, hWnd, NULL);
						DestroyMenu(hPopMenu);
						break;

					case ID_REBAR:
						POINT pt;
						GetCursorPos(&pt);
						TrackPopupMenu(GetSubMenu(GetSubMenu(m_hMainMenu, 2), 12), TPM_LEFTALIGN | TPM_TOPALIGN, /*(short)LOWORD(lp)*/pt.x, /*(short)HIWORD(lp)*/pt.y, 0, hWnd, NULL);
						break;
				}
			}
			break;


		case WM_TRAY:
			switch(lp){
				case WM_LBUTTONDOWN:
					if(s_nClickState==0){
						s_nClickState = 1;
						SetTimer(hWnd, ID_LBTNCLKTIMER, (g_cfg.nTrayClick[1]?GetDoubleClickTime():0), NULL);
					}
					break;

				case WM_RBUTTONDOWN:
					if(s_nClickState==0){
						s_nClickState = 1;
						SetTimer(hWnd, ID_RBTNCLKTIMER, (g_cfg.nTrayClick[3]?GetDoubleClickTime():0), NULL);
					}
					break;

				case WM_MBUTTONDOWN:
					if(s_nClickState==0){
						s_nClickState = 1;
						SetTimer(hWnd, ID_MBTNCLKTIMER, (g_cfg.nTrayClick[5]?GetDoubleClickTime():0), NULL);
					}
					break;

				case WM_LBUTTONUP:
					if(s_nClickState==1){
						s_nClickState = 2;
					}else if(s_nClickState==3){
						DoTrayClickAction(hWnd, 0);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_RBUTTONUP:
					if(s_nClickState==1){
						s_nClickState = 2;
					}else if(s_nClickState==3){
						DoTrayClickAction(hWnd, 2);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_MBUTTONUP:
					if(s_nClickState==1){
						s_nClickState = 2;
					}else if(s_nClickState==3){
						DoTrayClickAction(hWnd, 4);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_LBUTTONDBLCLK:
					KillTimer(hWnd, ID_LBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(hWnd, 1);
					break;

				case WM_RBUTTONDBLCLK:
					KillTimer(hWnd, ID_RBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(hWnd, 3);
					break;

				case WM_MBUTTONDBLCLK:
					KillTimer(hWnd, ID_MBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(hWnd, 5);
					break;

				default:
					return DefWindowProc(hWnd, msg, wp, lp);
			}
			break;

		case WM_TIMER:
			QWORD qPos;
			QWORD qLen;
			int nPos;
			int nLen;

			switch (wp){
				case ID_SEEKTIMER:
					//�Đ����Ԃ��X�e�[�^�X�o�[�ɕ\��
					qPos = BASS_ChannelGetPosition(g_cInfo[g_bNow].hChan, BASS_POS_BYTE) - g_cInfo[g_bNow].qStart;
					qLen = g_cInfo[g_bNow].qDuration;
					nPos = (int)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, qPos);
					nLen = (int)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, qLen);
					wsprintf(m_szTime, "\t%02d:%02d / %02d:%02d", nPos/60, nPos%60, nLen/60, nLen%60);
					PostMessage(m_hStatus, SB_SETTEXT, (WPARAM)2|0, (LPARAM)m_szTime);
					//�V�[�N���łȂ���Ό��݂̍Đ��ʒu���X���C�_�o�[�ɕ\������
					if(!(GetCapture()==m_hSeek || qLen==0))
						PostMessage(m_hSeek, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SLIDER_DIVIDED * qPos / qLen));
					break;

				case ID_TIPTIMER:
					//�c�[���`�b�v������
					SendMessage(m_hTitleTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, NULL);
					KillTimer(hWnd, ID_TIPTIMER);
					break;

				case ID_LBTNCLKTIMER:
					KillTimer(hWnd, ID_LBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(hWnd, 0);
						s_nClickState = 0;
					}else if(s_nClickState==1){
						s_nClickState = 3;
					}
					break;

				case ID_RBTNCLKTIMER:
					KillTimer(hWnd, ID_RBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(hWnd, 2);
						s_nClickState = 0;
					}else if(s_nClickState==1){
						s_nClickState = 3;
					}
					break;

				case ID_MBTNCLKTIMER:
					KillTimer(hWnd, ID_MBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(hWnd, 4);
						s_nClickState = 0;
					}else if(s_nClickState==1){
						s_nClickState = 3;
					}
					break;

				default:
					//���̃^�C�}�[��Default�ɂ܂�����
					return DefWindowProc(hWnd, msg, wp, lp);
			}
			break;

		case WM_NOTIFY:
			NMHDR *pnmhdr;

			pnmhdr = (LPNMHDR)lp;
			switch(pnmhdr->idFrom){
				case ID_TREE:
					NMTREEVIEW *pnmtv;
					switch(pnmhdr->code){
						case TVN_ITEMEXPANDED: // �\���ʒu����
							MyScroll(m_hTree);
							break;

						case TVN_SELCHANGED: // �t�H���_����������猟��
							pnmtv = (LPNMTREEVIEW)lp;
							GetPathFromNode(m_hTree, pnmtv->itemNew.hItem, m_szTreePath);
							SendMessage(hWnd, WM_COMMAND, IDM_FILEREVIEW, 0);
							break;

						case TVN_ITEMEXPANDING: // �c���[���J������T�u�m�[�h��ǉ�
							pnmtv = (LPNMTREEVIEW)lp; 
							if(pnmtv->action==TVE_EXPAND){
								MakeTwoTree(m_hTree, (pnmtv->itemNew).hItem);
							}
							break;

						case TVN_BEGINDRAG:	// �h���b�O�J�n
							pnmtv = (LPNMTREEVIEW)lp;
							m_hHitTree = pnmtv->itemNew.hItem;
							OnBeginDragTree(m_hTree);
							break;
					}
					break;

				case ID_TAB:
					switch(pnmhdr->code)
					{
						case TCN_SELCHANGING:
							LockWindowUpdate(hWnd);
							ShowWindow(GetCurListTab(m_hTab)->hList, SW_HIDE);
							break;
						
						case TCN_SELCHANGE:
							GetClientRect(m_hTab, &rcTab);
							if(!(g_cfg.nTabHide && TabCtrl_GetItemCount(m_hTab)==1)){
								TabCtrl_AdjustRect(m_hTab, FALSE, &rcTab);
							}
							SendMessage(GetCurListTab(m_hTab)->hList, WM_SETFONT, (WPARAM)m_hFont, 0);
							SetWindowPos(GetCurListTab(m_hTab)->hList,
								HWND_TOP,
								rcTab.left,
								rcTab.top,
								rcTab.right - rcTab.left,
								rcTab.bottom - rcTab.top,
								SWP_SHOWWINDOW);
							SetFocus(GetCurListTab(m_hTab)->hList);
							LockWindowUpdate(NULL);
							break;
					}
					break;

				case ID_REBAR:
					// ���o�[�̍s�����ς������WM_SIZE�𓊂���
					if(pnmhdr->code==RBN_HEIGHTCHANGE){
						UpdateWindow(s_hRebar);
						UpdateWindowSize(hWnd);
					}
					break;

				case ID_STATUS:
					if(pnmhdr->code==NM_DBLCLK){
						SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_MINIPANEL, 0);
					}
					break;

				default:
					// �c�[���o�[�̃c�[���`�b�v�̕\��
					if(pnmhdr->code==TTN_NEEDTEXT){ 
						TOOLTIPTEXT *lptip;
						lptip = (LPTOOLTIPTEXT)lp; 
						switch (lptip->hdr.idFrom){ 
							case IDM_PLAY: 
								lptip->lpszText = "�Đ� (Z)"; 
								break; 
							case IDM_PAUSE: 
								lptip->lpszText = "�ꎞ��~ (X)"; 
								break; 
							case IDM_STOP: 
								lptip->lpszText = "��~ (C)"; 
								break; 
							case IDM_PREV: 
								lptip->lpszText = "�O�̋� (V)"; 
								break; 
							case IDM_NEXT: 
								lptip->lpszText = "���̋� (B)"; 
								break; 
							case IDM_PM_TOGGLE:
								switch (m_nPlayMode)
								{
									case PM_LIST:
										lptip->lpszText = "���X�g"; 
										break;
									case PM_RANDOM:
										lptip->lpszText = "�����_��"; 
										break;
									case PM_SINGLE:
										lptip->lpszText = "�V���O��"; 
										break;
								}
								break; 
							case IDM_PM_REPEAT: 
								lptip->lpszText = "���s�[�g (Ctrl+R)"; 
								break;
						}
					}else if(pnmhdr->code==TBN_DROPDOWN){
						// �h���b�v�_�E�����j���[�̕\��
						PopupPlayModeMenu(pnmhdr->hwndFrom, (LPNMTOOLBAR)lp);
					}
					break;
			}
			break;

		case WM_HOTKEY: // �z�b�g�L�[
			switch((int)wp){
				case 0:	// �Đ�
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 0), 0);
					break;
				case 1:	// �Đ�/�ꎞ��~
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLAYPAUSE, 0), 0);
					break;
				case 2:	// ��~
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_STOP, 0), 0);
					break;
				case 3:	// �O�̋�
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PREV, 0), 0);
					break;
				case 4:	// ���̋�
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_NEXT, 0), 0);
					break;
				case 5:	// ���ʂ��グ��
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_VOLUP, 0);
					break;
				case 6:	// ���ʂ�������
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_VOLDOWN, 0);
					break;
				case 7: // �^�X�N�g���C���畜�A
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_TRAY_WINVIEW, 0), 0);
					break;
			}
			break;

		case WM_FITTLE:	// �v���O�C���C���^�[�t�F�C�X
			switch(wp){
				case GET_TITLE:
					return (LRESULT)&m_taginfo;

				case GET_ARTIST:
					return (LRESULT)m_taginfo.szArtist;

				case GET_PLAYING_PATH:
					return (LRESULT)g_cInfo[g_bNow].szFilePath;

				case GET_PLAYING_INDEX:
					return (LRESULT)GetCurListTab(m_hTab)->nPlaying;

				case GET_STATUS:
					return (LRESULT)BASS_ChannelIsActive(m_hChanOut);

				case GET_POSITION:
					return (LRESULT)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, BASS_ChannelGetPosition(g_cInfo[g_bNow].hChan, BASS_POS_BYTE) - g_cInfo[g_bNow].qStart);

				case GET_DURATION:
					return (LRESULT)BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, g_cInfo[g_bNow].qDuration);

				case GET_LISTVIEW:
					int nCount;

					if(lp<0){
						return (LRESULT)GetCurListTab(m_hTab)->hList;
					}else{
						nCount = TabCtrl_GetItemCount(m_hTab);
						for(i=0;i<nCount;i++){
							if(lp==i){
								return (LRESULT)GetListTab(m_hTab, (int)lp)->hList;
							}
						}
					}
					return NULL;

				case GET_CONTROL:
					switch(lp){
						case ID_COMBO:
						case ID_TREE:
						case ID_TAB:
						case ID_REBAR:
						case ID_STATUS:
							return (LRESULT)GetDlgItem(hWnd, lp);
						case TD_SEEKBAR:
						case ID_TOOLBAR:
						case ID_VOLUME:
							return (LRESULT)GetDlgItem(s_hRebar, lp);
					}
					break;

				case GET_CURPATH:
					return (LRESULT)(const char *)m_szTreePath;

				case GET_MENU:
					return (HRESULT)m_hMainMenu;

				case GET_MINIPANEL:
					return 0;

				default:
					break;
			}
			break;

		case WM_DEVICECHANGE:	// �h���C�u�̐��̕ύX�ɑΉ�
			switch (wp)
			{
				case 0x8000://DBT_DEVICEARRIVAL:
				case 0x8004://DBT_DEVICEREMOVECOMPLETE:
					SetDrivesToCombo(m_hCombo);
					break;
			}
			break;

		default:
			// �^�X�N�g���C���ĕ`�悳�ꂽ��A�C�R�����ĕ`��
			if((msg == s_uTaskbarRestart) && m_bTrayFlag){
				SetTaskTray(hWnd);
			}
			return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0L;
}

static void DoTrayClickAction(HWND hWnd, int nKind){
	switch(g_cfg.nTrayClick[nKind]){
		case 1:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_PLAY, 0);
			break;
		case 2:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_PAUSE, 0);
			break;
		case 3:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_STOP, 0);
			break;
		case 4:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_PREV, 0);
			break;
		case 5:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_NEXT, 0);
			break;
		case 6:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_TRAY_WINVIEW, 0);
			break;
		case 7:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_END, 0);
			break;
		case 8:
			PopupTrayMenu(hWnd);
			break;
		case 9:
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_PLAYPAUSE, 0);
			break;

	}
	return;
}

static void PopupTrayMenu(HWND hWnd){
	POINT pt;

	GetCursorPos(&pt);
	SetForegroundWindow(hWnd);
	TrackPopupMenu(m_hTrayMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
	PostMessage(hWnd, WM_NULL, 0, 0);
	return;
}

static void PopupPlayModeMenu(HWND hWnd, NMTOOLBAR *lpnmtb){
	RECT rc;
	MENUITEMINFO mii;
	HMENU hPopMenu;

	SendMessage(hWnd, TB_GETRECT, (WPARAM)lpnmtb->iItem, (LPARAM)&rc);
	MapWindowPoints(hWnd, HWND_DESKTOP, (LPPOINT)&rc, 2);
	hPopMenu = CreatePopupMenu();
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.cch = 100;
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	mii.fType = MFT_STRING;
	mii.dwTypeData = "���X�g(&L)";
	mii.wID = IDM_PM_LIST;
	if(m_nPlayMode==PM_LIST) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, 0, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = "�����_��(&R)";
	mii.wID = IDM_PM_RANDOM;
	if(m_nPlayMode==PM_RANDOM) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, IDM_PM_LIST, TRUE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = "�V���O��(&S)";
	mii.wID = IDM_PM_SINGLE;
	if(m_nPlayMode==PM_SINGLE) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, IDM_PM_RANDOM, TRUE, &mii);
	mii.fState = MFS_UNCHECKED;
	TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, rc.left, rc.bottom, 0, hWnd, NULL);
	DestroyMenu(hPopMenu);
	return;
}

// �O���[�o���Ȑݒ��ǂݍ���
static void LoadConfig(){
	int i;
	char szSec[10];

	// �V�X�e���̗D��x�̐ݒ�
	g_cfg.nHighTask = GetPrivateProfileInt("Main", "Priority", 0, m_szINIPath);
	// �R���g���[���J���[
	g_cfg.nBkColor = (int)GetPrivateProfileInt("Color", "BkColor", (int)GetSysColor(COLOR_WINDOW), m_szINIPath);
	g_cfg.nTextColor = (int)GetPrivateProfileInt("Color", "TextColor", (int)GetSysColor(COLOR_WINDOWTEXT), m_szINIPath);
	g_cfg.nPlayTxtCol = (int)GetPrivateProfileInt("Color", "PlayTxtCol", (int)RGB(0xFF, 0, 0), m_szINIPath);
	g_cfg.nPlayBkCol = (int)GetPrivateProfileInt("Color", "PlayBkCol", (int)RGB(230, 234, 238), m_szINIPath);
	g_cfg.nMiniPanel_x = (int)GetPrivateProfileInt("MiniPanel", "x", 0, m_szINIPath);
	g_cfg.nMiniPanel_y = (int)GetPrivateProfileInt("MiniPanel", "y", 0, m_szINIPath);
	g_cfg.nMiniPanelEnd = (int)GetPrivateProfileInt("MiniPanel", "End", 0, m_szINIPath);
	g_cfg.nMiniTop = (int)GetPrivateProfileInt("MiniPanel", "TopMost", 1, m_szINIPath);
	g_cfg.nMiniWidth = (int)GetPrivateProfileInt("MiniPanel", "Width", 402, m_szINIPath);
	g_cfg.nMiniScroll = (int)GetPrivateProfileInt("MiniPanel", "Scroll", 1, m_szINIPath);
	g_cfg.nMiniTimeShow = (int)GetPrivateProfileInt("MiniPanel", "TimeShow", 1, m_szINIPath);
	g_cfg.nMiniToolShow = (int)GetPrivateProfileInt("MiniPanel", "ToolShow", 1, m_szINIPath);
	// �O���b�h���C��
	g_cfg.nGridLine = GetPrivateProfileInt("Main", "GridLine", 1, m_szINIPath);
	g_cfg.nSingleExpand = GetPrivateProfileInt("Main", "SingleExp", 0, m_szINIPath);
	// ���݊m�F
	g_cfg.nExistCheck = GetPrivateProfileInt("Main", "ExistCheck", 0, m_szINIPath);
	// �v���C���X�g�ōX�V�������擾����
	g_cfg.nTimeInList = GetPrivateProfileInt("Main", "TimeInList", 0, m_szINIPath);
	// �c���[�̕���ݒ�
	g_cfg.nTreeWidth = GetPrivateProfileInt("Main", "TreeWidth", 200, m_szINIPath);
	//g_cfg.nTWidthSub = GetPrivateProfileInt("Main", "TWidthSub", 200, m_szINIPath);
	g_cfg.nTreeState = GetPrivateProfileInt("Main", "TreeState", TREE_SHOW, m_szINIPath);
	// �X�e�[�^�X�o�[�\����\��
	g_cfg.nShowStatus =  GetPrivateProfileInt("Main", "ShowStatus", 1, m_szINIPath);
	// �V�X�e���C���[�W���X�g������
	g_cfg.nTreeIcon = GetPrivateProfileInt("Main", "TreeIcon", TRUE, m_szINIPath);
	// �^�X�N�g���C�Ɏ��[�̃`�F�b�N
	g_cfg.nTrayOpt = GetPrivateProfileInt("Main", "Tray", 1, m_szINIPath);
	// �c���[�\���ݒ�
	g_cfg.nHideShow = GetPrivateProfileInt("Main", "HideShow", 0, m_szINIPath);
	// �^�u�����ɕ\������
	g_cfg.nTabBottom = GetPrivateProfileInt("Main", "TabBottom", 0, m_szINIPath);
	// ���i�ŕ\������
	g_cfg.nTabMulti = GetPrivateProfileInt("Main", "TabMulti", 1, m_szINIPath);
	// ���ׂẴt�H���_���`
	g_cfg.nAllSub = GetPrivateProfileInt("Main", "AllSub", 0, m_szINIPath);
	// �c�[���`�b�v�Ńt���p�X��\��
	g_cfg.nPathTip = GetPrivateProfileInt("Main", "PathTip", 1, m_szINIPath);
	// �Ȗ����m�点�@�\
	g_cfg.nInfoTip = GetPrivateProfileInt("Main", "Info", 1, m_szINIPath);
	// �\�����@
	g_cfg.nPlayView = GetPrivateProfileInt("Color", "PlayView", 1, m_szINIPath);
	// �^�O�𔽓]
	g_cfg.nTagReverse = GetPrivateProfileInt("Main", "TagReverse", 0, m_szINIPath);
	// �w�b�_�R���g���[����\������
	g_cfg.nShowHeader = GetPrivateProfileInt("Main", "ShowHeader", 1, m_szINIPath);
	// �V�[�N��
	g_cfg.nSeekAmount = GetPrivateProfileInt("Main", "SeekAmount", 5, m_szINIPath);
	g_cfg.nVolAmount = GetPrivateProfileInt("Main", "VolAmount", 5, m_szINIPath);
	// �I�����ɍĐ����Ă����Ȃ��N�����ɂ��Đ�����
	g_cfg.nResume = GetPrivateProfileInt("Main", "Resume", 0, m_szINIPath);
	g_cfg.nResPosFlag = GetPrivateProfileInt("Main", "ResPosFlag", 0, m_szINIPath);
	g_cfg.nResPos = GetPrivateProfileInt("Main", "ResPos", 0, m_szINIPath);
	// ����{�^���ōŏ�������
	g_cfg.nCloseMin = GetPrivateProfileInt("Main", "CloseMin", 0, m_szINIPath);
	// �T�u�t�H���_�������ň��k�t�@�C������������
	g_cfg.nZipSearch = GetPrivateProfileInt("Main", "ZipSearch", 0, m_szINIPath);
	// �^�u����̎��̓^�u���B��
	g_cfg.nTabHide = GetPrivateProfileInt("Main", "TabHide", 0, m_szINIPath);
	// 32bit(float)�ŏo�͂���
	g_cfg.nOut32bit = GetPrivateProfileInt("Main", "Out32bit", 0, m_szINIPath);
	m_bFLoat = (BOOL)g_cfg.nOut32bit;
	// ��~���Ƀt�F�[�h�A�E�g����
	g_cfg.nFadeOut = GetPrivateProfileInt("Main", "FadeOut", 1, m_szINIPath);
	// �X�^�[�g�A�b�v�t�H���_�ǂݍ���
	GetPrivateProfileString("Main", "StartPath", "", g_cfg.szStartPath, MAX_FITTLE_PATH, m_szINIPath);
	// �z�b�g�L�[�̐ݒ�
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintf(szSec, "HotKey%d", i);
		g_cfg.nHotKey[i] = GetPrivateProfileInt("HotKey", szSec, 0, m_szINIPath);
	}
	// �N���b�N���̓���
	g_cfg.nTrayClick[0] = GetPrivateProfileInt("TaskTray", "Click0", 6, m_szINIPath);
	g_cfg.nTrayClick[1] = GetPrivateProfileInt("TaskTray", "Click1", 0, m_szINIPath);
	g_cfg.nTrayClick[2] = GetPrivateProfileInt("TaskTray", "Click2", 8, m_szINIPath);
	g_cfg.nTrayClick[3] = GetPrivateProfileInt("TaskTray", "Click3", 0, m_szINIPath);
	g_cfg.nTrayClick[4] = GetPrivateProfileInt("TaskTray", "Click4", 5, m_szINIPath);
	g_cfg.nTrayClick[5] = GetPrivateProfileInt("TaskTray", "Click5", 0, m_szINIPath);
	// �t�H���g�ݒ�ǂݍ���
	m_hFont = NULL;
	GetPrivateProfileString("Font", "FontName", "", g_cfg.szFontName, 32, m_szINIPath);	// �t�H���g��""���f�t�H���g�̈�
	g_cfg.nFontHeight = GetPrivateProfileInt("Font", "Height", 10, m_szINIPath);
	g_cfg.nFontStyle = GetPrivateProfileInt("Font", "Style", 0, m_szINIPath);
	// ����������[�g�Ƃ��Ĉ���
	g_cfg.nBMRoot = GetPrivateProfileInt("BookMark", "BMRoot", 1, m_szINIPath);
	// ��������t���p�X�ŕ\��
	g_cfg.nBMFullPath = GetPrivateProfileInt("BookMark", "BMFullPath", 1, m_szINIPath);
	// �t�@�C���̃p�X
	GetPrivateProfileString("Main", "FilerPath", "", g_cfg.szFilerPath, MAX_FITTLE_PATH, m_szINIPath);
	// �Ō�ɍĐ����Ă����t�@�C��
	GetPrivateProfileString("Main", "LastFile", "", g_cfg.szLastFile, MAX_FITTLE_PATH, m_szINIPath);
	// �O���c�[��
	GetPrivateProfileString("Tool", "Path0", "", g_cfg.szToolPath, MAX_FITTLE_PATH, m_szINIPath);
	return;
}

// �ݒ��ۑ�
static void SaveConfig(HWND hWnd){
	int i;
	char szLastPath[MAX_FITTLE_PATH];
	char szSec[10];
	WINDOWPLACEMENT wpl;
	REBARBANDINFO rbbi;

	wpl.length = sizeof(WINDOWPLACEMENT);

	lstrcpyn(szLastPath, m_szTreePath, MAX_FITTLE_PATH);
	// �R���p�N�g���[�h���l�����Ȃ���E�B���h�E�T�C�Y��ۑ�
	GetWindowPlacement(hWnd, &wpl);
	WritePrivateProfileInt("Main", "Maximized", (wpl.showCmd==SW_SHOWMAXIMIZED || wpl.flags & WPF_RESTORETOMAXIMIZED), m_szINIPath);	// �ő剻
	WritePrivateProfileInt("Main", "Top", wpl.rcNormalPosition.top, m_szINIPath); //�E�B���h�E�ʒuTop
	WritePrivateProfileInt("Main", "Left", wpl.rcNormalPosition.left, m_szINIPath); //�E�B���h�E�ʒuLeft
	WritePrivateProfileInt("Main", "Height", wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top, m_szINIPath); //�E�B���h�E�ʒuHeight
	WritePrivateProfileInt("Main", "Width", wpl.rcNormalPosition.right - wpl.rcNormalPosition.left, m_szINIPath); //�E�B���h�E�ʒuWidth
	ShowWindow(hWnd, SW_HIDE);	// �I�������������Č����邽�߂ɔ�\��
	WritePrivateProfileInt("Main", "Mode", m_nPlayMode, m_szINIPath); //�v���C���[�h
	WritePrivateProfileInt("Main", "Repeat", m_nRepeatFlag, m_szINIPath); //���s�[�g���[�h
	WritePrivateProfileInt("Main", "Volumes", (int)SendMessage(m_hVolume, TBM_GETPOS, 0, 0), m_szINIPath); //�{�����[��
	WritePrivateProfileInt("Main", "Tray", g_cfg.nTrayOpt, m_szINIPath); //�^�X�N�g���C���[�h
	WritePrivateProfileInt("Main", "Info", g_cfg.nInfoTip, m_szINIPath); //�Ȗ����m�点
	WritePrivateProfileInt("Main", "PathTip", g_cfg.nPathTip, m_szINIPath); // �c�[���`�b�v�Ńt���p�X��\��
	WritePrivateProfileInt("Main", "Priority", g_cfg.nHighTask, m_szINIPath); //�V�X�e���D��x
	WritePrivateProfileInt("Main", "TreeWidth", g_cfg.nTreeWidth, m_szINIPath); //�c���[�̕�
	WritePrivateProfileInt("Main", "TreeState", (g_cfg.nTreeState==TREE_SHOW), m_szINIPath);
	WritePrivateProfileInt("Main", "ShowStatus", g_cfg.nShowStatus, m_szINIPath);
	WritePrivateProfileInt("Main", "TabIndex", (m_nPlayTab==-1?TabCtrl_GetCurSel(m_hTab):m_nPlayTab), m_szINIPath);	//Tab��Index
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
	WritePrivateProfileInt("MiniPanel", "x", g_cfg.nMiniPanel_x, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "y", g_cfg.nMiniPanel_y, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "End", g_cfg.nMiniPanelEnd, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "TopMost", g_cfg.nMiniTop, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "Width", g_cfg.nMiniWidth, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "Scroll", g_cfg.nMiniScroll, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "TimeShow", g_cfg.nMiniTimeShow, m_szINIPath);
	WritePrivateProfileInt("MiniPanel", "ToolShow", g_cfg.nMiniToolShow, m_szINIPath);
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
	WritePrivateProfileString("Main", "LastPath", szLastPath, m_szINIPath); //���X�g�p�X
	WritePrivateProfileString("Main", "StartPath", g_cfg.szStartPath, m_szINIPath);
	WritePrivateProfileString("Main", "FilerPath", g_cfg.szFilerPath, m_szINIPath);
	WritePrivateProfileString("Main", "LastFile", g_cfg.szLastFile, m_szINIPath);
	WritePrivateProfileInt("BookMark", "BMRoot", g_cfg.nBMRoot, m_szINIPath);
	WritePrivateProfileInt("BookMark", "BMFullPath", g_cfg.nBMFullPath, m_szINIPath);
	SaveBookMark(m_szINIPath);	// ������̕ۑ�
	WritePrivateProfileString("Font", "FontName", g_cfg.szFontName, m_szINIPath);
	// �z�b�g�L�[��ۑ�
	for(i=0;i<HOTKEY_COUNT;i++){
		wsprintf(szSec, "HotKey%d", i);
		WritePrivateProfileInt("HotKey", szSec, g_cfg.nHotKey[i], m_szINIPath); //�z�b�g�L�[
	}
	// �^�X�N�g���C��ۑ�
	for(i=0;i<6;i++){
		wsprintf(szSec, "Click%d", i);
		WritePrivateProfileInt("TaskTray", szSec, g_cfg.nTrayClick[i], m_szINIPath); //�z�b�g�L�[
	}
	//�@���o�[�̏�Ԃ�ۑ�
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
	// �O���c�[���ۑ�
	WritePrivateProfileString("Tool", "Path0", g_cfg.szToolPath, m_szINIPath);
	return;
}

// �E�B���h�E�̕\��/��\�����g�O��
static void ToggleWindowView(HWND hWnd){
	if(IsIconic(hWnd) || !IsWindowVisible(hWnd)){
		if(m_bTrayFlag && g_cfg.nTrayOpt==1){
			Shell_NotifyIcon(NIM_DELETE, &m_ni);
			m_bTrayFlag = FALSE;
		}
		SendMessage(m_hTitleTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, NULL);
		KillTimer(hWnd, ID_TIPTIMER);
		ShowWindow(hWnd, SW_SHOW);
		ShowWindow(hWnd, SW_RESTORE);
		//SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
		//SetForegroundWindow(hWnd);
	}else{
		ShowWindow(hWnd, SW_MINIMIZE);
	}
	return;
}

// DLL�S�������A�Ή��`������
static int InitFileTypes(){
	int i = 0;
	char szPath[MAX_PATH] = {0};
	HANDLE hFind = NULL;
	WIN32_FIND_DATA wfd;

	ZeroMemory(g_cfg.szTypeList, 5*MAX_EXT_COUNT*(sizeof(char)));
	lstrcpy(g_cfg.szTypeList[i++], "mp3");
	lstrcpy(g_cfg.szTypeList[i++], "mp2");
	lstrcpy(g_cfg.szTypeList[i++], "mp1");
	lstrcpy(g_cfg.szTypeList[i++], "wav");
	lstrcpy(g_cfg.szTypeList[i++], "ogg");
	lstrcpy(g_cfg.szTypeList[i++], "aif");

	GetModuleFileName(NULL, szPath, MAX_PATH);
	lstrcpyn(PathFindFileName(szPath), _T("bass*.dll"), MAX_PATH);

	hFind = FindFirstFile(szPath, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			HPLUGIN hPlugin = BASS_PluginLoad(wfd.cFileName, 0);
			if(hPlugin){
				const BASS_PLUGININFO *info = BASS_PluginGetInfo(hPlugin);
				for(DWORD d=0;d<info->formatc;d++){
					char szTemp[100] = {0};
					lstrcpyn(szTemp, info->formats[d].exts, 100);
					char *q = szTemp;
					while(*q){
						if(char *p = StrStr(q, ";")){
							*p = '\0';
							lstrcpy(g_cfg.szTypeList[i++], q + 2);
							q = p + 1;
						}else{
							lstrcpy(g_cfg.szTypeList[i++], q + 2);
							break;
						}
					}
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	return i;
}

// �t�@�C�����I�[�v���A�\���̂�ݒ�
static BOOL SetChannelInfo(BOOL bFlag, struct FILEINFO *pInfo){
	char szFilePath[MAX_FITTLE_PATH];
	QWORD qStart, qEnd;
	char szStart[100], szEnd[100];

	lstrcpyn(g_cInfo[bFlag].szFilePath, pInfo->szFilePath, MAX_FITTLE_PATH);
	g_cInfo[bFlag].pBuff = 0;
	g_cInfo[bFlag].qStart = 0;

	if(IsArchivePath(pInfo->szFilePath)){
		AnalyzeArchivePath(&g_cInfo[bFlag], szFilePath);
	}else if(IsCueSheetPath(pInfo->szFilePath)){
		GetCueSheetRealPath(pInfo->szFilePath, szFilePath, szStart, szEnd);
		g_cInfo[bFlag].qDuration = 0;
	}else{
		lstrcpyn(szFilePath, pInfo->szFilePath, MAX_FITTLE_PATH);
		g_cInfo[bFlag].qDuration = 0;
	}

	g_cInfo[bFlag].hChan = BASS_StreamCreateFile((BOOL)g_cInfo[bFlag].pBuff,
												(g_cInfo[bFlag].pBuff?(void *)g_cInfo[bFlag].pBuff:(void *)szFilePath),
												0, (DWORD)g_cInfo[bFlag].qDuration,
												BASS_STREAM_DECODE | m_bFLoat*BASS_SAMPLE_FLOAT);
	if(g_cInfo[bFlag].hChan){
		if(IsCueSheetPath(pInfo->szFilePath)){
			qStart = GetByteFromSecStr(g_cInfo[bFlag].hChan, szStart);
			qEnd = GetByteFromSecStr(g_cInfo[bFlag].hChan, szEnd);
			BASS_ChannelSetPosition(g_cInfo[bFlag].hChan, qStart, BASS_POS_BYTE);
			g_cInfo[bFlag].qStart = qStart;
			g_cInfo[bFlag].qDuration = qEnd - qStart;
			BASS_ChannelSetSync(g_cInfo[bFlag].hChan, BASS_SYNC_POS, qStart + (g_cInfo[bFlag].qDuration)*99/100, &EventSync, 0);
			BASS_ChannelSetSync(g_cInfo[bFlag].hChan, BASS_SYNC_POS, qEnd, &EventSync, (void *)1);
		}else{
			g_cInfo[bFlag].qDuration = BASS_ChannelGetLength(g_cInfo[bFlag].hChan, BASS_POS_BYTE);
			// �Ȃ�99%�Đ���A�C�x���g���N����悤�ɁB
			if(lstrcmpi(PathFindExtension(szFilePath), ".CDA")){
				BASS_ChannelSetSync(g_cInfo[bFlag].hChan, BASS_SYNC_POS, (g_cInfo[bFlag].qDuration)*99/100, &EventSync, 0);
			}
		}
		return TRUE;
	}else{
		BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 60000); // 5sec�͒Z���̂�60sec�ɂ���
		g_cInfo[bFlag].hChan = BASS_StreamCreateURL(szFilePath, 0, BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT * m_bFLoat, NULL, 0);
		if(g_cInfo[bFlag].hChan){
			g_cInfo[bFlag].qDuration = BASS_ChannelGetLength(g_cInfo[bFlag].hChan, BASS_POS_BYTE);
			if(g_cInfo[bFlag].qDuration<0){
				g_cInfo[bFlag].qDuration = 0;
			}
			BASS_ChannelSetSync(g_cInfo[bFlag].hChan, BASS_SYNC_END, 0, &EventSync, 0);
			return TRUE;
		}
		return FALSE;
	}
}

static void FreeChannelInfo(BOOL bFlag){
	BASS_ChannelStop(g_cInfo[bFlag].hChan);
	BASS_StreamFree(g_cInfo[bFlag].hChan);	// BASS_STREAM_AUTOFREE�ɂ���΂���Ȃ�����

	if(g_cInfo[bFlag].pBuff){
		HeapFree(GetProcessHeap(), 0, (LPVOID)g_cInfo[bFlag].pBuff);
	}
	g_cInfo[bFlag].pBuff = NULL;
	g_cInfo[bFlag].hChan = NULL;
	return;
}

// ���C���X�g���[���v���V�[�W��
static DWORD CALLBACK MainStreamProc(HSTREAM /*handle*/, void *buf, DWORD len, void * /*user*/){
	DWORD r;

	if(BASS_ChannelIsActive(g_cInfo[g_bNow].hChan)){
		r = BASS_ChannelGetData(g_cInfo[g_bNow].hChan, buf, len);
		if(m_bCueEnd || !BASS_ChannelIsActive(g_cInfo[g_bNow].hChan)){
			m_bCueEnd = FALSE;

			PostMessage(GetParent(m_hStatus), WM_COMMAND, MAKEWPARAM(IDM_PLAY_NEXT, 0), 0);
		}
	}else{
		r = BASS_STREAMPROC_END;
	}
	return r;
}

// ���C���X�g���[�����쐬
static HSTREAM CreateMainStream(BASS_CHANNELINFO *info){
	HSTREAM hRet;
	hRet = BASS_StreamCreate(info->freq, info->chans, info->flags & ~BASS_STREAM_DECODE, &MainStreamProc, 0);

	BASS_ChannelSetAttribute(hRet, BASS_ATTRIB_VOL, SendMessage(m_hVolume, TBM_GETPOS, 0, 0) / (float)SLIDER_DIVIDED);
	BASS_ChannelPlay(hRet, FALSE);

	// WINAMP DSP�v���O�C����L���ɂ���
	SetBassWaDsp(hRet);

	return hRet;
}

// �w��t�@�C�����Đ�
static BOOL PlayByUser(HWND hWnd, struct FILEINFO *pPlayFile){
	BASS_CHANNELINFO info;
//	DWORD d;

	// �Đ��Ȃ��~�A�J��
	StopOutputStream(hWnd);
	if(SetChannelInfo(g_bNow, pPlayFile)){
		// �X�g���[���쐬
		BASS_ChannelGetInfo(g_cInfo[g_bNow].hChan, &info);
		m_hChanOut = CreateMainStream(&info);

		OnStatusChangePlugins();

		// �V�[�N�o�[�p�^�C�}�[�쐬
		SetTimer(hWnd, ID_SEEKTIMER, 500, NULL);

		// �\���ؑ�
		m_pNext = pPlayFile;
		m_nGaplessState = GS_OK;
		OnChangeTrack();

		return TRUE;
	}else{
		SetWindowText(hWnd, "�t�@�C���̃I�[�v���Ɏ��s���܂����I");
		return FALSE;
	}
}

// ���̃t�@�C�����I�[�v������֐�(99���n�_�Ŕ���)
static void CALLBACK EventSync(HSYNC /*handle*/, DWORD /*channel*/, DWORD /*data*/, void *user){
	if(user==0){
		PostMessage(GetParent(m_hStatus), WM_COMMAND, MAKEWPARAM(IDM_SETUP_NEXT, 0), 0);
	}else{
		m_bCueEnd = TRUE;
	}
	return;
}

// �\���ؑ�
static void OnChangeTrack(){
	int nPlayIndex;
	char szShortTag[64] = {0};
	char szTitleCap[524] = {0};
	BASS_CHANNELINFO info;

	// 99���܂ł����Ȃ������ꍇ
	if(m_nGaplessState==GS_NOEND){
		m_pNext = SelectNextFile(TRUE);
		if(m_pNext){
			SetChannelInfo(g_bNow, m_pNext);
		}else{
			StopOutputStream(GetParent(m_hStatus));
			return;
		}
	}

	// ���s�[�g�I��
	if(!m_pNext){
		StopOutputStream(GetParent(m_hStatus));
		return;
	}

	//���g�����ς��Ƃ��͊J���Ȃ���
	BASS_ChannelGetInfo(g_cInfo[g_bNow].hChan, &info);
	if(m_nGaplessState==GS_NEWFREQ){
		BASS_ChannelStop(m_hChanOut);
		m_hChanOut = CreateMainStream(&info);
	}

	// �؂�ւ�����t�@�C���̃C���f�b�N�X���擾
	nPlayIndex = GetIndexFromPtr(GetPlayListTab(m_hTab)->pRoot, m_pNext);

	// �\����؂�ւ�
	ListView_SingleSelect(GetPlayListTab(m_hTab)->hList, nPlayIndex);
	InvalidateRect(GetCurListTab(m_hTab)->hList, NULL, TRUE); // CUSTOMDRAW�C�x���g����

	// �t�@�C���̃I�[�v���Ɏ��s����
	if(m_nGaplessState==GS_FAILED){
		StopOutputStream(GetParent(m_hStatus));
		SetWindowText(GetParent(m_hStatus), "�t�@�C���̃I�[�v���Ɏ��s���܂����I");
		return;
	}

	// �Đ��ς݂ɂ���
	m_pNext->bPlayFlag = TRUE;
	lstrcpyn(g_cfg.szLastFile, m_pNext->szFilePath, MAX_FITTLE_PATH);

	// �X�e�[�^�X�N���A
	m_nGaplessState = GS_NOEND;

	// ���ݍĐ��ȂƈႤ�ȂȂ�Đ������ɒǉ�����
	if(GetPlaying(GetPlayListTab(m_hTab))!=m_pNext)
		PushPlaying(GetPlayListTab(m_hTab), m_pNext);
	
	// �J�����g�t�@�C����ύX
	GetPlayListTab(m_hTab)->nPlaying = nPlayIndex;


	// �^�O��
	if(GetCueSheetTagInfo(g_cInfo[g_bNow].szFilePath, &m_taginfo)
	|| BASS_TAG_Read(g_cInfo[g_bNow].hChan, &m_taginfo)){
		if(!g_cfg.nTagReverse){
			wsprintf(m_szTag, "%s / %s", m_taginfo.szTitle, m_taginfo.szArtist);
		}else{
			wsprintf(m_szTag, "%s / %s", m_taginfo.szArtist, m_taginfo.szTitle);
		}
	}else{
		lstrcpyn(m_szTag, GetFileName(g_cInfo[g_bNow].szFilePath), MAX_FITTLE_PATH);
		lstrcpyn(m_taginfo.szTitle, m_szTag, 256);
	}

	//�^�C�g���o�[�̏���
	wsprintf(szTitleCap, "%s - %s", m_szTag, FITTLE_VERSION);
	SetWindowText(GetParent(m_hStatus), szTitleCap);

	float time = BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, BASS_ChannelGetLength(g_cInfo[g_bNow].hChan, BASS_POS_BYTE)); // playback duration
	DWORD dLen = BASS_StreamGetFilePosition(g_cInfo[g_bNow].hChan, BASS_FILEPOS_END); // file length
	DWORD bitrate;
	if(dLen>0 && time>0){
		bitrate = (DWORD)(dLen/(125*time)+0.5); // bitrate (Kbps)
		wsprintf(szTitleCap, "%d Kbps  %d Hz  %s", bitrate, info.freq, (info.chans!=1?"Stereo":"Mono"));
	}else{
		wsprintf(szTitleCap, "? Kbps  %d Hz  %s", info.freq, (info.chans!=1?"Stereo":"Mono"));
	}

	//�X�e�[�^�X�o�[�̏���
	if(g_cfg.nTreeIcon){
		SetStatusbarIcon(g_cInfo[g_bNow].szFilePath, TRUE);
	}
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)0|0, (LPARAM)szTitleCap);
	SendMessage(m_hStatus, SB_SETTIPTEXT, (WPARAM)0, (LPARAM)szTitleCap);

	wsprintf(szTitleCap, "\t%d / %d", GetPlayListTab(m_hTab)->nPlaying + 1, ListView_GetItemCount(GetPlayListTab(m_hTab)->hList));
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)1|0, (LPARAM)szTitleCap);

	//�V�[�N�o�[
	if(g_cInfo[g_bNow].qDuration>0){
		EnableWindow(m_hSeek, TRUE);
	}else{
		EnableWindow(m_hSeek, FALSE);
	}

	//�^�X�N�g���C�̏���
	lstrcpyn(szShortTag, m_szTag, 54);
	szShortTag[52] = '.';
	szShortTag[53] = '.';
	szShortTag[54] = '.';
	szShortTag[55] = '\0';
	wsprintf(m_ni.szTip, "%s - Fittle", szShortTag);

	if(m_bTrayFlag)
		Shell_NotifyIcon(NIM_MODIFY, &m_ni); //ToolTip�̕ύX
	if(g_cfg.nInfoTip == 2 || (g_cfg.nInfoTip == 1 && m_bTrayFlag))
		ShowToolTip(GetParent(m_hStatus), m_hTitleTip, m_ni.szTip);

	OnTrackChagePlugins();

	m_pNext = NULL;	// ����
	return;
}

// ���ɍĐ����ׂ��t�@�C���̃|�C���^�擾
static struct FILEINFO *SelectNextFile(BOOL bTimer){
	int nLBCount;
	int nPlayIndex;
	int nTmpIndex;

	nLBCount = ListView_GetItemCount(GetPlayListTab(m_hTab)->hList);
	if(nLBCount<=0){
		return NULL;
	}
	nPlayIndex = GetPlayListTab(m_hTab)->nPlaying;
	switch (m_nPlayMode)
	{
		case PM_LIST:
			nPlayIndex++;
			if(nPlayIndex==nLBCount){
				if(m_nRepeatFlag || !bTimer){
					nPlayIndex = 0;
				}else{
					return NULL;	// ���s�[�g���I�t
				}
			}					
			break;

		case PM_RANDOM:
			int nUnPlayCount;
			int nBefore;

			if(nLBCount==1){
				// ���X�g�Ƀt�@�C����������Ȃ�
				nPlayIndex = 0;
				if(!m_nRepeatFlag && bTimer && GetUnPlayedFile(GetPlayListTab(m_hTab)->pRoot)==0){
					SetUnPlayed(GetPlayListTab(m_hTab)->pRoot);
					return NULL;
				}
			}else{
				nUnPlayCount = GetUnPlayedFile(GetPlayListTab(m_hTab)->pRoot);
				if(nUnPlayCount==0){
					if(m_nRepeatFlag || !bTimer){
						// ���ׂčĐ����Ă����ꍇ
						nBefore = GetPlayListTab(m_hTab)->nPlaying;
						nUnPlayCount = SetUnPlayed(GetPlayListTab(m_hTab)->pRoot);
						do{
							nTmpIndex = genrand_int32() % nUnPlayCount;
						}while(nTmpIndex==nBefore);
						nPlayIndex = GetUnPlayedIndex(GetPlayListTab(m_hTab)->pRoot, nTmpIndex);
					}else{
						// ���s�[�g���I�t
						SetUnPlayed(GetPlayListTab(m_hTab)->pRoot);
						return NULL;
					}
				}else{
					// �Đ��r��
					nTmpIndex = genrand_int32() % nUnPlayCount;
					nPlayIndex = GetUnPlayedIndex(GetPlayListTab(m_hTab)->pRoot, nTmpIndex);
				}
			}
			break;

		case PM_SINGLE:
			// lp=1�̂Ƃ��̓^�C�}�[����B����ȊO�̓��[�U�[����
			if(bTimer){
				if(!m_nRepeatFlag){
					return NULL;
				}
				if(GetPlayListTab(m_hTab)->nPlaying==-1)
					nPlayIndex = 0;
			}else{
				nPlayIndex++;
				if(GetPlayListTab(m_hTab)->nPlaying==--nLBCount)
					nPlayIndex = 0;
			}
			break;

	}
	return GetPtrFromIndex(GetPlayListTab(m_hTab)->pRoot, nPlayIndex);
}


#define TB_BTN_NUM 8
#define TB_BMP_NUM 9
// �c�[���o�[�̍쐬
static HWND CreateToolBar(HWND hRebar){
	HIMAGELIST hToolImage;
	HBITMAP hBmp;
	TBBUTTON tbb[] = {
		{0, IDM_PLAY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{1, IDM_PAUSE, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
		{2, IDM_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{3, IDM_PREV, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{4, IDM_NEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
		{5, IDM_PM_TOGGLE, TBSTATE_ENABLED, TBSTYLE_DROPDOWN, 0L, 0},
		{6, IDM_PM_REPEAT, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
		//{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	};
	HWND hToolBar = NULL;

	hToolBar = CreateWindowEx(WS_EX_TOOLWINDOW,
                    TOOLBARCLASSNAME,
                    NULL,
                    WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_TOOLTIPS,// | WS_CLIPSIBLINGS,// | WS_CLIPCHILDREN, 
                    0, 0, 100, 100,
                    hRebar,
                    (HMENU)ID_TOOLBAR, m_hInst, NULL);
	if(!hToolBar) return NULL;
	SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	hBmp = (HBITMAP)LoadImage(m_hInst, "toolbarex.bmp", IMAGE_BITMAP, 16*TB_BMP_NUM, 16, LR_LOADFROMFILE | LR_SHARED);
	if(hBmp==NULL)	hBmp = LoadBitmap(m_hInst, (char *)IDR_TOOLBAR1);
	
	//SendMessage(hToolBar, TB_AUTOSIZE, 0, 0) ;
	
	hToolImage = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, TB_BMP_NUM, 0);
	ImageList_AddMasked(hToolImage, hBmp, RGB(0,255,0)); //�΂�w�i�F��

	DeleteObject(hBmp);
	SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)hToolImage);
	
	SendMessage(hToolBar, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
	SendMessage(hToolBar, TB_ADDBUTTONS, (WPARAM)TB_BTN_NUM, (LPARAM)&tbb);
	return hToolBar;
}

// �番���Ńt�@�C�����V�[�N
static BOOL _BASS_ChannelSetPosition(DWORD handle, int nPos){
	QWORD qPos;
	BOOL bRet;

	DWORD dwMode = BASS_ChannelIsActive(m_hChanOut);

	qPos = g_cInfo[g_bNow].qDuration;
	qPos = qPos*nPos/1000 + g_cInfo[g_bNow].qStart;
	// Fade-out
	if(g_cfg.nFadeOut){
		BASS_ChannelSlideAttribute(m_hChanOut, BASS_ATTRIB_VOL, 0.0f, 150);
		while (BASS_ChannelIsSliding(m_hChanOut, BASS_ATTRIB_VOL)) Sleep(1);
	}
	BASS_ChannelStop(m_hChanOut);

	bRet = BASS_ChannelSetPosition(handle, qPos, BASS_POS_BYTE);

	if(g_cfg.nFadeOut){
		BASS_ChannelSetAttribute(m_hChanOut, BASS_ATTRIB_VOL, SendMessage(m_hVolume, TBM_GETPOS, 0, 0) / (float)SLIDER_DIVIDED);
	}
	BASS_ChannelPlay(m_hChanOut, FALSE);
	if(dwMode != BASS_ChannelIsActive(m_hChanOut)){
		OnStatusChangePlugins();
	}

	SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	return bRet;
}

static void _BASS_ChannelSeekSecond(DWORD handle, float fSecond, int nSign){
	QWORD qPos;
	QWORD qSeek;
	QWORD qSet;

	qPos = BASS_ChannelGetPosition(handle, BASS_POS_BYTE) - g_cInfo[g_bNow].qStart;
	qSeek = BASS_ChannelSeconds2Bytes(handle, fSecond);
	if(nSign<0 && qPos<qSeek){
		qSet = 0;
	}else if(nSign>0 && qPos+qSeek>g_cInfo[g_bNow].qDuration){
		qSet = g_cInfo[g_bNow].qDuration-1;
	}else{
		qSet = qPos + nSign*qSeek;
	}
	BASS_ChannelStop(m_hChanOut);
	BASS_ChannelSetPosition(handle, qSet + g_cInfo[g_bNow].qStart, BASS_POS_BYTE);
	BASS_ChannelPlay(m_hChanOut, FALSE);

	return;
}

// �Đ����Ȃ�ꎞ��~�A�ꎞ��~���Ȃ畜�A
static int FilePause(){
	DWORD dMode;

	dMode = BASS_ChannelIsActive(m_hChanOut);
	if(dMode==BASS_ACTIVE_PLAYING){
		// Fade-out
		if(g_cfg.nFadeOut){
			BASS_ChannelSlideAttribute(m_hChanOut, BASS_ATTRIB_VOL, 0.0f, 250);
			while (BASS_ChannelIsSliding(m_hChanOut, BASS_ATTRIB_VOL)) Sleep(1);
		}
		BASS_ChannelPause(m_hChanOut);
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(TRUE, 0));
	}else if(dMode==BASS_ACTIVE_PAUSED){
		BASS_ChannelPlay(m_hChanOut, FALSE);
		// Fade-in
		if(g_cfg.nFadeOut){
			BASS_ChannelSlideAttribute(m_hChanOut, BASS_ATTRIB_VOL, SendMessage(m_hVolume, TBM_GETPOS, 0, 0) / (float)SLIDER_DIVIDED, 250);
			while (BASS_ChannelIsSliding(m_hChanOut, BASS_ATTRIB_VOL)) Sleep(1);
		}
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	}else{
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	}

	// �v���O�C�����Ă�
	OnStatusChangePlugins();

	return 0;
}

// �A�E�g�v�b�g�X�g���[�����~�A�\����������
static int StopOutputStream(HWND hWnd){
	// Fade-out
	if(g_cfg.nFadeOut){
		BASS_ChannelSlideAttribute(m_hChanOut, BASS_ATTRIB_VOL, 0.0f, 250);
		while (BASS_ChannelIsSliding(m_hChanOut, BASS_ATTRIB_VOL)) Sleep(1);
	}

	// �X�g���[�����폜
	KillTimer(hWnd, ID_SEEKTIMER);
	BASS_ChannelStop(m_hChanOut);
	BASS_StreamFree(m_hChanOut);

	if(g_cInfo[!g_bNow].hChan){
		FreeChannelInfo(!g_bNow);
	}
	if(g_cInfo[g_bNow].hChan){
		FreeChannelInfo(g_bNow);
	}

	//�X���C�_�o�[
	EnableWindow(m_hSeek, FALSE);
	SendMessage(m_hSeek, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);
	//�c�[���o�[
	SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	
	//������\���֌W
	SetWindowText(hWnd, FITTLE_VERSION);
	lstrcpy(m_ni.szTip, FITTLE_VERSION);
	if(m_bTrayFlag)
		Shell_NotifyIcon(NIM_MODIFY, &m_ni); //ToolTip�̕ύX

	SendMessage(hWnd, WM_TIMER, MAKEWPARAM(ID_TIPTIMER, 0), 0);

	lstrcpy(m_szTag, "");
	lstrcpy(m_szTime, "\t");

	// �X�e�[�^�X�o�[�̏�����
	SetStatusbarIcon(NULL, FALSE);
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)0|0, (LPARAM)"");
	SendMessage(m_hStatus, SB_SETTIPTEXT, (WPARAM)0, (LPARAM)"");
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)1|0, (LPARAM)"");
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)2|0, (LPARAM)"");
	// ���X�g�r���[���ĕ`��
	InvalidateRect(GetCurListTab(m_hTab)->hList, NULL, TRUE); 

	// �v���O�C�����Ă�
	OnStatusChangePlugins();

	return 0;
}

// �v���C���[�h�𐧌䂷��֐�
static int ControlPlayMode(HMENU hMenu, int nMode){
	MENUITEMINFO mii;
	int i = 0;

	if(nMode>PM_SINGLE) nMode = PM_LIST;
	m_nPlayMode = nMode;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	if(i++==nMode){
		mii.fState = MFS_CHECKED;
		SendMessage(m_hToolBar,  TB_CHANGEBITMAP, (WPARAM)IDM_PM_TOGGLE, (LPARAM)MAKELONG(5, 0));
	}
	SetMenuItemInfo(hMenu, IDM_PM_LIST, FALSE, &mii);
	SetMenuItemInfo(m_hTrayMenu, IDM_PM_LIST, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	if(i++==nMode){
		mii.fState = MFS_CHECKED;
		SendMessage(m_hToolBar,  TB_CHANGEBITMAP, (WPARAM)IDM_PM_TOGGLE, (LPARAM)MAKELONG(7, 0));
	}
	SetMenuItemInfo(hMenu, IDM_PM_RANDOM, FALSE, &mii);
	SetMenuItemInfo(m_hTrayMenu, IDM_PM_RANDOM, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	if(i++==nMode){
		mii.fState = MFS_CHECKED;
		SendMessage(m_hToolBar,  TB_CHANGEBITMAP, (WPARAM)IDM_PM_TOGGLE, (LPARAM)MAKELONG(8, 0));
	}
	SetMenuItemInfo(hMenu, IDM_PM_SINGLE, FALSE, &mii);
	SetMenuItemInfo(m_hTrayMenu, IDM_PM_SINGLE, FALSE, &mii);
	InvalidateRect(m_hToolBar, NULL, TRUE);
	return 0;
}

// ���s�[�g���[�h��؂�ւ�
static BOOL ToggleRepeatMode(HMENU hMenu){
	MENUITEMINFO mii;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	if(m_nRepeatFlag){
		m_nRepeatFlag = FALSE;
		mii.fState = MFS_UNCHECKED;
		SendMessage(m_hToolBar, TB_CHECKBUTTON, (WPARAM)IDM_PM_REPEAT, (LPARAM)MAKELONG(FALSE, 0));
	}else{
		m_nRepeatFlag = TRUE;
		mii.fState = MFS_CHECKED;
		SendMessage(m_hToolBar, TB_CHECKBUTTON, (WPARAM)IDM_PM_REPEAT, (LPARAM)MAKELONG(TRUE, 0));
	}
	SetMenuItemInfo(hMenu, IDM_PM_REPEAT, FALSE, &mii);
	SetMenuItemInfo(m_hTrayMenu, IDM_PM_REPEAT, FALSE, &mii);
	return TRUE;
}

// �V�X�e���g���C�ɓ����֐�
static int SetTaskTray(HWND hWnd){
	static HICON hIcon = NULL;

	if(!hIcon){
		hIcon = (HICON)LoadImage(m_hInst, "MYICON", IMAGE_ICON, 16, 16, LR_SHARED);
	}

	m_bTrayFlag = TRUE;
	m_ni.cbSize = sizeof(m_ni);
	m_ni.hWnd = hWnd;
	m_ni.uID = 1;
	m_ni.uCallbackMessage = WM_TRAY;
	m_ni.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_ni.hIcon = hIcon;
	Shell_NotifyIcon(NIM_ADD, &m_ni);
	return 0;
}

// �c�[���`�b�v��\������֐�
static int ShowToolTip(HWND hWnd, HWND hTitleTip, char *pszToolText){
	TOOLINFO tin;
	RECT rcTask, rcTip;
	HWND hTaskBar;
	POINT ptDraw;

	//�^�X�N�o�[�̈ʒu�A�������擾
	hTaskBar = FindWindow("Shell_TrayWnd", NULL);
	if(hTaskBar==NULL) return 0;
	GetWindowRect(hTaskBar, &rcTask);
	//�c�[���`�b�v�����A�\��
	ZeroMemory(&tin, sizeof(tin));
	tin.cbSize = sizeof(tin);
	tin.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	tin.hwnd = NULL;//hWnd;
	tin.lpszText = pszToolText;
	SendMessage(hTitleTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&tin);
	SendMessage(hTitleTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&tin);
	SendMessage(hTitleTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&tin);
	//�f�X�N�g�b�v��Ɨ̈�̋��Ɉړ�
	GetWindowRect(hTitleTip, &rcTip);
	ptDraw.x = rcTask.right - (rcTip.right - rcTip.left) - 1;
	// �^�X�N�o�[�̈ʒu
	if(rcTask.top==0){
		if((rcTask.right-rcTask.left)>=(rcTask.bottom-rcTask.top)){
			ptDraw.y = rcTask.bottom - 2; // ��ɂ���
		}else{
			ptDraw.y = rcTask.bottom - (rcTip.bottom - rcTip.top);
			if(rcTask.left==0){	// ���ɂ���
				ptDraw.x = 0;
			}
		}
	}else{
		ptDraw.y = rcTask.top - (rcTip.bottom - rcTip.top) + 2; // ���ɂ���
	}
	SendMessage(hTitleTip, TTM_TRACKPOSITION, 0, (LPARAM)(DWORD)MAKELONG(ptDraw.x, ptDraw.y));
	//��O�ɕ\��
	SetWindowPos(hTitleTip, HWND_TOPMOST, 0, 0, 0, 0,
		/*SWP_SHOWWINDOW | */SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	//6�b������܂�
	SetTimer(hWnd, ID_TIPTIMER, 6000, NULL);
	return 0;
}

static int RegHotKey(HWND hWnd){
	UINT uMod;
	int i;

	for(i=0;i<HOTKEY_COUNT;i++){
		uMod = 0;
		if(HIBYTE(g_cfg.nHotKey[i]) & HOTKEYF_ALT)
			uMod = MOD_ALT;
		if(HIBYTE(g_cfg.nHotKey[i]) & HOTKEYF_SHIFT)
			uMod = uMod | MOD_SHIFT;
		if(HIBYTE(g_cfg.nHotKey[i]) & HOTKEYF_CONTROL)
			uMod = uMod | MOD_CONTROL;
		RegisterHotKey(hWnd, i, uMod, LOBYTE(g_cfg.nHotKey[i]));
	}
	return 0;
}

static int UnRegHotKey(HWND hWnd){
	int i;
	for(i=0;i<HOTKEY_COUNT;i++) UnregisterHotKey(hWnd, i);
	return 0;
}

// �Ή��t�@�C�����ǂ����g���q�Ń`�F�b�N
BOOL CheckFileType(char *szFilePath){
	int i=0;
	char *szCheckType;

	szCheckType = PathFindExtension(szFilePath);
	if(!szCheckType || !*szCheckType) return FALSE;
	szCheckType++;
	while(g_cfg.szTypeList[i][0]!='\0' && i<MAX_EXT_COUNT){
		if(lstrcmpi(g_cfg.szTypeList[i], szCheckType)==0) return TRUE;
		i++;
	}
	return FALSE;
}

static BOOL OpenFilerPath(char *szPath, HWND hWnd, LPCSTR pszMsg){
	OPENFILENAME ofn;
	char szFile[MAX_FITTLE_PATH] = {0};
	char szFileTitle[MAX_FITTLE_PATH] = {0};

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "���s�t�@�C��(*.exe)\0*.exe\0\0";
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

// �R�}���h���C���I�v�V�������l������Execute
static void MyShellExecute(HWND hWnd, LPSTR pszExePath, LPSTR pszFilePathes, BOOL bMulti){
	LPSTR pszArgs = PathGetArgs(pszExePath);

	int nSize = lstrlen(pszArgs) + lstrlen(pszFilePathes) + 5;

	LPSTR pszBuff = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TCHAR) * nSize);

	if(!bMulti){
		PathQuoteSpaces(pszFilePathes);
	}
	if(*pszArgs){	// �R�}���h���C���I�v�V�������l��
		*(pszArgs-1) = '\0';
		wsprintf(pszBuff, "%s %s", pszArgs, pszFilePathes);
	}else{
		wsprintf(pszBuff, "%s", pszFilePathes);
	}
	if(!bMulti){
		PathUnquoteSpaces(pszFilePathes);
	}
	ShellExecute(hWnd, "open", pszExePath, pszBuff, NULL, SW_SHOWNORMAL);
	if(*pszArgs){	// �߂���
		*(pszArgs-1) = ' ';
	}

	HeapFree(GetProcessHeap(), 0, pszBuff);
	return;
}

// ���X�g�őI�����ꂽ�A�C�e���̃p�X��A�����ĕԂ��B�ǂ�����Free���Ă��������B
static LPSTR MallocAndConcatPath(LISTTAB *pListTab){
	LPSTR pszRet = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TCHAR) * 2);

	int i = -1;
	while( (i = ListView_GetNextItem(pListTab->hList, i, LVNI_SELECTED)) != -1 ){
		LPSTR pszTarget = GetPtrFromIndex(pListTab->pRoot, i)->szFilePath;
		if(!IsURLPath(pszTarget) && !IsArchivePath(pszTarget) && !IsCueSheetPath(pszTarget)){	// �폜�ł��Ȃ��t�@�C���łȂ����
			int nNewSize = HeapSize(GetProcessHeap(), 0, pszRet) + (lstrlen(pszTarget) + 5) * sizeof(TCHAR);
			pszRet = (LPSTR)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pszRet, nNewSize);
			lstrcat(pszRet, "\"");
			lstrcat(pszRet, pszTarget);
			lstrcat(pszRet, "\" ");
		}
	}
	return pszRet;
}

// �t�@�C����ۑ��_�C�A���O���o��
static int SaveFileDialog(char *szDir, char *szDefTitle){
	char szFile[MAX_FITTLE_PATH];
	char szFileTitle[MAX_FITTLE_PATH];
	OPENFILENAME ofn;

	lstrcpyn(szFile, szDefTitle, MAX_FITTLE_PATH);
	lstrcpyn(szFileTitle, szDefTitle, MAX_FITTLE_PATH);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "�v���C���X�g(��΃p�X) (*.m3u)\0*.m3u\0�v���C���X�g(���΃p�X) (*.m3u)\0*.m3u\0���ׂẴt�@�C��(*.*)\0*.*\0\0";
	ofn.lpstrFile = szFile;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.lpstrInitialDir = szDir;
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = MAX_FITTLE_PATH;
	ofn.nMaxFileTitle = MAX_FITTLE_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = "m3u";
	ofn.lpstrTitle = "���O��t���ĕۑ�����";
	if(GetSaveFileName(&ofn)==0) return 0;
	lstrcpyn(szDir, szFile, MAX_FITTLE_PATH);
	return ofn.nFilterIndex;
}

// �h���b�O�C���[�W�쐬�A�\��
static void OnBeginDragList(HWND hLV, POINT pt){
	m_hDrgImg = ListView_CreateDragImage(hLV, ListView_GetNextItem(hLV, -1, LVNI_SELECTED), &pt);
	ImageList_BeginDrag(m_hDrgImg, 0, 0, 0);
	ImageList_DragEnter(hLV, pt.x, pt.y);
	SetCapture(hLV);
	return;
}

// �h���b�O�C���[�W�𓮂����Ȃ���LVIS_DROPHILITED��\��
static void OnDragList(HWND hLV, POINT pt){
	LVHITTESTINFO pinfo;
	int nHitItem;
	RECT rcLV;

	pinfo.pt.x = pt.x;
	pinfo.pt.y = pt.y;
	ImageList_DragShowNolock(FALSE);
	ListView_SetItemState(hLV, -1, 0, LVIS_DROPHILITED);
	nHitItem = ListView_HitTest(hLV, &pinfo);
	if(nHitItem!=-1)	ListView_SetItemState(hLV, nHitItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
	UpdateWindow(hLV);
	ImageList_DragShowNolock(TRUE);
	ImageList_DragMove(pt.x, pt.y);

	GetClientRect(hLV, &rcLV);
	if(pt.y >=0 && pt.y <= GetSystemMetrics(SM_CYHSCROLL)){
		//��X�N���[��
		SetTimer(GetParent(hLV), ID_SCROLLTIMERUP, 100, NULL);
	}else if(pt.y >= rcLV.bottom - GetSystemMetrics(SM_CYHSCROLL)){
		//���X�N���[��
		SetTimer(GetParent(hLV), ID_SCROLLTIMERDOWN, 100, NULL);
	}else{
		KillTimer(GetParent(hLV), ID_SCROLLTIMERUP);
		KillTimer(GetParent(hLV), ID_SCROLLTIMERDOWN);
	}
}

// �h���b�O�C���[�W�폜
static void OnEndDragList(HWND hLV){
	KillTimer(GetParent(hLV), ID_SCROLLTIMERUP);
	KillTimer(GetParent(hLV), ID_SCROLLTIMERDOWN);
	ListView_SetItemState(hLV, -1, 0, LVIS_DROPHILITED);
	ImageList_DragLeave(hLV);
	ImageList_EndDrag();
	ImageList_Destroy(m_hDrgImg);
	m_hDrgImg = NULL;
	ReleaseCapture();
}

static void DrawTabFocus(HWND m_hTab, int nIdx, BOOL bFlag){
	RECT rcItem;
	HDC hDC;

	if((bFlag && !GetListTab(m_hTab, nIdx)->bFocusRect)
	|| (!bFlag && GetListTab(m_hTab, nIdx)->bFocusRect)){
		TabCtrl_GetItemRect(m_hTab, nIdx, &rcItem);
		rcItem.left += 1;
		rcItem.top += 1;
		rcItem.right -= 1;
		rcItem.bottom -= 1;
		hDC = GetDC(m_hTab);
		DrawFocusRect(hDC, &rcItem);
		GetListTab(m_hTab, nIdx)->bFocusRect = !GetListTab(m_hTab, nIdx)->bFocusRect;
		ReleaseDC(m_hTab, hDC);
	}
}

static BOOL SetUIFont(void){
	int nPitch;
	HDC hDC;

	// �g���Ă����t�H���g���폜
	if(m_hFont) DeleteObject(m_hFont);
	if(m_hBoldFont){
		DeleteObject(m_hBoldFont);
		m_hBoldFont = NULL;
	}
	// �t�H���g�쐬
	if(g_cfg.szFontName[0]!='\0'){
		hDC = GetDC(m_hTree);
		nPitch = -MulDiv(g_cfg.nFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		ReleaseDC(m_hTree, hDC);
		m_hFont = CreateFont(nPitch, 0, 0, 0, (g_cfg.nFontStyle&BOLD_FONTTYPE?FW_BOLD:0),
			(DWORD)(g_cfg.nFontStyle&ITALIC_FONTTYPE?TRUE:FALSE), FALSE, FALSE, (DWORD)DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
			g_cfg.szFontName);
		if(!m_hFont) g_cfg.szFontName[0] = '\0';	// �t�H���g���ǂݍ��߂Ȃ�
	}else{
		m_hFont = NULL;
	}
	// �t�H���g���e�R���g���[���ɓ�����
	SendMessage(m_hCombo, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	if(m_hFont){	// �^�u�͓���
		SendMessage(m_hTab, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	}else{
		SendMessage(m_hTab, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	}
	SendMessage(m_hTree, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	for(int i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
		SendMessage(GetListTab(m_hTab, i)->hList, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	}
	// �R���{�{�b�N�X�̃T�C�Y����
	UpdateWindowSize(GetParent(m_hCombo));
	return TRUE;
}

static void UpdateWindowSize(HWND hWnd){
	RECT rcDisp;

	GetClientRect(hWnd, &rcDisp);
	SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(rcDisp.right, rcDisp.bottom));
	return;
}

static void SetStatusbarIcon(LPSTR pszPath, BOOL bShow){
	static HICON s_hIcon = NULL;
	SHFILEINFO shfinfo = {0};
	char szIconPath[MAX_FITTLE_PATH] = {0};

	if(s_hIcon){
		DestroyIcon(s_hIcon);
		s_hIcon = NULL;
	}
	if(bShow){
		if(IsCueSheetPath(pszPath)){
			char szStart[100], szEnd[100];
			GetCueSheetRealPath(pszPath, szIconPath, szStart, szEnd);
		}else{
			lstrcpyn(szIconPath, pszPath, MAX_FITTLE_PATH);
		}

		SHGetFileInfo(szIconPath, FILE_ATTRIBUTE_NORMAL, &shfinfo, sizeof(shfinfo), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON); 
		s_hIcon = shfinfo.hIcon;
		SendMessage(m_hStatus, SB_SETICON, (WPARAM)0, (LPARAM)s_hIcon);
	}else{
		SendMessage(m_hStatus, SB_SETICON, (WPARAM)0, (LPARAM)NULL);
	}
	return;
}

static BOOL SetUIColor(void){
	HWND hList;
	for(int i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
		hList = GetListTab(m_hTab, i)->hList;
		ListView_SetBkColor(hList, (COLORREF)g_cfg.nBkColor);
		ListView_SetTextBkColor(hList, (COLORREF)g_cfg.nBkColor);
		ListView_SetTextColor(hList, (COLORREF)g_cfg.nTextColor);
	}
	TreeView_SetBkColor(m_hTree, g_cfg.nBkColor);
	TreeView_SetTextColor(m_hTree, g_cfg.nTextColor);
	InitTreeIconIndex(m_hCombo, m_hTree, (BOOL)g_cfg.nTreeIcon);
	InvalidateRect(GetCurListTab(m_hTab)->hList, NULL, TRUE);
	return TRUE;
}

// �������ǂݍ���
static void LoadBookMark(HMENU hBMMenu, char *pszINIPath){
	int i;
	char szSec[10];
	char szMenuBuff[MAX_FITTLE_PATH+4];

	for(i=0;i<MAX_BM_SIZE;i++){
		wsprintf(szSec, "Path%d", i);
		GetPrivateProfileString("BookMark", szSec, "", g_cfg.szBMPath[i], MAX_FITTLE_PATH, pszINIPath);
		if(g_cfg.szBMPath[i][0]){
			if(g_cfg.nBMFullPath){
				wsprintf(szMenuBuff, "&%d: %s", i, g_cfg.szBMPath[i]);
			}else{
				wsprintf(szMenuBuff, "&%d: %s", i, PathFindFileName(g_cfg.szBMPath[i]));
			}
			AppendMenu(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuff);
		}
	}
	return;
}

// �������ۑ�
static void SaveBookMark(char *pszINIPath){
	int i;
	char szSec[10];

	for(i=0;i<MAX_BM_SIZE;i++){
		wsprintf(szSec, "Path%d", i);
		WritePrivateProfileString("BookMark", szSec, (g_cfg.szBMPath[i][0]?g_cfg.szBMPath[i]:NULL), pszINIPath);
	}
	return;
}

// ������ɒǉ��i���ƂŏC���j
static int AddBookMark(HMENU hBMMenu, HWND hWnd){
	int i;
	//HTREEITEM hNode;
	char szMenuBuff[MAX_FITTLE_PATH+4];
	
	/*
	hNode = TreeView_GetSelection(m_hTree);
	if(hNode==NULL || hNode==TVI_ROOT) return -1;
	GetPathFromNode(m_hTree, hNode, szMenuBuff);
	*/

	lstrcpyn(szMenuBuff, m_szTreePath, MAX_FITTLE_PATH+4);

	for(i=0;i<MAX_BM_SIZE;i++){
		if(!g_cfg.szBMPath[i][0]){
			lstrcpyn(g_cfg.szBMPath[i], szMenuBuff, MAX_FITTLE_PATH);
			if(g_cfg.nBMFullPath){
				wsprintf(szMenuBuff, "&%d: %s", i, g_cfg.szBMPath[i]);
			}else{
				wsprintf(szMenuBuff, "&%d: %s", i, PathFindFileName(g_cfg.szBMPath[i]));
			}
			AppendMenu(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuff);
			return i;
		}
	}
	MessageBox(hWnd, "������̌��������z���܂����B", "Fittle", MB_OK | MB_ICONEXCLAMATION);
	return -1;
}

// ����������j���[�ɕ`��
static void DrawBookMark(HMENU hBMMenu){
	int i;
	char szMenuBuff[MAX_FITTLE_PATH+4];

	// �����郁�j���[���폜
	for(i=0;i<MAX_BM_SIZE;i++){
		if(!DeleteMenu(hBMMenu, IDM_BM_FIRST+i, MF_BYCOMMAND)) break;
	}
	// �V�������j���[��ǉ�
	for(i=0;i<MAX_BM_SIZE;i++){
		if(g_cfg.nBMFullPath){
			wsprintf(szMenuBuff, "&%d: %s", i, g_cfg.szBMPath[i]);
		}else{
			wsprintf(szMenuBuff, "&%d: %s", i, PathFindFileName(g_cfg.szBMPath[i]));
		}
		if(g_cfg.szBMPath[i][0]){
			AppendMenu(hBMMenu, MF_STRING | MF_ENABLED, IDM_BM_FIRST+i, szMenuBuff);
		}
	}
	return;
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
	psh.pszCaption = "�ݒ�";
	psh.nPages = 6;
	psh.nStartPage = nStart;
	psh.phpage = hPsp;
	PropertySheet(&psh);
	RegHotKey(hWnd);
	return;
}

static LRESULT CALLBACK MyHookProc(int nCode, WPARAM wp, LPARAM lp){
	MSG *msg;
	msg = (MSG *)lp;

	if(nCode<0)
		return CallNextHookEx(m_hHook, nCode, wp, lp);
	
	if(msg->message==WM_MOUSEWHEEL){
		SendMessage(WindowFromPoint(msg->pt), WM_MOUSEWHEEL, msg->wParam, msg->lParam);
		msg->message = WM_NULL;
	}

	return CallNextHookEx(m_hHook, nCode, wp, lp);
}

// �X���C�_�o�[�̐V�����v���V�[�W��
static LRESULT CALLBACK NewSliderProc(HWND hSB, UINT msg, WPARAM wp, LPARAM lp){
	int nXPos;
	TOOLINFO tin;

	switch(msg)
	{
		case WM_LBUTTONUP: // ���A�b�v��������V�[�N�K�p
			if(GetCapture()==m_hSeek){
				_BASS_ChannelSetPosition(g_cInfo[g_bNow].hChan, SendMessage(hSB, TBM_GETPOS, 0, 0));
			}
		case WM_RBUTTONUP: // �E�A�b�v��������h���b�O�L�����Z��
			if(GetCapture()==m_hSeek || GetCapture()==m_hVolume){
				ReleaseCapture();
				SendMessage(m_hSliderTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&tin);
			}else if(msg!=WM_LBUTTONUP){
				SendMessage(GetParent(m_hStatus), WM_CONTEXTMENU, (WPARAM)GetParent(hSB), lp);
			}
			return 0;

		case WM_LBUTTONDOWN: //�h���b�O���[�h�Ɉڍs
			SetCapture(hSB);
			SendMessage(hSB, WM_MOUSEMOVE, wp, lp);
			return 0;

		case WM_MOUSEMOVE:
			int nPos;
			float fLen;
			RECT rcSB, rcTool, rcThumb;
			POINT pt;
			char szDrawBuff[10];

			//�V�[�N�����̈����������
			if(GetCapture()==m_hSeek || GetCapture()==m_hVolume){
				// �`�����l����RECT(�X���C�_����)���擾
				SendMessage(hSB, TBM_GETCHANNELRECT, 0, (LPARAM)(LPRECT)&rcSB);
				SendMessage(hSB, TBM_GETTHUMBRECT, 0, (LPARAM)(LPRECT)&rcThumb);
				// �}�E�X�̈ʒu(�X���C�_����)���擾
				nXPos = (short)LOWORD(lp);
				if((short)LOWORD(lp)<rcSB.left) nXPos = rcSB.left;
				if((short)LOWORD(lp)>rcSB.right ) nXPos = rcSB.right ;
				//�}�E�X�̉��Ƀo�[���ړ�
				nXPos -= rcSB.left + (rcThumb.right - rcThumb.left)/2;
				SendMessage(hSB, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SLIDER_DIVIDED * nXPos / (rcSB.right - rcSB.left - (rcThumb.right - rcThumb.left))));

				nPos = (int)SendMessage(hSB, TBM_GETPOS, 0 ,0);
				if(hSB==m_hSeek){
					//�o�[�̈ʒu���玞�Ԃ��擾
					fLen = BASS_ChannelBytes2Seconds(g_cInfo[g_bNow].hChan, g_cInfo[g_bNow].qDuration);
					wsprintf(szDrawBuff, "%02d:%02d", (nPos * (int)fLen / SLIDER_DIVIDED)/60, (nPos * (int)fLen / SLIDER_DIVIDED)%60);
				}else if(hSB==m_hVolume){
					wsprintf(szDrawBuff, "%02d%%", nPos/10);
				}

				//�c�[���`�b�v�����A�\��
				ZeroMemory(&tin, sizeof(tin));
				tin.cbSize = sizeof(tin);
				tin.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
				tin.hwnd = GetParent(hSB);
				tin.lpszText = szDrawBuff;
				SendMessage(m_hSliderTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&tin);
				SendMessage(m_hSliderTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&tin);
				SendMessage(m_hSliderTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&tin);
				GetWindowRect(m_hSliderTip, &rcTool);
				pt.x = nXPos + rcSB.left - (int)((rcThumb.right - rcThumb.left)*1.5);
				pt.y = rcSB.bottom;
				ClientToScreen(hSB, &pt);
				SendMessage(m_hSliderTip, TTM_TRACKPOSITION, 0,
					(LPARAM)(DWORD)MAKELONG(pt.x,  pt.y + 20));	// �X���C�_��20Pixel���ɕ\��

				if(hSB==m_hVolume){
					BASS_ChannelSetAttribute(m_hChanOut, BASS_ATTRIB_VOL, nPos / (float)SLIDER_DIVIDED);
				}

			}
			break;

		case WM_MOUSEWHEEL:
			if(hSB==m_hSeek){
				if((short)HIWORD(wp)<0){
					SendMessage(GetParent(m_hStatus), WM_COMMAND, MAKEWPARAM(IDM_SEEKFRONT, 0), 0);
				}else{
					SendMessage(GetParent(m_hStatus), WM_COMMAND, MAKEWPARAM(IDM_SEEKBACK, 0), 0);
				}
			}else if(hSB==m_hVolume){
				if((short)HIWORD(wp)<0){
					SendMessage(hSB, TBM_SETPOS, TRUE, (WPARAM)SendMessage(hSB, TBM_GETPOS, 0, 0) - SLIDER_DIVIDED/50);
				}else{
					SendMessage(hSB, TBM_SETPOS, TRUE, (WPARAM)SendMessage(hSB, TBM_GETPOS, 0, 0) + SLIDER_DIVIDED/50);
				}
			}
			return 0;

		case WM_HSCROLL:
			return 0; // �`���g�z�C�[���}�E�X���~����

		case WM_MBUTTONDOWN:	// ���{�^���N���b�N
			if(hSB==m_hSeek){
				FilePause();	// �|�[�Y�ɂ��Ă݂�
			}else if(hSB==m_hVolume){
				SendMessage(hSB, TBM_SETPOS, TRUE, (WPARAM)0);	// �~���[�g�ɂ��Ă݂�
			}
			return 0; // �t�H�[�J�X�𓾂�̂�h��

		case TBM_SETPOS:
			if(hSB==m_hVolume){
				BASS_ChannelSetAttribute(m_hChanOut, BASS_ATTRIB_VOL,lp / (float)SLIDER_DIVIDED);
			}
			break;

		default:
			break;
	}
	return CallWindowProc((WNDPROC)(LONG64)GetWindowLongPtr(hSB, GWLP_USERDATA), hSB, msg, wp, lp);
}

static LRESULT CALLBACK NewSplitBarProc(HWND hSB, UINT msg, WPARAM wp, LPARAM lp){
	static int s_nPreX;
	static int s_nMouseState = 0;

	RECT rcDisp;
	POINT pt;

	switch(msg){
		case WM_MOUSEMOVE:
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			if(s_nMouseState==1){
				GetCursorPos(&pt);
				g_cfg.nTreeWidth += pt.x - s_nPreX;
				//g_cfg.nTWidthSub = g_cfg.nTreeWidth;
				UpdateWindowSize(GetParent(hSB));
				s_nPreX = pt.x;
			}
			break;

		case WM_MOUSELEAVE:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			break;

		case WM_LBUTTONDOWN:
			if(g_cfg.nTreeState==TREE_SHOW){
				// �}�E�X�̈ړ��͈͂𐧌�
				GetWindowRect(GetParent(hSB), &rcDisp);
				rcDisp.right -= SPLITTER_WIDTH*2;
				rcDisp.left += SPLITTER_WIDTH;
				ClipCursor(&rcDisp);
				// �X�v���b�^�h���b�O�J�n
				s_nMouseState = 1;
				GetCursorPos(&pt);
				s_nPreX = pt.x;
				SetCapture(hSB);
			}
			break;

		case WM_LBUTTONUP:
			if(g_cfg.nTreeState==TREE_SHOW){
				// �X�v���b�^�h���b�O�I��
				ClipCursor(NULL);
				s_nMouseState = 0;
				ReleaseCapture();
			}
			break;

		case WM_LBUTTONDBLCLK:	// �c���[�r���[���g�O��
			/*if(g_cfg.nTreeWidth<=0){	// �P���ɓW�J
				if(g_cfg.nTWidthSub<=0) g_cfg.nTWidthSub = 200;	// �蓮�ō��[�܂ł��������Ƃ̓W�J
				g_cfg.nTreeWidth = g_cfg.nTWidthSub;
			}else{						// ��\��
				g_cfg.nTWidthSub = g_cfg.nTreeWidth;
				g_cfg.nTreeWidth = -SPLITTER_WIDTH;
			}
			UpdateWindowSize(GetParent(hSB));
			MyScroll(m_hTree);
			*/
			SendMessage(GetParent(hSB), WM_COMMAND, (WPARAM)IDM_TOGGLETREE, 0);
			break;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hSB, GWLP_USERDATA), hSB, msg, wp, lp);
}

static LRESULT CALLBACK NewTabProc(HWND hTC, UINT msg, WPARAM wp, LPARAM lp){
	static int s_nDragTab = -1;
	TCHITTESTINFO tchti;
	RECT rcItem = {0};

	switch(msg)
	{
		case WM_LBUTTONDOWN:
			tchti.flags = TCHT_NOWHERE;
			tchti.pt.x = (short)LOWORD(lp);
			tchti.pt.y = (short)HIWORD(lp);
			s_nDragTab = TabCtrl_HitTest(hTC, &tchti);
			if(s_nDragTab>0){
				SetCapture(hTC);
			}
			break;

		case WM_LBUTTONUP:
			if(GetCapture()==hTC){
				ReleaseCapture();
				s_nDragTab = -1;
			}
			break;

		case WM_MOUSEMOVE:
			int nHitTab;
			if(GetCapture()==hTC){
				tchti.flags = TCHT_NOWHERE;
				tchti.pt.x = (short)LOWORD(lp);
				tchti.pt.y = (short)HIWORD(lp);
				nHitTab = TabCtrl_HitTest(hTC, &tchti);
				if(nHitTab>0 && nHitTab!=s_nDragTab){
					MoveTab(hTC, s_nDragTab, nHitTab-s_nDragTab);
					s_nDragTab = nHitTab;
				}
			}
			break;

		case WM_CONTEXTMENU:	// ���X�g�E�N���b�N���j���[
			HMENU hPopMenu;
			int nItem;
			int i;
			char szMenu[MAX_FITTLE_PATH+4];

			switch(GetDlgCtrlID((HWND)wp)){
				case ID_LIST:
					if(lp==-1){	//�L�[�{�[�h
						nItem = ListView_GetNextItem((HWND)wp, -1, LVNI_SELECTED);
						if(nItem<0) return 0; 
						ListView_GetItemRect((HWND)wp, nItem, &rcItem, LVIR_SELECTBOUNDS);
						MapWindowPoints((HWND)wp, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
						lp = MAKELPARAM(rcItem.left, rcItem.bottom);
					}
					hPopMenu = GetSubMenu(LoadMenu(m_hInst, "LISTMENU"), 0);

					for(i=0;i<TabCtrl_GetItemCount(hTC);i++){
						wsprintf(szMenu, "&%d: %s", i, GetListTab(hTC, i)->szTitle);
						AppendMenu(GetSubMenu(hPopMenu, 8), MF_STRING, IDM_SENDPL_FIRST+i, szMenu); 
					}

					/*if(ListView_GetSelectedCount((HWND)wp)!=1){
						EnableMenuItem(hPopMenu, IDM_LIST_DELFILE, MF_GRAYED | MF_DISABLED);
					}*/
					TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, GetParent(hTC), NULL);
					DestroyMenu(hPopMenu);
					return 0;	// �^�u�R���g���[���̕���WM_CONTEXTMENU��������̂�h��
			}
			break;

		case WM_NOTIFY:
			NMHDR *pnmhdr;

			pnmhdr = (LPNMHDR)lp;
			switch(pnmhdr->idFrom)
			{
				case ID_LIST:
					LPNMLISTVIEW lplv;
					lplv = (LPNMLISTVIEW)lp;
					switch(pnmhdr->code)
					{

						// ���X�g�r���[����̗v��
						case LVN_GETDISPINFO:
						case LVN_GETDISPINFOW:
							LVITEM* item;
							struct FILEINFO *pTmp;
							item = &((NMLVDISPINFO*)lp)->item;

							// �e�L�X�g���Z�b�g
							if(item->mask & LVIF_TEXT){
								for(i=0;pnmhdr->hwndFrom!=GetListTab(m_hTab, i)->hList;i++);
								pTmp = GetListTab(m_hTab, i)->pRoot;
								switch(item->iSubItem){
									case 0:
										lstrcpyn(item->pszText, GetFileName(GetPtrFromIndex(pTmp, item->iItem)->szFilePath), MAX_FITTLE_PATH);
										break;
									case 1:
										lstrcpyn(item->pszText, GetPtrFromIndex(pTmp, item->iItem)->szSize, MAX_FITTLE_PATH);
										break;
									case 2:
										LPSTR pszPath;
										pszPath = GetPtrFromIndex(pTmp, item->iItem)->szFilePath;
										if(IsURLPath(pszPath)){
											lstrcpyn(item->pszText, "URL", MAX_FITTLE_PATH);
										}else if(IsCueSheetPath(pszPath)){
											lstrcpyn(item->pszText, "CUE", MAX_FITTLE_PATH);
										}else{
											char *p = PathFindExtension(pszPath);
											if (p && *p) lstrcpyn(item->pszText, p+1, MAX_FITTLE_PATH);
										}
										break;
									case 3:
										lstrcpyn(item->pszText, GetPtrFromIndex(pTmp, item->iItem)->szTime, MAX_FITTLE_PATH);
										break;
								}
							}
							break;

					
						case LVN_GETINFOTIP:
						case LVN_GETINFOTIPW:
							NMLVGETINFOTIPW *lvgit;
							WCHAR wFilename[MAX_FITTLE_PATH];

							if(!g_cfg.nPathTip) break;
							lvgit = (NMLVGETINFOTIPW *)lp;
							lvgit->dwFlags = 0;
							lvgit->cchTextMax = MAX_FITTLE_PATH;
							MultiByteToWideChar(CP_ACP, 0, GetPtrFromIndex(GetCurListTab(m_hTab)->pRoot, lvgit->iItem)->szFilePath, -1, wFilename, MAX_FITTLE_PATH); //S-JIS->Unicode
							lstrcpyW(lvgit->pszText, wFilename);
							break;

						case NM_CUSTOMDRAW:
							NMLVCUSTOMDRAW *lplvcd;
							LOGFONT lf;

							lplvcd = (LPNMLVCUSTOMDRAW)lp;
							if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
								return CDRF_NOTIFYITEMDRAW;
							if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
								if(m_nPlayTab==TabCtrl_GetCurSel(hTC)
									&& GetCurListTab(hTC)->nPlaying==(signed int)lplvcd->nmcd.dwItemSpec
									&& BASS_ChannelIsActive(m_hChanOut)/*>=BASS_ACTIVE_PLAYING*/ ){
										switch(g_cfg.nPlayView){
											case 1:
												if(!m_hBoldFont){
													GetObject(GetCurrentObject(lplvcd->nmcd.hdc, OBJ_FONT), sizeof(LOGFONT), &lf);
													lf.lfWeight = FW_DEMIBOLD;
													m_hBoldFont = CreateFontIndirect(&lf);
												}
												SelectObject(lplvcd->nmcd.hdc, m_hBoldFont);
												break;
											case 2:
												lplvcd->clrText = g_cfg.nPlayTxtCol;
												break;
											case 3:
												lplvcd->clrTextBk = g_cfg.nPlayBkCol;
												break;
										}
										return CDRF_NEWFONT;
								}
							}
							break;

						case LVN_BEGINDRAG:	// �A�C�e���h���b�O�J�n
							OnBeginDragList(pnmhdr->hwndFrom, lplv->ptAction);
							break;

						case LVN_COLUMNCLICK:	// �J�����N���b�N
							int nSort;

							if(lplv->iSubItem+1==abs(GetCurListTab(hTC)->nSortState)){
								nSort = -1*GetCurListTab(hTC)->nSortState;
							}else{
								nSort = lplv->iSubItem+1;
							}
							SortListTab(GetCurListTab(hTC), nSort);
							break;

						case NM_DBLCLK:
							SendMessage(GetParent(hTC), WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_PLAY, 0), 0);
							break;

					}
					break;
			}
			break;

		case WM_COMMAND:
			SendMessage(GetParent(hTC), msg, wp, lp); //�e�E�B���h�E�ɂ��̂܂ܑ��M
			break;

		case WM_CHAR:
			if((int)wp=='\t'){
				if(GetKeyState(VK_SHIFT) < 0){
					SetFocus(m_hTree);
				}else{
					SetFocus(GetCurListTab(hTC)->hList);
				}
				return 0;
			}
			break;

		case WM_TIMER:
			switch (wp){
				case ID_SCROLLTIMERUP:		// ��X�N���[��
					ListView_Scroll(GetCurListTab(hTC)->hList, 0, -10);
					break;

				case ID_SCROLLTIMERDOWN:	// ���X�N���[��
					ListView_Scroll(GetCurListTab(hTC)->hList, 0, 10);
					break;
			}
			break;

		case WM_MOUSEWHEEL:
			if((short)HIWORD(wp)<0){
				TabCtrl_SetCurFocus(hTC, TabCtrl_GetItemCount(hTC)==TabCtrl_GetCurSel(hTC)+1?0:TabCtrl_GetCurSel(hTC)+1);
			}else{
				TabCtrl_SetCurFocus(hTC, (TabCtrl_GetCurSel(hTC)==0?TabCtrl_GetItemCount(hTC)-1:TabCtrl_GetCurSel(hTC)-1));
			}
			break;

		case WM_LBUTTONDBLCLK:
			m_nPlayTab = TabCtrl_GetCurSel(hTC);
			SendMessage(GetParent(hTC), WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_NEXT, 0), 0);
			break;

		case WM_DROPFILES:
			HDROP hDrop;
			struct FILEINFO *pSub;
			int nFileCount;
			POINT pt;
			char szPath[MAX_FITTLE_PATH];
			char szLabel[MAX_FITTLE_PATH];

			// �h���b�O���ꂽ�t�@�C����ǉ�
			pSub = NULL;
			hDrop = (HDROP)wp;
			nFileCount = (int)DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			for(i=0;i<nFileCount;i++){
				DragQueryFile(hDrop, i, szPath, MAX_FITTLE_PATH);
				SearchFiles(&pSub, szPath, TRUE);
			}
			DragQueryPoint(hDrop, &pt);
			DragFinish(hDrop);

			// �����̃^�u�ɒǉ�
			nItem = TabCtrl_GetItemCount(hTC);
			for(i=0;i<nItem;i++){
				TabCtrl_GetItemRect(hTC, i, &rcItem);
				if(PtInRect(&rcItem, pt)){
					ListView_SetItemState(GetListTab(hTC, i)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);	// �I����ԃN���A
					InsertList(GetListTab(hTC, i), -1, pSub);
					ListView_EnsureVisible(GetListTab(hTC, i)->hList, ListView_GetItemCount(GetListTab(hTC, i)->hList)-1, TRUE);	// ��ԉ��̃A�C�e����\��
					TabCtrl_SetCurFocus(hTC, i);
					break;
				}
			}

			// �^�u��V�K�ɒǉ�
			if(nFileCount==1){
				lstrcpyn(szLabel, PathFindFileName(szPath), MAX_FITTLE_PATH);
				if(PathIsDirectory(szPath)){
					PathRemoveBackslash(szLabel);
				}else{
					PathRemoveExtension(szLabel);
				}
			}else{
				lstrcpy(szLabel, "Default");
			}
			if(pt.x>rcItem.right && pt.y>=rcItem.top && pt.y<=rcItem.bottom){
				MakeNewTab(hTC, szLabel, -1);
				InsertList(GetListTab(hTC, nItem), -1, pSub);
				ListView_SingleSelect(GetListTab(hTC, nItem)->hList, 0);
				TabCtrl_SetCurFocus(hTC, nItem);
			}
			break;

		default:
			break;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hTC, GWLP_USERDATA), hTC, msg, wp, lp);
}

// ���X�g�r���[�̐V�����v���V�[�W��
LRESULT CALLBACK NewListProc(HWND hLV, UINT msg, WPARAM wp, LPARAM lp){
	LVHITTESTINFO pinfo;
	RECT rcItem;
	POINT pt;
	struct FILEINFO *pSub = NULL;
	// struct FILEINFO *pPlaying = NULL;
	int i;
	HWND hWnd;

	switch (msg) {
		case WM_MOUSEMOVE:
			if(GetCapture()==hLV){
				// �J�[�\����ύX
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);
				if(hWnd==m_hTab){
					SetOLECursor(3);
				}else if(hWnd==GetCurListTab(m_hTab)->hList){
					SetOLECursor(2);
				}else{
					SetOLECursor(1);
				}
				// �^�u�t�H�[�J�X�̕`��
				ScreenToClient(GetParent(hLV), &pt);
				for(i=0;i<TabCtrl_GetItemCount(GetParent(hLV));i++){
					TabCtrl_GetItemRect(GetParent(hLV), i, &rcItem);
					if(PtInRect(&rcItem, pt)){
						DrawTabFocus(GetParent(hLV), i, TRUE);
					}else{
						DrawTabFocus(GetParent(hLV), i, FALSE);
					}
				}
				// �h���b�O�C���[�W��`��
				pt.x = (short)LOWORD(lp);
				pt.y = (short)HIWORD(lp);
				OnDragList(hLV, pt);
			}
			break;

		case WM_RBUTTONDOWN:
			if(GetCapture()==hLV){
				OnEndDragList(hLV);	// �h���b�O�L�����Z��
			}
			break;

		case WM_LBUTTONUP:
			int nBefore, nAfter, nCount;

			if(GetCapture()==hLV){
				OnEndDragList(hLV); //�h���b�O�I��
				//�ړ��O�A�C�e���ƈړ���A�C�e���̃C���f�b�N�X���擾
				pinfo.pt.x = (short)LOWORD(lp);
				pinfo.pt.y = (short)HIWORD(lp);
				nBefore = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
				nAfter = ListView_HitTest(hLV, &pinfo);
				if(nAfter!=-1 && nBefore!=nAfter){
					ChangeOrder(GetCurListTab(m_hTab), nAfter);
				}else{
					//�^�u�փh���b�O
					GetCursorPos(&pt);
					ScreenToClient(m_hTab, &pt);
					for(i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
						TabCtrl_GetItemRect(m_hTab, i, &rcItem);
						if(PtInRect(&rcItem, pt)){
							DrawTabFocus(m_hTab, i, FALSE);
							SendToPlaylist(GetCurListTab(m_hTab), GetListTab(m_hTab, i));
							nCount = ListView_GetItemCount(GetListTab(m_hTab ,i)->hList) - 1;
							ListView_SingleSelect(GetListTab(m_hTab ,i)->hList, nCount);
							break;
						}
					}
				}
			}
			break;

		case WM_KEYDOWN:
			switch (wp)
			{
				case VK_RETURN:
					SendMessage(GetParent(GetParent(hLV)), WM_COMMAND, (WPARAM)IDM_PLAY, 0);
					break;

				case VK_DELETE:
					DeleteFiles(GetCurListTab(GetParent(hLV)));
					break;

				case VK_TAB:
					if(g_cfg.nTreeState!=TREE_SHOW) break;
					if(GetKeyState(VK_SHIFT) < 0){
						SetFocus(GetParent(hLV));
					}else{
						SetFocus(m_hCombo);
					}
					break;

				case VK_DOWN:
					int nToIndex;
					if(GetKeyState(VK_CONTROL) < 0){
						nToIndex = ListView_GetNextItem(hLV, -1, LVIS_SELECTED);
						do{
							nToIndex++;
						}while(ListView_GetItemState(hLV, nToIndex, LVIS_SELECTED));
						ChangeOrder(GetCurListTab(GetParent(hLV)), nToIndex);
						return 0;
					}
					break;

				case VK_UP:
					if(GetKeyState(VK_CONTROL) < 0){
						nToIndex = ListView_GetNextItem(hLV, -1, LVIS_SELECTED);
						if(nToIndex>0){
							ChangeOrder(GetCurListTab(GetParent(hLV)), nToIndex-1);
						}
						return 0;
					}
					break;

				case VK_LEFT:
					SCROLLINFO sinfo;

					ZeroMemory(&sinfo, sizeof(SCROLLINFO));
					sinfo.cbSize = sizeof(SCROLLINFO);
					sinfo.fMask = SIF_POS;
					GetScrollInfo(hLV, SB_HORZ, &sinfo);
					if(g_cfg.nTreeState==TREE_SHOW && sinfo.nPos==0){
						SetFocus(m_hTree);
						TreeView_EnsureVisible(m_hTree, TreeView_GetSelection(m_hTree));
						MyScroll(m_hTree);
					}
					break;

				default:
					break;
			}
			break;

		case WM_DROPFILES:
			HDROP hDrop;
			char szPath[MAX_FITTLE_PATH];

			/*	�J�[�\���̉��ɒǉ�
			int nHitItem;
			GetCursorPos(&pt);
			ScreenToClient(hLV, &pt);
			pinfo.pt.x = pt.x;
			pinfo.pt.y = pt.y;
			nHitItem = ListView_HitTest(hLV, &pinfo);
			*/
			hDrop = (HDROP)wp;
			//pPlaying = GetPtrFromIndex(GetCurListTab(GetParent(hLV))->pRoot, GetCurListTab(GetParent(hLV))->nPlaying);
			for(i=0;i<(int)DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);i++){
				DragQueryFile(hDrop, i, szPath, MAX_FITTLE_PATH);
				SearchFiles(&pSub, szPath, TRUE);
			}
			DragFinish(hDrop);

			ListView_SetItemState(hLV, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);	// �\�����N���A
			InsertList(GetCurListTab(GetParent(hLV)), -1, pSub);
			ListView_EnsureVisible(hLV, ListView_GetItemCount(hLV)-1, TRUE);
			//GetCurListTab(GetParent(hLV))->nPlaying = GetIndexFromPtr(GetCurListTab(GetParent(hLV))->pRoot, pPlaying);
			SetForegroundWindow(GetParent(GetParent(hLV)));
			break;

		default:
			break;
	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hLV, GWLP_USERDATA), hLV, msg, wp, lp);
}

// �c���[�r���[�̃v���V�[�W��
static LRESULT CALLBACK NewTreeProc(HWND hTV, UINT msg, WPARAM wp, LPARAM lp){
	int i;
	HTREEITEM hti;
	static int s_nPrevCur;

	switch(msg){
		case WM_LBUTTONDBLCLK:	// �q�������Ȃ��m�[�h�_�u���N���b�N�ōĐ�
			if(!TreeView_GetChild(hTV, TreeView_GetSelection(hTV))){
				m_nPlayTab = 0;
				SendMessage(GetParent(hTV), WM_COMMAND, (WPARAM)IDM_NEXT, 0);
			}
			break;

		case WM_CHAR:
			switch(wp){
				case '\t':
					if(GetKeyState(VK_SHIFT) < 0){
						SetFocus(GetNextWindow(hTV, GW_HWNDPREV));
					}else{
						SetFocus(GetNextWindow(hTV, GW_HWNDNEXT));
					}
					return 0;

				case VK_RETURN:
					if(ListView_GetItemCount(GetListTab(m_hTab, 0)->hList)>0){
						m_nPlayTab = 0;
						SendMessage(GetParent(hTV), WM_COMMAND, (WPARAM)IDM_NEXT, 0);
					}else{
						TreeView_Expand(hTV, TreeView_GetSelection(hTV), TVE_TOGGLE);
					}
					return 0;

				default:
					break;
			}
			break;

		case WM_KEYDOWN:
			if(wp==VK_RIGHT){	// �q�m�[�h���Ȃ� or �W�J��ԂȂ��List�Ƀt�H�[�J�X���ڂ�
				hti = TreeView_GetSelection(hTV);

				TVITEM tvi;
				tvi.mask = TVIF_STATE;
				tvi.hItem = hti;
				tvi.stateMask = TVIS_EXPANDED;
				TreeView_GetItem(hTV, &tvi);

				if(!TreeView_GetChild(hTV, hti) || (tvi.state & TVIS_EXPANDED)){
					SetFocus(GetCurListTab(m_hTab)->hList);
					return 0;
				}
				break;
			}
			break;

		case WM_LBUTTONUP:
			POINT pt;
			HWND hWnd;
			struct FILEINFO *pSub;
			pSub = NULL;
			char szNowDir[MAX_FITTLE_PATH];
			int nHitTab;
			TCHITTESTINFO info;
			RECT rcTab;
			RECT rcList;

			if(GetCapture()==hTV){
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);
				if(hWnd==m_hTab){
					GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					SearchFiles(&pSub, szNowDir, TRUE);
					ScreenToClient(m_hTab, &pt);
					info.pt.x = pt.x;
					info.pt.y = pt.y;
					info.flags = TCHT_ONITEM;
					nHitTab = TabCtrl_HitTest(m_hTab, &info);
					if(nHitTab!=-1){
						DrawTabFocus(m_hTab, nHitTab, FALSE);
						ListView_SetItemState(GetListTab(m_hTab, nHitTab)->hList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
						InsertList(GetListTab(m_hTab, nHitTab), -1, pSub);
						ListView_EnsureVisible(GetListTab(m_hTab, nHitTab)->hList, ListView_GetItemCount(GetListTab(m_hTab, nHitTab)->hList)-1, TRUE);
					}
				}else if(hWnd==GetCurListTab(m_hTab)->hList){
					SendMessage(GetParent(hTV), WM_COMMAND, MAKEWPARAM(IDM_TREE_ADD, 0), 0);
				}else if(hWnd==GetParent(hTV)){
					// �V�K�^�u
					GetWindowRect(GetCurListTab(m_hTab)->hList, &rcList);
					GetWindowRect(m_hTab, &rcTab);

					if(g_cfg.nTabBottom==0 && pt.y>=rcTab.top && pt.y<=rcList.top && pt.x>=rcTab.left
					|| g_cfg.nTabBottom==1 && pt.y<=rcTab.bottom && pt.y>=rcList.bottom && pt.x>=rcTab.left){
						SendMessage(GetParent(hTV), WM_COMMAND, MAKEWPARAM(IDM_TREE_NEW, 0), 0);
					}
				}
				ReleaseCapture();
			}
			break;

		case WM_RBUTTONUP:
			if(GetCapture()==hTV){
				ReleaseCapture();
				for(i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
					DrawTabFocus(m_hTab, i, FALSE);
				}
			}
			break;

		case WM_MOUSEMOVE:
			RECT rcItem;

			if(GetCapture()==m_hTree){
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);

				if(hWnd==m_hTab || hWnd==GetCurListTab(m_hTab)->hList){
					SetOLECursor(3);
				}else if(hWnd==GetParent(m_hTab)){
					// �V�K�^�u
					GetWindowRect(GetCurListTab(m_hTab)->hList, &rcList);
					GetWindowRect(m_hTab, &rcTab);

					if(g_cfg.nTabBottom==0 && pt.y>=rcTab.top && pt.y<=rcList.top && pt.x>=rcTab.left
					|| g_cfg.nTabBottom==1 && pt.y<=rcTab.bottom && pt.y>=rcList.bottom && pt.x>=rcTab.left){
						SetOLECursor(3);
					}else{
						SetOLECursor(1);
					}
				}else{
					SetOLECursor(1);
				}
				
				// �^�u�̃t�H�[�J�X��`��
				ScreenToClient(m_hTab, &pt);
				for(i=0;i<TabCtrl_GetItemCount(m_hTab);i++){
					TabCtrl_GetItemRect(m_hTab, i, &rcItem);
					if(PtInRect(&rcItem, pt)){
						DrawTabFocus(m_hTab, i, TRUE);
					}else{
						DrawTabFocus(m_hTab, i, FALSE);
					}
				}
			}
			break;

		case WM_DROPFILES:
			HDROP hDrop;
			char szPath[MAX_FITTLE_PATH];
			char szParDir[MAX_PATH];

			hDrop = (HDROP)wp;
			DragQueryFile(hDrop, 0, szPath, MAX_FITTLE_PATH);
			DragFinish(hDrop);

			GetParentDir(szPath, szParDir);
			MakeTreeFromPath(hTV, m_hCombo, szParDir);

			SetForegroundWindow(GetParent(GetParent(hTV)));
			break;

	}
	return CallWindowProc((WNDPROC)(LONG_PTR)GetWindowLongPtr(hTV, GWLP_USERDATA), hTV, msg, wp, lp);
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
			SendDlgItemMessage(hDlg, IDC_CHECK7, BM_SETCHECK, (WPARAM)g_cfg.nMiniTop, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK8, BM_SETCHECK, (WPARAM)g_cfg.nMiniTimeShow, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK9, BM_SETCHECK, (WPARAM)g_cfg.nMiniScroll, 0);
			for(i=1;i<=60;i++){
				wsprintf(szBuff, "%d", i);
				SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, (WPARAM)0, (LPARAM)szBuff);
			}
			SendDlgItemMessage(hDlg, IDC_CHECK11, BM_SETCHECK, (WPARAM)g_cfg.nMiniToolShow, 0);
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
				g_cfg.nMiniTop = (int)SendDlgItemMessage(hDlg, IDC_CHECK7, BM_GETCHECK, 0, 0);
				g_cfg.nMiniTimeShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK8, BM_GETCHECK, 0, 0);
				g_cfg.nMiniScroll = (int)SendDlgItemMessage(hDlg, IDC_CHECK9, BM_GETCHECK, 0, 0);
				g_cfg.nMiniToolShow = (int)SendDlgItemMessage(hDlg, IDC_CHECK11, BM_GETCHECK, 0, 0);
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
				bi.lpszTitle = "�N�����ɊJ���t�H���_��I�����Ă�������";
				pidl = SHBrowseForFolder(&bi);
				if(pidl){
					SHGetPathFromIDList(pidl, szBuff);
					CoTaskMemFree(pidl);
					SetDlgItemText(hDlg, IDC_EDIT1, szBuff);
				}
				return TRUE;
			}else if(LOWORD(wp)==IDC_BUTTON2){
				// TODO �ꏏ�ɂ���
				GetWindowText(GetDlgItem(hDlg, IDC_EDIT3), szBuff, MAX_FITTLE_PATH);
				if(OpenFilerPath(szBuff, hDlg, "�t�@�C����I�����Ă�������")){
					SetWindowText(GetDlgItem(hDlg, IDC_EDIT3), szBuff);
				}
			}else if(LOWORD(wp)==IDC_BUTTON3){
				GetWindowText(GetDlgItem(hDlg, IDC_EDIT4), szBuff, MAX_FITTLE_PATH);
				if(OpenFilerPath(szBuff, hDlg, "�O���c�[����I�����Ă�������")){
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

				case IDC_BUTTON5:	// �W���ɖ߂�
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

// �G�f�B�b�g�{�b�N�X�̃v���V�[�W��
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
			lstrcpy(lf.lfFaceName , "�l�r �o�S�V�b�N");
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
			// DLL�A�C�R���ݒ�
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
				if(!g_cfg.szBMPath[i][0]) break;
				item.pszText = g_cfg.szBMPath[i];
				nMax = ListView_GetStringWidth(hList, item.pszText)+10>nMax?ListView_GetStringWidth(hList, item.pszText)+10:nMax;
				item.iItem = i;
				ListView_InsertItem(hList, &item);
			}
			ListView_SetColumnWidth(hList, 0, nMax);
			ListView_SetItemState(hList, 0, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
			SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)(g_cfg.nBMRoot?BST_CHECKED:BST_UNCHECKED), 0);
			SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, (WPARAM)(g_cfg.nBMFullPath?BST_CHECKED:BST_UNCHECKED), 0);
			return FALSE;

		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDOK:
					int nCount;

					nCount = ListView_GetItemCount(hList);
					for(i=0;i<nCount;i++){
						ListView_GetItemText(hList, i, 0, g_cfg.szBMPath[i], MAX_FITTLE_PATH);
					}
					for(i=nCount;i<MAX_BM_SIZE;i++){
						lstrcpy(g_cfg.szBMPath[i], "");
					}
					g_cfg.nBMRoot = SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0);
					g_cfg.nBMFullPath = SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0);
					EndDialog(hDlg, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, -1);
					return TRUE;

				case IDC_BUTTON1:	// ���
					int nSel;
					char szPath[MAX_FITTLE_PATH], szSub[MAX_FITTLE_PATH];

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


// �c�[���o�[�̕����擾�i�h���b�v�_�E���������Ă����܂��s���܂��j
static LONG GetToolbarTrueWidth(HWND hToolbar){
	RECT rct;

	SendMessage(hToolbar, TB_GETITEMRECT, SendMessage(hToolbar, TB_BUTTONCOUNT, 0, 0)-1, (LPARAM)&rct);
	return rct.right;
}

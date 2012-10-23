/*
 * Fittle.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdio.h>
#endif

#include "fittle.h"
#include "func.h"
#include "tree.h"
#include "list.h"
#include "listtab.h"
#include "finddlg.h"
#include "bass_tag.h"
#include "archive.h"
#include "mt19937ar.h"
#include "plugins.h"
#include "plugin.h"
#include "f4b24.h"
#include "oplugin.h"
#include "gplugin.h"

// ���C�u�����������N
#if defined(_MSC_VER)
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#if (_MSC_VER >= 1200) && (_MSC_VER < 1500)
#pragma comment(linker, "/OPT:NOWIN98")
#elif (_MSC_VER >= 1500) && (_MSC_VER < 1700)
#pragma comment(lib, "../../extra/smartvc9/smartvc9.lib")
#pragma comment(linker,"/NODEFAULTLIB:msvcrt.lib")
#pragma comment(linker,"/ENTRY:SmartStartup")
#endif
#endif

// �\�t�g���i�o�[�W�����A�b�v���ɖY�ꂸ�ɍX�V�j
#define FITTLE_VERSION TEXT("Fittle Ver.2.2.2 Preview 3")
#define F4B24_IF_VERSION 39
#define F4B24_VERSION 49
#ifdef UNICODE
#define F4B24_VERSION_STRING TEXT("test49u")
#else
#define F4B24_VERSION_STRING TEXT("test49")
#endif
#ifndef _DEBUG
#define FITTLE_TITLE TEXT("Fittle - f4b24 ") F4B24_VERSION_STRING
#else
#define FITTLE_TITLE TEXT("Fittle - f4b24 ") F4B24_VERSION_STRING TEXT(" <Debug>")
#endif

/* 1:�o�̓v���O�C���Ȃ��ł������o�͉\ 0:�����o�͂̓v���O�C���ɔC���� */
#define DEFAULT_DEVICE_ENABLE 0

#define ODS(X) \
	OutputDebugString(X); OutputDebugString(TEXT("\n"));

enum {
	WM_F4B24_INTERNAL_PREPARE_NEXT_MUSIC = 1,
	WM_F4B24_INTERNAL_PLAY_NEXT = 2, 
	WM_F4B24_INTERNAL_RESTORE_POSITION = 3
};
// --�񋓌^�̐錾--
enum {PM_LIST=0, PM_RANDOM, PM_SINGLE};	// �v���C���[�h
enum {GS_NOEND=0, GS_OK, GS_FAILED, GS_NEWFREQ, GS_END};	// EventSync�̖߂�l�݂�����
enum {TREE_UNSHOWN=0, TREE_SHOW, TREE_HIDE};	// �c���[�̕\�����

//--�֐��̃v���g�^�C�v�錾--
// �v���V�[�W��
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK MyHookProc(int, WPARAM, LPARAM);
static LRESULT CALLBACK NewSliderProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewTabProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewTreeProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewSplitBarProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK NewComboEditProc(HWND, UINT, WPARAM, LPARAM);
static void CALLBACK EventSync(DWORD, DWORD, DWORD, void *);
// �ݒ�֌W
static void LoadState();
static void LoadConfig();
static void SaveState(HWND);
static void ApplyConfig(HWND hWnd);
// �R���g���[���֌W
static void StatusBarDisplayTime();
static void SelectListItem(struct LISTTAB *pList, LPTSTR lpszPath);
static void SelectFolderItem(LPTSTR lpszPath);
static BOOL GetTabName(LPTSTR szTabName);
struct LISTTAB *CreateTab(LPTSTR szTabName); 
static void DoTrayClickAction(int);
static void PopupTrayMenu();
static void PopupPlayModeMenu(HWND, NMTOOLBAR *);
static void ToggleWindowView(HWND);
static HWND CreateToolBar();
static HWND CreateToolTip(HWND hParent);
static HWND CreateTrackBar(int nId, DWORD dwStyle);

static int ShowToolTip(HWND, HWND, LPTSTR);
static int ControlPlayMode(HMENU, int);
static BOOL ToggleRepeatMode(HMENU);
static int SetTaskTray(HWND);
static int RegHotKey(HWND);
static int UnRegHotKey(HWND);
static void OnBeginDragList(HWND, POINT);
static void OnDragList(HWND, POINT);
static void OnEndDragList(HWND);
static void DrawTabFocus(int, BOOL);
static BOOL SetUIFont(void);
static BOOL SetUIColor(void);
static void SetStatusbarIcon(LPTSTR, BOOL);
static LPTSTR MallocAndConcatPath(LISTTAB *);
static void MyShellExecute(HWND, LPTSTR, LPTSTR);
static void InitFileTypes();

// ���t����֌W
static BOOL SetChannelInfo(BOOL, struct FILEINFO *);
static BOOL _BASS_ChannelSetPosition(DWORD, int);
static void _BASS_ChannelSeekSecond(DWORD, float, int);
static BOOL PlayByUser(HWND, struct FILEINFO *);
static void OnChangeTrack();
static int FilePause();
static int StopOutputStream();
static struct FILEINFO *SelectNextFile(BOOL);
static void FreeChannelInfo(BOOL bFlag);

static void RemoveFiles();

// �����o�ϐ�
static CRITICAL_SECTION m_cs;

// �����o�ϐ��i��ԕҁj
static int m_nPlayTab = -1;			// �Đ����̃^�u�C���f�b�N�X
static int m_nGaplessState = GS_OK;	// �M���b�v���X�Đ��p�̃X�e�[�^�X
static int m_nPlayMode = PM_LIST;	// �v���C���[�h
static HINSTANCE m_hInst = NULL;	// �C���X�^���X
static NOTIFYICONDATA m_ni;	// �^�X�N�g���C�̃A�C�R���̃f�[�^
static BOOL m_nRepeatFlag = FALSE;	// ���s�[�g�����ǂ���
static BOOL m_bTrayFlag = FALSE;	// �^�X�N�g���C�ɓ����Ă邩�ǂ���
static TCHAR m_szTime[100];			// �Đ�����
static TCHAR m_szTag[MAX_FITTLE_PATH];	// �^�O
static TCHAR m_szTreePath[MAX_FITTLE_PATH];	// �c���[�̃p�X
static BOOL m_bFloat = FALSE;

static TAGINFO m_taginfo = {0};
typedef struct{
	CHAR szTitle[256];
	CHAR szArtist[256];
	CHAR szAlbum[256];
	CHAR szTrack[10];
}TAGINFOOLD;
static TAGINFOOLD m_taginfoold;

#ifdef UNICODE
static CHAR m_szTreePathA[MAX_FITTLE_PATH];	// �c���[�̃p�X
static CHAR m_szFilePathA[MAX_FITTLE_PATH];	// �c���[�̃p�X
#endif

static volatile BOOL m_bCueEnd = FALSE;
static UINT s_uTaskbarRestart = 0;	// �^�X�N�g���C�ĕ`�惁�b�Z�[�W�̂h�c

// �����o�ϐ��i�n���h���ҁj
static HWND m_hMainWnd = NULL;		// ���C���E�C���h�E�n���h��
 static HWND m_hRebar = NULL;		// ���o�[�n���h��
  static HWND m_hToolBar = NULL;	// �c�[���o�[�n���h��
  static HWND m_hSeek = NULL;		// �V�[�N�o�[�n���h��
  static HWND m_hVolume = NULL;		// �{�����[���o�[�n���h��
 static HWND m_hCombo = NULL;		// �R���{�{�b�N�X�n���h��
 static HWND m_hTree = NULL;			// �c���[�r���[�n���h��
 static HWND m_hTab = NULL;			// �^�u�R���g���[���n���h��
 static HWND m_hStatus = NULL;		// �X�e�[�^�X�o�[�n���h��
 static HWND m_hSliderTip = NULL;	// �X���C�_�p�c�[���`�b�v�n���h��
 static HWND m_hSplitter = NULL;	// �X�v���b�^�n���h��
static HWND m_hTitleTip = NULL;		// �^�C�g���p�c�[���`�b�v�n���h��

static HMENU m_hTrayMenu = NULL;	// �g���C���j���[�n���h��
static HIMAGELIST m_hDrgImg = NULL;	// �h���b�O�C���[�W
static HHOOK m_hHook = NULL;		// ���[�J���t�b�N�n���h��
static HFONT m_hBoldFont = NULL;	// �������t�H���g�n���h��
static HFONT m_hFont = NULL;		// �t�H���g�n���h��
static HTREEITEM m_hHitTree = NULL;	// �c���[�̃q�b�g�A�C�e��
static HMENU m_hMainMenu = NULL;	// ���C�����j���[�n���h��

// �N������
static int XARGC = 0;
static LPTSTR *XARGV = 0;

// �O���[�o���ϐ�
struct CONFIG g_cfg = {0};			// �ݒ�\����
CHANNELINFO g_cInfo[2] = {0};		// �`�����l�����
BOOL g_bNow = 0;				// �A�N�e�B�u�ȃ`�����l��0 or 1
struct FILEINFO *g_pNextFile = NULL;	// �Đ���

#include "oplugins.h"


// �^�u�֌W

int TabGetListSel(){
	return TabCtrl_GetCurSel(m_hTab);
}
int TabGetListFocus(){
	return TabCtrl_GetCurFocus(m_hTab);
}
void TabSetListFocus(int nIndex){
	TabCtrl_SetCurFocus(m_hTab, nIndex);
}
int TabGetListCount(){
	return TabCtrl_GetItemCount(m_hTab);
}
int TabGetRowCount(){
	return TabCtrl_GetRowCount(m_hTab);
}

static int TabHitTest(LONG lp, int flag){
	TCHITTESTINFO tchti;
	tchti.flags = flag & 0xf;
	tchti.pt.x = (short)LOWORD(lp);
	tchti.pt.y = (short)HIWORD(lp);
	if (flag & 0x40) ScreenToClient(m_hTab, &tchti.pt);
	return TabCtrl_HitTest(m_hTab, &tchti);
}

static void TabGetItemRect(int nItem, LPRECT pRect){
	TabCtrl_GetItemRect(m_hTab, nItem, pRect);
}

void TabAdjustRect(LPRECT pRect){
	TabCtrl_AdjustRect(m_hTab, FALSE, pRect);
}

// �Đ����̃��X�g�^�u�̃|�C���^���擾
static struct LISTTAB *GetPlayListTab(){
	return GetListTab(m_hTab, (m_nPlayTab!=-1 ? m_nPlayTab : TabGetListSel()));
}

// �I�𒆂̃��X�g�^�u�̃|�C���^���擾
static struct LISTTAB *GetSelListTab(){
	return GetListTab(m_hTab, TabGetListSel());
}

// �t�H���_�^�u�̃|�C���^���擾
static struct LISTTAB *GetFolderListTab(){
	return GetListTab(m_hTab, 0);
}

/* �{�����[���o�[�֌W */
static int GetVolumeBar() {
	return SendMessage(m_hVolume, TBM_GETPOS, 0, 0);
}

static void TreeSelect(HTREEITEM hNode) {
	TreeView_Select(m_hTree, hNode, TVGN_CARET);
}

static void TreeSelectDrive() {
	TreeSelect(MakeDriveNode(m_hCombo, m_hTree));
}

static void TreeGetNextItem() {
	m_hHitTree = TreeView_GetNextItem(m_hTree, TVI_ROOT, TVGN_CARET);
}

static HTREEITEM TreeGetSelection(){
	return TreeView_GetSelection(m_hTree);
}

static void TreeSetColor(){
	TreeView_SetBkColor(m_hTree, g_cfg.nBkColor);
	TreeView_SetTextColor(m_hTree, g_cfg.nTextColor);
}

static void SetVolumeBar(int nVol){
	SendMessage(m_hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)nVol);
}

static LRESULT SendFittleMessage(UINT uMsg, WPARAM wp, LPARAM lp){
	return SendMessage(m_hMainWnd, uMsg, wp, lp);
}

LRESULT SendF4b24Message(WPARAM wp, LPARAM lp){
	return SendFittleMessage(WM_F4B24_IPC, wp, lp);
}

static void PostF4b24Message(WPARAM wp, LPARAM lp){
	PostMessage(m_hMainWnd, WM_F4B24_IPC, wp, lp);
}

static LRESULT SendFittleCommand(int nCmd){
	return SendFittleMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0), 0);
}

static void PostFittleCommand(int nCmd){
	PostMessage(m_hMainWnd, WM_COMMAND, MAKEWPARAM(nCmd, 0), 0);
}

#ifndef _DEBUG
/*  ���Ɏ��s����Fittle�Ƀp�����[�^�𑗐M���� */
static void SendCopyData(int iType, LPTSTR lpszString){
#ifdef UNICODE
	CHAR nameA[MAX_FITTLE_PATH];
	LPBYTE lpWork;
	COPYDATASTRUCT cds;
	DWORD la, lw, cbData;
	lstrcpyntA(nameA, lpszString, MAX_FITTLE_PATH);
	la = lstrlenA(nameA) + 1;
	if (la & 1) la++; /* WORD align */
	lw = lstrlenW(lpszString) + 1;
	cbData = la * sizeof(CHAR) + lw * sizeof(WCHAR);
	lpWork = (LPBYTE)HZAlloc(cbData);
	if (!lpWork) return;
	lstrcpyA((LPSTR)(lpWork), nameA);
	lstrcpyW((LPWSTR)(lpWork + la), lpszString);
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpWork;
	cds.cbData = cbData;
	SendFittleMessage(WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
	HFree(lpWork);
#else
	COPYDATASTRUCT cds;
	cds.dwData = iType;
	cds.lpData = (LPVOID)lpszString;
	cds.cbData = (lstrlenA(lpszString) + 1) * sizeof(CHAR);
	// �����񑗐M
	SendFittleMessage(WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
#endif
}

// ���d�N���̖h�~
static LRESULT CheckMultiInstance(BOOL *pbMulti){
	HANDLE hMutex;
	int i;

	*pbMulti = FALSE;
	hMutex = CreateMutex(NULL, TRUE, TEXT("Mutex of Fittle"));

	if(GetLastError()==ERROR_ALREADY_EXISTS){
		*pbMulti = TRUE;

		m_hMainWnd = FindWindow(TEXT("Fittle"), NULL);
		// �R�}���h���C��������Ζ{�̂ɑ��M
		if(XARGC>1){
			// �R�}���h���C���I�v�V����
			if(XARGV[1][0] == '/'){
				if(!lstrcmpi(XARGV[1], TEXT("/play"))){
					return SendFittleCommand(IDM_PLAY);
				}else if(!lstrcmpi(XARGV[1], TEXT("/pause"))){
					return SendFittleCommand(IDM_PAUSE);
				}else if(!lstrcmpi(XARGV[1], TEXT("/stop"))){
					return SendFittleCommand(IDM_STOP);
				}else if(!lstrcmpi(XARGV[1], TEXT("/prev"))){
					return SendFittleCommand(IDM_PREV);
				}else if(!lstrcmpi(XARGV[1], TEXT("/next"))){
					return SendFittleCommand(IDM_NEXT);
				}else if(!lstrcmpi(XARGV[1], TEXT("/exit"))){
					return SendFittleCommand(IDM_END);
				}else if(!lstrcmpi(XARGV[1], TEXT("/add"))){
					for(i=2;i<XARGC;i++){
						SendCopyData(1, XARGV[i]);
					}
					if(XARGC>2) return 0;
				}else if(!lstrcmpi(XARGV[1], TEXT("/addplay"))){
					for(i=2;i<XARGC;i++){
						SendCopyData(1, XARGV[i]);
					}
					SendFittleCommand(IDM_PLAY);
					return 0;
				}
			}else{
				SendCopyData(0, XARGV[1]);
				return 0;
			}
		}
		// �~�j�p�l������������I���
		if(SendFittleMessage(WM_USER, 0, 0))	return 0;

		if(!IsWindowVisible(m_hMainWnd) || IsIconic(m_hMainWnd)){
			SendFittleCommand(IDM_TRAY_WINVIEW);
		}	
		SetForegroundWindow(m_hMainWnd);
		return 0;
	}
	return 0;
}
#endif

static HWND Initialze(HINSTANCE hCurInst, int nCmdShow){
	HWND hWnd;
	WNDCLASSEX wc;
	WINDOWPLACEMENT wpl = {0};
	TCHAR szClassName[] = TEXT("Fittle");	// �E�B���h�E�N���X

	DWORD dTime = GetTickCount();
	TCHAR szBuff[100];

	// BASS�̃o�[�W�����m�F
	if(HIWORD(BASS_GetVersion())!=BASSVERSION){
		MessageBox(NULL, TEXT("bass.dll�̃o�[�W�������m�F���Ă��������B"), TEXT("Fittle"), MB_OK);
		return 0;
	}

	// �R�����R���g���[���̏�����
	INITCOMMONCONTROLSEX ic;
	ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ic.dwICC = ICC_USEREX_CLASSES | ICC_TREEVIEW_CLASSES
			 | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES
			 | ICC_COOL_CLASSES | ICC_TAB_CLASSES;
	InitCommonControlsEx(&ic);

	// �E�B���h�E�E�N���X�̓o�^
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_DBLCLKS; //CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc; //�v���V�[�W����
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInst = hCurInst; //�C���X�^���X
	wc.hIcon = (HICON)LoadImage(hCurInst, TEXT("MYICON"), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hIconSm = (HICON)LoadImage(hCurInst, TEXT("MYICON"), IMAGE_ICON, 16, 16, LR_SHARED);
	wc.hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,	0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = TEXT("MAINMENU");	//���j���[��
	wc.lpszClassName = (LPCTSTR)szClassName;
	if(!RegisterClassEx(&wc)) return 0;

	// INI�t�@�C���̈ʒu���擾
	WASetIniFile(NULL, "Fittle.ini");

	LoadState();
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
	if(nCmdShow==SW_SHOWNORMAL && WAGetIniInt("Main", "Maximized", 0)){
		nCmdShow = SW_SHOWMAXIMIZED;			// �ő剻���
		//wpl.flags = WPF_RESTORETOMAXIMIZED;		// �ŏ������ꂽ�ő剻���
	}

	// �E�B���h�E�̈ʒu��ݒ�
	UpdateWindow(hWnd);
	wpl.length = sizeof(WINDOWPLACEMENT);
	wpl.showCmd = SW_HIDE;
	wpl.rcNormalPosition.left = WAGetIniInt("Main", "Left", (GetSystemMetrics(SM_CXSCREEN)-FITTLE_WIDTH)/2);
	wpl.rcNormalPosition.top = WAGetIniInt("Main", "Top", (GetSystemMetrics(SM_CYSCREEN)-FITTLE_HEIGHT)/2);
	wpl.rcNormalPosition.right = wpl.rcNormalPosition.left + WAGetIniInt("Main", "Width", FITTLE_WIDTH);
	wpl.rcNormalPosition.bottom = wpl.rcNormalPosition.top + WAGetIniInt("Main", "Height", FITTLE_HEIGHT);
	SetWindowPlacement(hWnd, &wpl);

	// �~�j�p�l���̕���
	if(g_cfg.nMiniPanelEnd){
		if(nCmdShow!=SW_SHOWNORMAL)
			ShowWindow(hWnd, nCmdShow);	// �ő剻���̃E�B���h�E��Ԃ�K��
		PostFittleCommand(IDM_MINIPANEL);
	}else{
		ShowWindow(hWnd, nCmdShow);	// �\��
	}

	GetWindowText(m_hStatus, szBuff, 100);
	if(lstrlen(szBuff)==0){
		wsprintf(szBuff, TEXT("Startup time: %d ms\n"), GetTickCount() - dTime);
		SetWindowText(m_hStatus, szBuff);
	}
	return hWnd;
}

int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE /*hPrevInst*/, LPSTR /*lpsCmdLine*/, int nCmdShow){
	HWND hWnd;
	MSG msg;
	HACCEL hAccel;
	HMODULE XARGD;

#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	WAIsUnicode();
	XARGD = ExpandArgs(&XARGC, &XARGV);
	if (!LoadBASS()) {
		MessageBox(NULL, TEXT("bass.dll�̓ǂݍ��݂Ɏ��s���܂����B"), TEXT("Fittle"), MB_OK);
		return 0;
	}

#ifndef _DEBUG
	{
		BOOL bMulti = FALSE;
		LRESULT lRet = CheckMultiInstance(&bMulti);
		// ���d�N���̖h�~
		if (bMulti) return lRet;
	}
#endif

	// ������
	hWnd = Initialze(hCurInst, nCmdShow);
	if (!hWnd) return 0;

	// �A�N�Z�����[�^�[�L�[���擾
	hAccel = LoadAccelerators(hCurInst, TEXT("MYACCEL"));

	// ���b�Z�[�W���[�v
	while(GetMessage(&msg, NULL, 0, 0) > 0){
		if(!TranslateAccelerator(hWnd, hAccel, &msg)){	// �ʏ���A�N�Z�����[�^�L�[�̗D��x���������Ă܂�
			HWND m_hMiniPanel = (HWND)SendFittleMessage(WM_FITTLE, GET_MINIPANEL, 0);
			if(!m_hMiniPanel || !IsDialogMessage(m_hMiniPanel, &msg)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

#ifdef UNICODE
	if (XARGV) GlobalFree(XARGV);
#elif defined(_MSC_VER)
#else
	if (XARGD) FreeLibrary(XARGD);
#endif
	ClearTypelist();
	FreeOutputPlugins();

	return (int)msg.wParam;
}

#if 1
#define TIMESTART
#define TIMECHECK(m) 
#else
#define TIMESTART \
	DWORD dStartTime; \
	dStartTime = GetTickCount();

#define TIMECHECK(m) { \
	TCHAR buf[128]; \
	wsprintf(buf, TEXT(m) TEXT(": %d ms\n"), GetTickCount() - dStartTime); \
	OutputDebugString(buf); \
}
#endif

#ifdef UNICODE
/* �T�u�N���X���ɂ�镶�������΍� */
static WNDPROC mUniWndProcOld = 0;
static LRESULT CALLBACK UniWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	/* ANSI�ϊ���}�~���� */
	if (msg == WM_SETTEXT || msg == WM_GETTEXT)
		return DefWindowProcW(hWnd, msg, wp, lp);
	return CallWindowProc(mUniWndProcOld, hWnd, msg, wp, lp);
}
static void UniFix(HWND hWnd){
	//if (!IsWindowUnicode(hWnd))
	mUniWndProcOld = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)UniWndProc);
}
#endif

static void SetFolder(LPCTSTR lpszFolderPath){
	if(g_cfg.nTreeState==TREE_SHOW){
		MakeTreeFromPath(m_hTree, m_hCombo, lpszFolderPath);
	}else{
		lstrcpyn(m_szTreePath, lpszFolderPath, MAX_FITTLE_PATH);
		SendFittleCommand(IDM_FILEREVIEW);
	}
}

static int WAGetIniIntFmt(LPCSTR pSec, LPCSTR pKeyFormat, int nKeyParam, int nDefault)
{
	CHAR szKey[10];
	wsprintfA(szKey, pKeyFormat, nKeyParam);
	return WAGetIniInt(pSec, szKey, nDefault);
}
static void WASetIniIntFmt(LPCSTR pSec, LPCSTR pKeyFormat, int nKeyParam, int iValue)
{
	CHAR szKey[10];
	wsprintfA(szKey, pKeyFormat, nKeyParam);
	WASetIniInt(pSec, szKey, iValue);
}

static void OnCreate(HWND hWnd){
	int nDevice;
	MENUITEMINFO mii;
	int i;
	WASTR temppath;
	TCHAR szLastPath[MAX_FITTLE_PATH];
	HWND hComboEx;

	TIMESTART

	nDevice = InitOutputPlugin(hWnd);

	TIMECHECK("�o�̓v���O�C��������")

	// BASS������
	if(!BASS_Init(nDevice, OPGetRate(), 0, hWnd, NULL)){
		MessageBox(hWnd, TEXT("BASS�̏������Ɏ��s���܂����B"), TEXT("Fittle"), MB_OK);
		BASS_Free();
		ExitProcess(1);
	}

	TIMECHECK("BASS������")

	m_bFloat = (g_cfg.nOut32bit && OPIsSupportFloatOutput()) ? TRUE : FALSE;
	g_cInfo[0].sGain = 1;
	g_cInfo[1].sGain = 1;

	TIMECHECK("�o�͌`���m�F")

	// �D��x
	if(g_cfg.nHighTask){
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	}

	TIMECHECK("�D��x�ݒ�")

	// ���Ƀv���O�C��������
	InitArchive(hWnd);
	TIMECHECK("���Ƀv���O�C��������")

	// �����g���q�̌���
	InitFileTypes();
	TIMECHECK("�����g���q�̌���")

	// ��ʃv���O�C��������
	InitGeneralPlugins(hWnd);
	TIMECHECK("��ʃv���O�C��������")

	// ���X�g�v���O�C��������
	LoadColumnsOrder();
	TIMECHECK("���X�g�v���O�C��������")

	// �^�X�N�g���C�ĕ`��̃��b�Z�[�W��ۑ�
	s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));

	TIMECHECK("�^�X�N�g���C�ĕ`��̃��b�Z�[�W��ۑ�")

	// ���݂̃v���Z�XID�ŗ����W�F�l���[�^��������
	init_genrand((unsigned int)(GetTickCount()+GetCurrentProcessId()));

	TIMECHECK("RNG������")

	// ���j���[�n���h����ۑ�
	m_hMainMenu = GetMenu(hWnd);

	TIMECHECK("���j���[�n���h����ۑ�")

	TIMECHECK("InitCommonControlsEx")

	// �X�e�[�^�X�o�[�쐬
	m_hStatus = CreateStatusWindow(WS_CHILD | /*WS_VISIBLE |*/ SBARS_SIZEGRIP | CCS_BOTTOM | SBT_TOOLTIPS, TEXT(""), hWnd, ID_STATUS);
	if(g_cfg.nShowStatus){
		g_cfg.nShowStatus = 0;
		PostFittleCommand(IDM_SHOWSTATUS);
	}

	TIMECHECK("�X�e�[�^�X�o�[�쐬")

	// �R���{�{�b�N�X�쐬
	m_hCombo = CreateWindowEx(0,
		WC_COMBOBOXEX,
		NULL,
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,	// CBS_DROPDOWNLIST
		0, 0, 0, 200,
		hWnd,
		(HMENU)ID_COMBO,
		m_hInst,
		NULL);

	TIMECHECK("�R���{�{�b�N�X�쐬")

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
	TreeSetColor();
	DragAcceptFiles(m_hTree, TRUE);

	TIMECHECK("�c���[�쐬")

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

	TIMECHECK("�^�u�쐬")

	MakeNewTab(m_hTab, TEXT("�t�H���_"), 0);

	TIMECHECK("�t�H���_�^�u�쐬")

	// �v���C���X�g���X�g�ǂݍ���
	LoadPlaylists(m_hTab);

	TIMECHECK("�v���C���X�g���X�g�ǂݍ���")

	//�X�v���b�g�E�B���h�E���쐬
	m_hSplitter = CreateWindow(TEXT("static"),
		NULL,
		SS_NOTIFY | WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		hWnd,
		NULL,
		m_hInst,
		NULL
	);

	TIMECHECK("�X�v���b�^�[�쐬")

	//���o�[�R���g���[���̍쐬
	REBARINFO rbi;
	m_hRebar = CreateWindowEx(WS_EX_TOOLWINDOW,
		REBARCLASSNAME,
		NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER | /*WS_CLIPSIBLINGS | */WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_AUTOSIZE | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE,
		0, 0, 0, 0,
		hWnd, (HMENU)ID_REBAR, m_hInst, NULL);

	ZeroMemory(&rbi, sizeof(REBARINFO));
	rbi.cbSize = sizeof(REBARINFO);
	rbi.fMask = 0;
	rbi.himl = 0;
	PostMessage(m_hRebar, RB_SETBARINFO, 0, (LPARAM)&rbi);

	TIMECHECK("���o�[�쐬")

	//�c�[���`�b�v�̍쐬
	m_hTitleTip = CreateToolTip(NULL);
	m_hSliderTip = CreateToolTip(hWnd);

	TIMECHECK("�c�[���`�b�v�쐬")

	// �c�[���o�[�̍쐬
	m_hToolBar = CreateToolBar();
	TIMECHECK("�c�[���o�[�쐬")

	//�{�����[���o�[�̍쐬
	m_hVolume = CreateTrackBar(ID_VOLUME, WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_BOTH | TBS_TOOLTIPS | TBS_FIXEDLENGTH | TBS_TRANSPARENTBKGND);
	TIMECHECK("�{�����[���o�[�쐬")

	//�V�[�N�o�[�̍쐬
	m_hSeek = CreateTrackBar(TD_SEEKBAR, WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_BOTTOM | WS_DISABLED | TBS_FIXEDLENGTH | TBS_TRANSPARENTBKGND);
	TIMECHECK("�V�[�N�o�[�쐬")

	// ���o�[�R���g���[���̕���
	REBARBANDINFO rbbi;
	ZeroMemory(&rbbi, sizeof(REBARBANDINFO));
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask  = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_ID;
	rbbi.cyMinChild = 22;
	rbbi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
	for(i=0;i<BAND_COUNT;i++){
		rbbi.cx = WAGetIniIntFmt("Rebar2", "cx%d", i, 0);
		rbbi.wID = WAGetIniIntFmt("Rebar2", "wID%d", i, i);
		rbbi.fStyle = WAGetIniIntFmt("Rebar2", "fStyle%d", i, (RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS)); //RBBS_BREAK
		
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
		SendMessage(m_hRebar, RB_INSERTBAND, (WPARAM)i, (LPARAM)&rbbi);

		// ��\�����̏���
		if(!(rbbi.fStyle & RBBS_HIDDEN)){
			CheckMenuItem(GetMenu(hWnd), IDM_SHOWMAIN+i, MF_BYCOMMAND | MF_CHECKED);
		}else{
			if(rbbi.wID==0)	ShowWindow(m_hToolBar, SW_HIDE);	// �c�[���o�[���\������Ă��܂��o�O�΍�
		}
	}

	TIMECHECK("���o�[��ԕ���")

	// ���j���[�`�F�b�N
	if(rbbi.fStyle & RBBS_NOGRIPPER){
		CheckMenuItem(GetMenu(hWnd), IDM_FIXBAR, MF_BYCOMMAND | MF_CHECKED);
	}

	if(g_cfg.nTreeState==TREE_SHOW){
		CheckMenuItem(GetMenu(hWnd), IDM_TOGGLETREE, MF_BYCOMMAND | MF_CHECKED);
	}

	TIMECHECK("���j���[��ԕ���")

	//�R���g���[���̃T�u�N���X��
	hComboEx = GetWindow(m_hCombo, GW_CHILD);
	SubClassControl(m_hSeek, NewSliderProc);
	SubClassControl(m_hVolume, NewSliderProc);
	SubClassControl(m_hTab, NewTabProc);
	SubClassControl(hComboEx, NewComboProc);
	SubClassControl(GetWindow(hComboEx, GW_CHILD), NewComboEditProc);
	SubClassControl(m_hTree, NewTreeProc);
	SubClassControl(m_hSplitter, NewSplitBarProc);

	TIMECHECK("�R���g���[���̃T�u�N���X��")

	// TREE_INIT
	InitTreeIconIndex(m_hCombo, m_hTree, g_cfg.nTreeIcon != 0);

	TIMECHECK("�c���[�A�C�R���̏�����")

	// �V�X�e�����j���[�̕ύX
	m_hTrayMenu = GetSubMenu(LoadMenu(m_hInst, TEXT("TRAYMENU")), 0);
	ZeroMemory(&mii, sizeof(mii));
	mii.fMask = MIIM_TYPE;
	mii.cbSize = sizeof(mii);
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(GetSystemMenu(hWnd, FALSE), 7, FALSE, &mii);

	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = TEXT("���j���[�̕\��(&V)\tCtrl+M");
	mii.wID = IDM_TOGGLEMENU;
	InsertMenuItem(GetSystemMenu(hWnd, FALSE), 8, FALSE, &mii);

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = TEXT("&Fittle");
	mii.hSubMenu = m_hTrayMenu;
	InsertMenuItem(GetSystemMenu(hWnd, FALSE), 9, FALSE, &mii);

	DrawMenuBar(hWnd);

	TIMECHECK("���j���[��ԕ���2")

	// �v���C���[�h��ݒ肷��
	ControlPlayMode(GetMenu(hWnd), WAGetIniInt("Main", "Mode", PM_LIST));
	m_nRepeatFlag = WAGetIniInt("Main", "Repeat", TRUE);
	if(m_nRepeatFlag){
		m_nRepeatFlag = FALSE;
		ToggleRepeatMode(GetMenu(hWnd));
	}

	// �E�B���h�E�^�C�g���̐ݒ�
	SetWindowText(hWnd, FITTLE_TITLE);
	lstrcpy(m_ni.szTip, FITTLE_TITLE);

	// �^�X�N�g���C
	if(g_cfg.nTrayOpt==2) SetTaskTray(hWnd);

	// �{�����[���̐ݒ�
	PostMessage(m_hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)WAGetIniInt("Main", "Volumes", SLIDER_DIVIDED));

	TIMECHECK("���̑���Ԃ̕���")

	// �O���[�o���z�b�g�L�[
	RegHotKey(hWnd);

	TIMECHECK("�O���[�o���z�b�g�L�[")

	// ���[�J���t�b�N
	m_hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)MyHookProc, m_hInst, GetWindowThreadProcessId(hWnd, NULL));

	TIMECHECK("���[�J���t�b�N")

	// �N���e�B�J���Z�N�V�����̍쐬
	InitializeCriticalSection(&m_cs);

	TIMECHECK("�N���e�B�J���Z�N�V�����̍쐬")

	// ���[�U�C���^�[�t�F�C�X
	SetUIFont();

	TIMECHECK("�O�ς̏�����")

	// ���j���[�̔�\��
	if(!WAGetIniInt("Main", "MainMenu", 1))
		PostFittleCommand(IDM_TOGGLEMENU);

	TIMECHECK("���j���[�̔�\��")

	// �R���{�{�b�N�X�̏�����
	SendF4b24Message((WPARAM)WM_F4B24_IPC_UPDATE_DRIVELIST, (LPARAM)0);
	//SetDrivesToCombo(m_hCombo);

	TIMECHECK("�R���{�{�b�N�X�̏�����")

	// �R�}���h���C���̏���
	BOOL bCmd;
	bCmd = FALSE;
	for(i=1;i<XARGC;i++){
		if(bCmd) break;
		switch(GetParentDir(XARGV[i], szLastPath)){
			case FOLDERS: // �t�H���_
			case LISTS:   // ���X�g
			case ARCHIVES:// �A�[�J�C�u
				SetFolder(szLastPath);
				bCmd = TRUE;
				break;

			case FILES: // �t�@�C��
				SetFolder(szLastPath);
				GetLongPathName(XARGV[i], szLastPath, MAX_FITTLE_PATH); // 98�ȍ~
				SelectFolderItem(szLastPath);
				bCmd = TRUE;
				break;
		}
	}

	TIMECHECK("�R�}���h���C���̏���")

	// Fittle�v���O�C��������
	InitFittlePlugins(hWnd);

	TIMECHECK("Fittle�v���O�C��������")

#ifdef UNICODE
	/* �T�u�N���X���ɂ�镶�������΍� */
	UniFix(hWnd);

	TIMECHECK("�T�u�N���X���ɂ�镶�������΍�")
#endif

	WAGetModuleParentDir(NULL, &temppath);
	WAstrcpyt(szLastPath, &temppath, MAX_FITTLE_PATH);

	TIMECHECK("EXE�p�X�̎擾")

	if(bCmd){
		PostFittleCommand(IDM_PLAY);
	}else{
		struct LISTTAB *pCurList;
		// �R�}���h���C���Ȃ�
		// �����Oor���݂��Ȃ��Ȃ�Ō�̃p�X�ɂ���
		if(WAstrlen(&g_cfg.szStartPath)==0 || !WAFILE_EXIST(&g_cfg.szStartPath)){
			WAGetIniStr("Main", "LastPath", &temppath);
			WAstrcpyt(szLastPath, &temppath, MAX_FITTLE_PATH);
		}else{
			WAstrcpyt(szLastPath, &g_cfg.szStartPath, MAX_FITTLE_PATH);
		}
		if(FILE_EXIST(szLastPath)){
			SetFolder(szLastPath);
			TIMECHECK("�t�H���_�W�J")
		}else{
			TreeSelectDrive();
		}
		// �^�u�̕���
		TabSetListFocus(m_nPlayTab = WAGetIniInt("Main", "TabIndex", 0));

		TIMECHECK("�^�u�̕���")
		pCurList = GetSelListTab();

		// �Ō�ɍĐ����Ă����t�@�C�����Đ�
		if(g_cfg.nResume){
			if(m_nPlayMode==PM_RANDOM && !g_cfg.nResPosFlag){
				PostFittleCommand(IDM_NEXT);
			}else{
				WAstrcpyt(szLastPath, &g_cfg.szLastFile, MAX_FITTLE_PATH);
				SelectListItem(pCurList, szLastPath);
				PostFittleCommand(IDM_PLAY);
				// �|�W�V����������
				PostF4b24Message(WM_F4B24_INTERNAL_RESTORE_POSITION, 0);
			}
		}else if (g_cfg.nSelLastPlayed) {
			WAstrcpyt(szLastPath, &g_cfg.szLastFile, MAX_FITTLE_PATH);
			SelectListItem(pCurList, szLastPath);
			if(GetKeyState(VK_SHIFT) < 0){
				// Shift�L�[��������Ă�����Đ�
				PostFittleCommand(IDM_PLAY);
			}
		}else{
			if(GetKeyState(VK_SHIFT) < 0){
				// Shift�L�[��������Ă�����Đ�
				PostFittleCommand(IDM_NEXT);
			}
		}
		TIMECHECK("���W���[��")
	}
	TIMECHECK("�E�C���h�E�쐬�I��")
}

// �O���c�[���̎��s
static void ToolExecute(HWND hWnd, LPTSTR pszFilePathes, BOOL bTool){
	LPWASTR pExePath = bTool ? &g_cfg.szToolPath : &g_cfg.szFilerPath;
	TCHAR szToolPath[MAX_FITTLE_PATH];
	if(!WAstrlen(pExePath)){
		if (bTool){
			SendF4b24Message(WM_F4B24_IPC_SETTING, WM_F4B24_IPC_SETTING_LP_PATH);
			return;
		}
		lstrcpy(szToolPath, TEXT("explorer.exe"));
	}else{
		WAstrcpyt(szToolPath, pExePath, MAX_FITTLE_PATH);
		if (szToolPath[0] != TEXT('\"')){
			PathQuoteSpaces(szToolPath);
		}
	}
	MyShellExecute(hWnd, szToolPath, pszFilePathes);
}

// �E�B���h�E�v���V�[�W��
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	static int s_nHitTab = -1;				// �^�u�̃q�b�g�A�C�e��
	static int s_nClickState = 0;			// �^�X�N�g���C�A�C�R���̃N���b�N���
	static BOOL s_bReviewAllow = TRUE;		// �c���[�̃Z���ω��Ō����������邩

	int i;
	TCHAR szLabel[MAX_FITTLE_PATH];
	RECT rcTab;

	LRESULT lHookRet;

	if (OnWndProcPlugins(hWnd, msg, wp, lp, &lHookRet)) return lHookRet;

	switch(msg)
	{
		case WM_USER:
			return (LRESULT)(HWND)SendFittleMessage(WM_FITTLE, GET_MINIPANEL, 0);

		case WM_USER+1:
			OnChangeTrack();
			OPResetOnPlayNextBySystem();
			break;

		//case WM_SYSCOLORCHANGE:

		//	SendMessage(m_hSeek, msg, wp, lp);
		//	SendMessage(m_hVolume, msg, wp, lp);
		//	SendMessage(m_hTab, msg, wp, lp);
		//	SendMessage(m_hTree, msg, wp, lp);
		//	SendMessage(m_hSplitter, msg, wp, lp);

		//	break;

		case WM_CREATE:

			m_hMainWnd = hWnd;
			OnCreate(hWnd);

			break;

		case WM_SETFOCUS:
			{
				struct LISTTAB *pCurList = GetSelListTab();
				if(pCurList) SetFocus(pCurList->hList);
			}
			break;

		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			if(OPGetStatus() != OUTPUT_PLUGIN_STATUS_STOP){
				g_cfg.nResPos = (int)TrackGetPos();
			}else{
				g_cfg.nResPos = 0;
			}
			StopOutputStream();
			OPTerm();

			// �N���e�B�J���Z�N�V�������폜
			DeleteCriticalSection(&m_cs);

			SaveState(hWnd);
			SaveColumnsOrder(GetSelListTab()->hList);
			SavePlaylists(m_hTab);

			if(m_bTrayFlag){
				Shell_NotifyIcon(NIM_DELETE, &m_ni);	// �^�X�N�g���C�ɓ����Ă������̏���
				m_bTrayFlag = FALSE;
			}

			QuitPlugins();	// �v���O�C���Ăяo��

			UnRegHotKey(hWnd);
			UnhookWindowsHookEx(m_hHook);
			BASS_Free(); // Bass�̊J��
			
			PostQuitMessage(0);
			break;
			
		case WM_SIZE:
			{
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
				SendMessage(m_hRebar, WM_SIZE, wp, lp);

				nRebarHeight = (int)SendMessage(m_hRebar, RB_GETBARHEIGHT, 0, 0); 
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

				hdwp = DeferWindowPos(hdwp, m_hSplitter, HWND_TOP,
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
				if(!(g_cfg.nTabHide && TabGetListCount()==1)){
					if(rcTab.left<rcTab.right){
						TabAdjustRect(&rcTab);
					}
				}
				hdwp = DeferWindowPos(hdwp, GetSelListTab()->hList, HWND_TOP,
					rcTab.left,
					rcTab.top,
					rcTab.right - rcTab.left,
					rcTab.bottom - rcTab.top,
					SWP_SHOWWINDOW | SWP_NOZORDER);

				EndDeferWindowPos(hdwp);
			}
			break;

		case WM_COPYDATA: // �������M
			{
				COPYDATASTRUCT *pcds= (COPYDATASTRUCT *)lp;
#ifdef UNICODE
				WCHAR wszRecievedPath[MAX_FITTLE_PATH];
				LPTSTR pRecieved;
				size_t la = lstrlenA((LPSTR)pcds->lpData) + 1;
				lstrcpynAt(wszRecievedPath, (LPSTR)pcds->lpData, MAX_FITTLE_PATH);
				pRecieved = wszRecievedPath;
				if (la & 1) la++; /* WORD align */
				if (la < pcds->cbData){
					LPWSTR pw = (LPWSTR)(((LPBYTE)pcds->lpData) + la);
					size_t lw = lstrlenW(pw) + 1;
					//if (la * sizeof(CHAR) + lw * sizeof(WCHAR)==pcds->cbData)
					lstrcpynW(wszRecievedPath, pw, (lw > MAX_FITTLE_PATH) ? MAX_FITTLE_PATH : lw);
				}
#else
				LPTSTR pRecieved = (LPTSTR)pcds->lpData;
#endif
				if(pcds->dwData==0){
					TCHAR szParPath[MAX_FITTLE_PATH];
					switch(GetParentDir(pRecieved, szParPath)){
						case FOLDERS:
						case LISTS:
						case ARCHIVES:
							MakeTreeFromPath(m_hTree, m_hCombo, szParPath);
							m_nPlayTab = 0;
							SendFittleCommand(IDM_NEXT);
							return TRUE;

						case FILES:
							MakeTreeFromPath(m_hTree, m_hCombo, szParPath);
							GetLongPathName(pRecieved, szParPath, MAX_FITTLE_PATH); // 98�ȍ~
							SelectFolderItem(szParPath);
							PostFittleCommand(IDM_PLAY);
							return TRUE;
					}
				}else{
					struct FILEINFO *pSub = NULL;
					TCHAR szTest[MAX_FITTLE_PATH];
					if (IsURLPath(pRecieved))
						lstrcpyn(szTest, pRecieved, MAX_FITTLE_PATH);
					else
						GetLongPathName(pRecieved, szTest, MAX_FITTLE_PATH);
					SearchFiles(&pSub, szTest, TRUE);
					AppendToList(GetSelListTab(), pSub);
					return TRUE;
				}
			}

			break;

		case WM_COMMAND:
			TCHAR szNowDir[MAX_FITTLE_PATH + 2];
			switch(LOWORD(wp))
			{
				case IDM_REVIEW: //�ŐV�̏��ɍX�V
					lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					MakeTreeFromPath(m_hTree, m_hCombo, szNowDir); 
					break;

				case IDM_FILEREVIEW:
					if(s_bReviewAllow) {
						struct LISTTAB *pFolderTab = GetFolderListTab();
						pFolderTab->nPlaying = -1;
						pFolderTab->nStkPtr = -1;
						DeleteAllList(&(pFolderTab->pRoot));
						lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
						if(SearchFiles(&(pFolderTab->pRoot), szNowDir, GetKeyState(VK_CONTROL) < 0)!=LISTS){
							MergeSort(&(pFolderTab->pRoot), pFolderTab->nSortState);
						}
						TraverseList(pFolderTab);
						TabSetListFocus(0);
					}
					break;

				case IDM_SAVE: //�v���C���X�g�ɕۑ�
					s_nHitTab = TabGetListSel();
				case IDM_TAB_SAVE:
					{
						int nRet;
						struct LISTTAB *pHitList;
						lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
						GetFolderPart(szNowDir);
						pHitList = GetListTab(m_hTab, s_nHitTab);
						nRet = SaveM3UDialog(szNowDir, pHitList->szTitle);
						if(nRet){
							WriteM3UFile(pHitList->pRoot, szNowDir, nRet);
						}
						s_nHitTab = NULL;
					}
					break;

				case IDM_END: //�I��
					SendFittleMessage(WM_CLOSE, 0, 0);
					break;

				case IDM_PLAY: //�Đ�
					{
						struct LISTTAB *pCurList = GetSelListTab();
						int nLBIndex = LV_GetNextSelect(pCurList->hList, -1);
						if(OPGetStatus() == OUTPUT_PLUGIN_STATUS_PAUSE && nLBIndex==pCurList->nPlaying)
						{
							FilePause();	// �|�[�Y���Ȃ�ĊJ
						}else{ // ����ȊO�Ȃ�I���t�@�C�����Đ�
							if(nLBIndex!=-1){
								m_nPlayTab = TabGetListSel();
								PlayByUser(hWnd, GetPtrFromIndex(pCurList->pRoot, nLBIndex));
							}
						}
					}
					break;

				case IDM_PLAYPAUSE:
 					switch(OPGetStatus()){
					case OUTPUT_PLUGIN_STATUS_PLAY:
					case OUTPUT_PLUGIN_STATUS_PAUSE:
						SendFittleCommand(IDM_PAUSE);
						break;
					case OUTPUT_PLUGIN_STATUS_STOP:
						SendFittleCommand(IDM_PLAY);
						break;
					}
					break;

				case IDM_PAUSE: //�ꎞ��~
					FilePause();
					break;

				case IDM_STOP: //��~
					StopOutputStream();
					break;

				case IDM_PREV: //�O�̋�
					{
						struct FILEINFO *pPrev;
						int nPrev;
						struct LISTTAB *pPlayList = GetPlayListTab();

						switch(m_nPlayMode){
							case PM_LIST:
							case PM_SINGLE:
								if(pPlayList->nPlaying<=0){
									nPrev = LV_GetCount(pPlayList->hList)-1;
								}else{
									nPrev = pPlayList->nPlaying-1;
								}
								pPrev = GetPtrFromIndex(pPlayList->pRoot, nPrev);
								break;

							case PM_RANDOM:
								if(GetStackPtr(pPlayList)<=0){
									pPrev = NULL;
								}else{
									PopPrevious(pPlayList);	// ���ݍĐ����̋Ȃ��폜
									pPrev = PopPrevious(pPlayList);
								}
								break;

							default:
								pPrev = NULL;
								break;
						}
						if(pPrev){
							PlayByUser(hWnd, pPrev);
						}
					}
					break;

				case IDM_NEXT: //���̋�
					{
						struct FILEINFO *pNext = SelectNextFile(FALSE);
						if(pNext){
							PlayByUser(hWnd, pNext);
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
					SetVolumeBar(GetVolumeBar() + g_cfg.nVolAmount * 10);
					break;

				case IDM_VOLDOWN: //���ʂ�������
					SetVolumeBar(GetVolumeBar() - g_cfg.nVolAmount * 10);
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
					{
						struct LISTTAB *pFolderTab = GetFolderListTab();
						pFolderTab->nPlaying = -1;
						pFolderTab->nStkPtr = -1;
						DeleteAllList(&(pFolderTab->pRoot));
						lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
						SetCursor(LoadCursor(NULL, IDC_WAIT));  // �����v�J�[�\���ɂ���
						if (PathIsDirectory(szNowDir)){
							SearchFolder(&(pFolderTab->pRoot), szNowDir, TRUE);
							MergeSort(&(pFolderTab->pRoot), pFolderTab->nSortState);
						}else if(IsPlayListFast(szNowDir)){
							ReadM3UFile(&(pFolderTab->pRoot), szNowDir);
						}else if(IsArchiveFast(szNowDir)){
							ReadArchive(&(pFolderTab->pRoot), szNowDir);
						}else{
						}
						TraverseList(pFolderTab);
						SetCursor(LoadCursor(NULL, IDC_ARROW)); // ���J�[�\���ɖ߂�
						TabSetListFocus(0);
						break;
					}

				case IDM_FIND:
					{
						struct LISTTAB *pCurList = GetSelListTab();
						int nFileIndex = (int)DialogBoxParam(m_hInst, TEXT("FIND_DIALOG"), hWnd, FindDlgProc, (LPARAM)pCurList->pRoot);
						if(nFileIndex!=-1){
							LV_SingleSelectView(pCurList->hList, nFileIndex);
							SendFittleCommand(IDM_PLAY);
						}
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
					{
						REBARBANDINFO rbbi;
						ZeroMemory(&rbbi, sizeof(rbbi));
						rbbi.cbSize = sizeof(rbbi);
						rbbi.fMask  = RBBIM_STYLE;
						SendMessage(m_hRebar, RB_GETBANDINFO, (WPARAM)SendMessage(m_hRebar, RB_IDTOINDEX, (WPARAM)(LOWORD(wp)-IDM_SHOWMAIN), 0), (LPARAM)&rbbi);
						if(rbbi.fStyle & RBBS_HIDDEN){
							rbbi.fStyle &= ~RBBS_HIDDEN;
							CheckMenuItem(m_hMainMenu, LOWORD(wp), MF_CHECKED);
						}else{
							rbbi.fStyle |= RBBS_HIDDEN;
							CheckMenuItem(m_hMainMenu, LOWORD(wp), MF_UNCHECKED);
						}
						SendMessage(m_hRebar, RB_SETBANDINFO, (WPARAM)SendMessage(m_hRebar, RB_IDTOINDEX, (WPARAM)(LOWORD(wp)-IDM_SHOWMAIN), 0), (LPARAM)&rbbi);
					}
					break;

				case IDM_FIXBAR:
					{
						REBARBANDINFO rbbi;
						ZeroMemory(&rbbi, sizeof(rbbi));
						rbbi.cbSize = sizeof(rbbi);
						rbbi.fMask  = RBBIM_STYLE | RBBIM_SIZE;
						rbbi.fStyle = 0;
						for(i=0;i<BAND_COUNT;i++){
							SendMessage(m_hRebar, RB_GETBANDINFO, i, (LPARAM)&rbbi);
							if(rbbi.fStyle & RBBS_NOGRIPPER){
								rbbi.fStyle |= RBBS_GRIPPERALWAYS;
								rbbi.fStyle &= ~RBBS_NOGRIPPER;
								rbbi.cx += 10;
							}else{
								rbbi.fStyle |= RBBS_NOGRIPPER;
								rbbi.fStyle &= ~RBBS_GRIPPERALWAYS;
								rbbi.cx -= 10;
							}
							SendMessage(m_hRebar, RB_SETBANDINFO, i, (LPARAM)&rbbi);
						}
						CheckMenuItem(m_hMainMenu, IDM_FIXBAR, MF_BYCOMMAND | (rbbi.fStyle & RBBS_NOGRIPPER)?MF_CHECKED:MF_UNCHECKED);
					}
					break;

				case IDM_TOGGLETREE:
					switch(g_cfg.nTreeState){
						case TREE_UNSHOWN:
						case TREE_HIDE:
							s_bReviewAllow = FALSE;
							MakeTreeFromPath(m_hTree, m_hCombo, m_szTreePath);
							s_bReviewAllow = TRUE;
							CheckMenuItem(m_hMainMenu, IDM_TOGGLETREE, MF_BYCOMMAND | MF_CHECKED);
							g_cfg.nTreeState = TREE_SHOW;
							break;

						case TREE_SHOW:
							CheckMenuItem(m_hMainMenu, IDM_TOGGLETREE, MF_BYCOMMAND | MF_UNCHECKED);
							g_cfg.nTreeState = TREE_HIDE;
							SetFocus(GetSelListTab()->hList);
							break;
					}
					UpdateWindowSize(hWnd);
					break;

				case IDM_SETTING:
					SendF4b24Message(WM_F4B24_IPC_SETTING, WM_F4B24_IPC_SETTING_LP_GENERAL);
					break;


				case IDM_BM_PLAYING:
					{
						LPTSTR lpszPlayingPath = g_cInfo[g_bNow].szFilePath;
						if(FILE_EXIST(lpszPlayingPath)){
							GetParentDir(lpszPlayingPath, szNowDir);
							MakeTreeFromPath(m_hTree, m_hCombo, szNowDir);
						}else if(IsArchivePath(lpszPlayingPath)){
							LPTSTR p = StrStr(lpszPlayingPath, TEXT("/"));
							*p = TEXT('\0');
							lstrcpyn(szNowDir, lpszPlayingPath, MAX_FITTLE_PATH);
							*p = TEXT('/');
							MakeTreeFromPath(m_hTree, m_hCombo, szNowDir);
						}
						SelectFolderItem(lpszPlayingPath);
					}
					break;

				case IDM_README:	// Readme.txt��\��
					{
						WASTR szPath;
						WAGetModuleParentDir(NULL, &szPath);
						WAstrcatA(&szPath, "Readme.txt");
						WAShellExecute(hWnd, &szPath, NULL);
					}
					break;

				case IDM_VER:	// �o�[�W�������
					SendF4b24Message(WM_F4B24_IPC_SETTING, WM_F4B24_IPC_SETTING_LP_ABOUT);
					break;

				case IDM_LIST_MOVETOP:	// ��ԏ�Ɉړ�
					ChangeOrder(GetSelListTab(), 0);
					break;

				case IDM_LIST_MOVEBOTTOM: //��ԉ��Ɉړ�
					{
						struct LISTTAB *pCurList = GetSelListTab();
						ChangeOrder(pCurList, LV_GetCount(pCurList->hList) - 1);
					}
					break;

				case IDM_LIST_URL:
					break;

				case IDM_LIST_NEW:	// �V�K�v���C���X�g
					{
						lstrcpy(szLabel, TEXT("Default"));
						if(CreateTab(szLabel)){
							SendToPlaylist(GetSelListTab(), GetListTab(m_hTab, TabGetListCount()-1));
							TabSetListFocus(TabGetListCount()-1);
						}
					}
					break;

				case IDM_LIST_ALL:	// �S�đI��
					LV_SetState(GetSelListTab()->hList, -1, LVIS_SELECTED);
					break;

				case IDM_LIST_DEL: //���X�g����폜
					DeleteFiles(GetSelListTab());
					break;

				case IDM_LIST_DELFILE:
					RemoveFiles();
					break;

				case IDM_LIST_TOOL:
					{
						struct LISTTAB *pCurList = GetSelListTab();
						int nLBIndex = LV_GetNextSelect(pCurList->hList, -1);
						if(nLBIndex!=-1){
							LPTSTR pszFiles = MallocAndConcatPath(pCurList);
							if (pszFiles) {
								ToolExecute(hWnd, pszFiles, TRUE);
								HFree(pszFiles);
							}
						}
					}
					break;

				case IDM_LIST_PROP:	// �v���p�e�B
					{
						SHELLEXECUTEINFO sei;
						struct LISTTAB *pCurList = GetSelListTab();
						int nLBIndex = LV_GetNextSelect(pCurList->hList, -1);
						if(nLBIndex!=-1){
							ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
							sei.cbSize = sizeof(SHELLEXECUTEINFO);
							sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
							sei.lpFile = GetPtrFromIndex(pCurList->pRoot, nLBIndex)->szFilePath;
							sei.lpVerb = TEXT("properties");
							sei.hwnd = NULL;
							ShellExecuteEx(&sei);
						}
					}
					break;

				case IDM_TREE_UP:
					if(g_cfg.nTreeState==TREE_UNSHOWN){
						if(!PathIsRoot(m_szTreePath)){
							*PathFindFileName(m_szTreePath) = TEXT('\0');
							MakeTreeFromPath(m_hTree, m_hCombo, m_szTreePath);
						}
					}else{
						HTREEITEM hParent;
						if(!m_hHitTree) TreeGetNextItem();
						hParent = TreeView_GetParent(m_hTree, m_hHitTree);
						if(hParent!=NULL){
							TreeSelect(hParent);
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
					TreeSelect(m_hHitTree);
					SendFittleCommand(IDM_SUB);
					m_hHitTree = NULL;
					break;

				case IDM_TREE_ADD:
					{
						struct FILEINFO *pSub = NULL;

						GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
						SearchFiles(&pSub, szNowDir, TRUE);
						AppendToList(GetSelListTab(), pSub);
						m_hHitTree = NULL;
					}
					break;

				case IDM_TREE_NEW:
					{
						struct LISTTAB *pNew;
						GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
						lstrcpyn(szLabel, PathFindFileName(szNowDir), MAX_FITTLE_PATH);
						if(IsPlayList(szNowDir) || IsArchive(szNowDir)){
							// ".m3u"���폜
							PathRemoveExtension(szLabel);
						}
						pNew = CreateTab(szLabel);
						if(pNew){
							SearchFiles(&(pNew->pRoot), szNowDir, TRUE);
							TraverseList(pNew);
							TabSetListFocus(TabGetListCount()-1);
							InvalidateRect(m_hTab, NULL, FALSE);
						}
						m_hHitTree = NULL;
					}
					break;

				case IDM_EXPLORE:
				case IDM_TREE_EXPLORE:
				case IDM_TREE_TOOL:
					// �p�X�̎擾
#if 1
					// step �o�O�΍� ������quote
					if(LOWORD(wp)==IDM_EXPLORE){
						lstrcpyn(szNowDir + 1, m_szTreePath, MAX_FITTLE_PATH);
					}else{
						GetPathFromNode(m_hTree, m_hHitTree, szNowDir + 1);
					}
					GetFolderPart(szNowDir + 1);
					szNowDir[0] = '\"';
					i = lstrlen(szNowDir);
					szNowDir[i] = '\"';
					szNowDir[i + 1] = 0;
#else
					if(LOWORD(wp)==IDM_EXPLORE){
						lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
					}else{
						GetPathFromNode(m_hTree, m_hHitTree, szNowDir);
					}
					GetFolderPart(szNowDir);
					PathQuoteSpaces(szNowDir);
#endif

					// �G�N�X�v���[���ɓ����鏈��
					ToolExecute(hWnd, szNowDir, LOWORD(wp)==IDM_TREE_TOOL);
					m_hHitTree = NULL;
					break;

				case IDM_TAB_NEW:
					lstrcpy(szLabel, TEXT("Default"));
					if(CreateTab(szLabel)){
						TabSetListFocus(TabGetListCount()-1);
					}
					break;

				case IDM_TAB_RENAME:
					lstrcpyn(szLabel, GetListTab(m_hTab, s_nHitTab)->szTitle, MAX_FITTLE_PATH);
					if(GetTabName(szLabel)){
						RenameListTab(m_hTab, s_nHitTab, szLabel);
						InvalidateRect(m_hTab, NULL, FALSE);
					}
					break;

				case IDM_TAB_DEL:
					// �J�����g�^�u�������Ȃ獶���J�����g�ɂ���
					if(TabGetListSel()==s_nHitTab){
						TabSetListFocus(s_nHitTab-1);
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
					TabSetListFocus((TabGetListSel()+1==TabGetListCount()?0:TabGetListSel()+1));
					break;

				case IDM_TAB_FOR_LEFT:
					TabSetListFocus((TabGetListSel()?TabGetListSel()-1:TabGetListCount()-1));
					break;

				case IDM_TAB_SORT:
					SortListTab(GetSelListTab(), 0);
					break;

				case IDM_TAB_NOEXIST:
					{
						struct LISTTAB *pCurList = GetSelListTab();
						ListView_SetItemCountEx(pCurList->hList, LinkCheck(&(pCurList->pRoot)), 0);
					}
					break;

				case IDM_TRAY_WINVIEW:
					ToggleWindowView(hWnd);
					break;

				case IDM_TOGGLEFOCUS:
					{
						struct LISTTAB *pCurList = GetSelListTab();
						if(GetFocus()!=pCurList->hList){
							SetFocus(pCurList->hList);
						}else{
							SetFocus(m_hTree);
						}
					}
					break;

				case ID_COMBO:
					if(HIWORD(wp)==CBN_SELCHANGE){
						TreeSelectDrive();
					}
					break;

				default:
					if(IDM_SENDPL_FIRST<=LOWORD(wp) && LOWORD(wp)<IDM_BM_FIRST){
						//�v���C���X�g�ɑ���
						SendToPlaylist(GetSelListTab(), GetListTab(m_hTab, LOWORD(wp) - IDM_SENDPL_FIRST));
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
					SendFittleCommand(wp);
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
			if((HWND)wp==hWnd){
				return DefWindowProc(hWnd, msg, wp, lp);		
			}else{
				HMENU hPopMenu;
				RECT rcItem;
				switch(GetDlgCtrlID((HWND)wp))
				{
					case ID_TREE:
						TVHITTESTINFO tvhti;

						if(lp==-1){	// �L�[�{�[�h
							TreeGetNextItem();
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
						hPopMenu = GetSubMenu(LoadMenu(m_hInst, TEXT("TREEMENU")), 0);
						TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, hWnd, NULL);
						DestroyMenu(hPopMenu);
						TreeView_SelectDropTarget(m_hTree, NULL);
						break;

					case ID_TAB:
						if(lp==-1){	// �L�[�{�[�h
							s_nHitTab = TabGetListSel();
							TabGetItemRect(s_nHitTab, &rcItem);
							MapWindowPoints(m_hTab, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
							lp = MAKELPARAM(rcItem.left, rcItem.bottom);
						}else{	// �}�E�X
							s_nHitTab = TabHitTest(lp, 0x40 | TCHT_NOWHERE);
						}
						hPopMenu = GetSubMenu(LoadMenu(m_hInst, TEXT("TABMENU")), 0);
						// �O���[�\���̏���
						if(s_nHitTab<=0){
							EnableMenuItem(hPopMenu, IDM_TAB_DEL, MF_GRAYED | MF_DISABLED);
							EnableMenuItem(hPopMenu, IDM_TAB_RENAME, MF_GRAYED | MF_DISABLED);
						}
						if(s_nHitTab<=1) EnableMenuItem(hPopMenu, IDM_TAB_LEFT, MF_GRAYED | MF_DISABLED);
						if(s_nHitTab>=TabGetListCount()-1 || s_nHitTab<=0)
							EnableMenuItem(hPopMenu, IDM_TAB_RIGHT, MF_GRAYED | MF_DISABLED);
						TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, hWnd, NULL);
						DestroyMenu(hPopMenu);
						break;

					case ID_REBAR:
						POINT pt;
						HMENU hOptionMenu = GetSubMenu(m_hMainMenu, GetMenuPosFromString(m_hMainMenu, TEXT("�I�v�V����(&O)")));
						GetCursorPos(&pt);
						TrackPopupMenu(GetSubMenu(hOptionMenu, GetMenuPosFromString(hOptionMenu, TEXT("�c�[���o�[(&B)"))), TPM_LEFTALIGN | TPM_TOPALIGN, /*(short)LOWORD(lp)*/pt.x, /*(short)HIWORD(lp)*/pt.y, 0, hWnd, NULL);
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
						DoTrayClickAction(0);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_RBUTTONUP:
					if(s_nClickState==1){
						s_nClickState = 2;
					}else if(s_nClickState==3){
						DoTrayClickAction(2);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_MBUTTONUP:
					if(s_nClickState==1){
						s_nClickState = 2;
					}else if(s_nClickState==3){
						DoTrayClickAction(4);
						s_nClickState = 0;
					}else{
						s_nClickState = 0;
					}
					break;

				case WM_LBUTTONDBLCLK:
					KillTimer(hWnd, ID_LBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(1);
					break;

				case WM_RBUTTONDBLCLK:
					KillTimer(hWnd, ID_RBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(3);
					break;

				case WM_MBUTTONDBLCLK:
					KillTimer(hWnd, ID_MBTNCLKTIMER);
					s_nClickState = 0;
					DoTrayClickAction(5);
					break;

				default:
					return DefWindowProc(hWnd, msg, wp, lp);
			}
			break;

		case WM_TIMER:

			switch (wp){
				case ID_SEEKTIMER:
					StatusBarDisplayTime();
					break;

				case ID_TIPTIMER:
					//�c�[���`�b�v������
					SendMessage(m_hTitleTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, NULL);
					KillTimer(hWnd, ID_TIPTIMER);
					break;

				case ID_LBTNCLKTIMER:
					KillTimer(hWnd, ID_LBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(0);
						s_nClickState = 0;
					}else if(s_nClickState==1){
						s_nClickState = 3;
					}
					break;

				case ID_RBTNCLKTIMER:
					KillTimer(hWnd, ID_RBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(2);
						s_nClickState = 0;
					}else if(s_nClickState==1){
						s_nClickState = 3;
					}
					break;

				case ID_MBTNCLKTIMER:
					KillTimer(hWnd, ID_MBTNCLKTIMER);
					if(s_nClickState==2){
						DoTrayClickAction(4);
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
							SendFittleCommand(IDM_FILEREVIEW);
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
							ShowWindow(GetSelListTab()->hList, SW_HIDE);
							break;
						
						case TCN_SELCHANGE:
							{
								struct LISTTAB *pCurList = GetSelListTab();
								GetClientRect(m_hTab, &rcTab);
								if(!(g_cfg.nTabHide && TabGetListCount()==1)){
									TabAdjustRect(&rcTab);
								}
								SendMessage(pCurList->hList, WM_SETFONT, (WPARAM)m_hFont, 0);
								SetWindowPos(pCurList->hList,
									HWND_TOP,
									rcTab.left,
									rcTab.top,
									rcTab.right - rcTab.left,
									rcTab.bottom - rcTab.top,
									SWP_SHOWWINDOW);
								SetFocus(pCurList->hList);
								LockWindowUpdate(NULL);
							}
							break;
					}
					break;

				case ID_REBAR:
					// ���o�[�̍s�����ς������WM_SIZE�𓊂���
					if(pnmhdr->code==RBN_HEIGHTCHANGE){
						UpdateWindow(m_hRebar);
						UpdateWindowSize(hWnd);
					}
					break;

				case ID_STATUS:
					if(pnmhdr->code==NM_DBLCLK){
						SendFittleCommand(IDM_MINIPANEL);
					}
					break;

				default:
					// �c�[���o�[�̃c�[���`�b�v�̕\��
					if(pnmhdr->code==TTN_NEEDTEXT){ 
						TOOLTIPTEXT *lptip;
						lptip = (LPTOOLTIPTEXT)lp; 
						switch (lptip->hdr.idFrom){ 
							case IDM_PLAY: 
								lptip->lpszText = TEXT("�Đ� (Z)"); 
								break; 
							case IDM_PAUSE: 
								lptip->lpszText = TEXT("�ꎞ��~ (X)"); 
								break; 
							case IDM_STOP: 
								lptip->lpszText = TEXT("��~ (C)"); 
								break; 
							case IDM_PREV: 
								lptip->lpszText = TEXT("�O�̋� (V)"); 
								break; 
							case IDM_NEXT: 
								lptip->lpszText = TEXT("���̋� (B)"); 
								break; 
							case IDM_PM_TOGGLE:
								switch (m_nPlayMode)
								{
									case PM_LIST:
										lptip->lpszText = TEXT("���X�g"); 
										break;
									case PM_RANDOM:
										lptip->lpszText = TEXT("�����_��"); 
										break;
									case PM_SINGLE:
										lptip->lpszText = TEXT("�V���O��"); 
										break;
								}
								break; 
							case IDM_PM_REPEAT: 
								lptip->lpszText = TEXT("���s�[�g (Ctrl+R)"); 
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
					SendFittleCommand(IDM_PLAY);
					break;
				case 1:	// �Đ�/�ꎞ��~
					SendFittleCommand(IDM_PLAYPAUSE);
					break;
				case 2:	// ��~
					SendFittleCommand(IDM_STOP);
					break;
				case 3:	// �O�̋�
					SendFittleCommand(IDM_PREV);
					break;
				case 4:	// ���̋�
					SendFittleCommand(IDM_NEXT);
					break;
				case 5:	// ���ʂ��グ��
					SendFittleCommand(IDM_VOLUP);
					break;
				case 6:	// ���ʂ�������
					SendFittleCommand(IDM_VOLDOWN);
					break;
				case 7: // �^�X�N�g���C���畜�A
					SendFittleCommand(IDM_TRAY_WINVIEW);
					break;
			}
			break;

		case WM_FITTLE:	// �v���O�C���C���^�[�t�F�C�X
			switch(wp){
				case GET_TITLE:
					return (LRESULT)m_taginfoold.szTitle;

				case GET_ARTIST:
					return (LRESULT)m_taginfoold.szArtist;

				case GET_PLAYING_PATH:
#ifdef UNICODE
					lstrcpyntA(m_szFilePathA, g_cInfo[g_bNow].szFilePath, MAX_FITTLE_PATH);
					return (LRESULT)m_szFilePathA;
#else
					return (LRESULT)g_cInfo[g_bNow].szFilePath;
#endif

				case GET_PLAYING_INDEX:
					return (LRESULT)GetSelListTab()->nPlaying;

				case GET_STATUS:
					return (LRESULT)OPGetStatus();

				case GET_POSITION:
					return (LRESULT)TrackPosToSec(TrackGetPos());

				case GET_DURATION:
					return (LRESULT)TrackPosToSec(g_cInfo[g_bNow].qDuration);

				case GET_LISTVIEW:
					int nCount;

					if(lp<0){
						return (LRESULT)GetSelListTab()->hList;
					}else{
						nCount = TabGetListCount();
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
							return (LRESULT)GetDlgItem(m_hRebar, lp);
					}
					break;

				case GET_CURPATH:
#ifdef UNICODE
					lstrcpyntA(m_szTreePathA, m_szTreePath, MAX_FITTLE_PATH);
					return (LRESULT)m_szTreePathA;
#else
					return (LRESULT)m_szTreePath;
#endif

				case GET_MENU:
					return (HRESULT)m_hMainMenu;

				case GET_MINIPANEL:
					return 0;

				default:
					break;
			}
			break;

		case WM_F4B24_IPC:
			switch(wp){
			case WM_F4B24_INTERNAL_PLAY_NEXT:
				EnterCriticalSection(&m_cs);

				// �t�@�C���؂�ւ�
				g_bNow = !g_bNow;
				// �J��
				FreeChannelInfo(!g_bNow);

				LeaveCriticalSection(&m_cs);

				PostMessage(m_hMainWnd, WM_USER+1, 0, 0); 
				break;
			case WM_F4B24_INTERNAL_PREPARE_NEXT_MUSIC:
				g_pNextFile = SelectNextFile(TRUE);

				if(g_pNextFile){
					m_nGaplessState = GS_OK;
					// �I�[�v��
					if(!SetChannelInfo(!g_bNow, g_pNextFile)){
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
				}else{
					m_nGaplessState = GS_END;
				}
				break;
			case WM_F4B24_INTERNAL_RESTORE_POSITION:
				if(g_cfg.nResPosFlag){
					DWORD hChan = g_cInfo[g_bNow].hChan;
					BASS_ChannelStop(hChan);
					TrackSetPos(g_cfg.nResPos);
					BASS_ChannelPlay(hChan, FALSE);
				}
				break;

			case WM_F4B24_IPC_GET_VERSION:
				return F4B24_VERSION;
			case WM_F4B24_IPC_GET_IF_VERSION:
				return F4B24_IF_VERSION;
			case WM_F4B24_IPC_APPLY_CONFIG:
				ApplyConfig(hWnd);
				break;
			case WM_F4B24_IPC_UPDATE_DRIVELIST:
				SetDrivesToCombo(m_hCombo);
				break;
			case WM_F4B24_IPC_GET_REPLAYGAIN_MODE:
				return g_cfg.nReplayGainMode;
			case WM_F4B24_IPC_GET_PREAMP:
				return g_cfg.nReplayGainPreAmp;
			case WM_F4B24_IPC_INVOKE_OUTPUT_PLUGIN_SETUP:
				return OPSetup(hWnd);
			case WM_F4B24_IPC_TRAYICONMENU:
				PopupTrayMenu();
				break;
			case WM_F4B24_IPC_GET_LX_IF:
				return (LRESULT)GetLXIf();

			case WM_F4B24_IPC_GET_VERSION_STRING:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)FITTLE_VERSION);
				break;
			case WM_F4B24_IPC_GET_VERSION_STRING2:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)TEXT("f4b24 - ") F4B24_VERSION_STRING);
				break;
			case WM_F4B24_IPC_GET_SUPPORT_LIST:
				SendSupportList((HWND)lp);
				break;
			case WM_F4B24_IPC_GET_PLAYING_PATH:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)g_cInfo[g_bNow].szFilePath);
				break;
			case WM_F4B24_IPC_GET_PLAYING_TITLE:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_taginfo.szTitle);
				break;
			case WM_F4B24_IPC_GET_PLAYING_ARTIST:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_taginfo.szArtist);
				break;
			case WM_F4B24_IPC_GET_PLAYING_ALBUM:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_taginfo.szAlbum);
				break;
			case WM_F4B24_IPC_GET_PLAYING_TRACK:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_taginfo.szTrack);
				break;

			case WM_F4B24_IPC_SETTING:
				ShowSettingDialog(hWnd, lp);
				break;

			case WM_F4B24_IPC_GET_CAPABLE:
				if (lp == WM_F4B24_IPC_GET_CAPABLE_LP_FLOATOUTPUT){
					return OPIsSupportFloatOutput() ? WM_F4B24_IPC_GET_CAPABLE_RET_SUPPORTED : WM_F4B24_IPC_GET_CAPABLE_RET_NOT_SUPPORTED;
				}
				break;

			case WM_F4B24_IPC_GET_COLUMN_TYPE:
				return GetColumnType(lp);
				
			case WM_F4B24_IPC_GET_CURPATH:
				SendMessage((HWND)lp, WM_SETTEXT, 0, (LPARAM)m_szTreePath);
				break;
			case WM_F4B24_IPC_SET_CURPATH:
				{
					TCHAR szWorkPath[MAX_FITTLE_PATH];
					SendMessage((HWND)lp, WM_GETTEXT, (WPARAM)MAX_FITTLE_PATH, (LPARAM)szWorkPath);
					SetFolder(szWorkPath);
				}
				break;

			case WM_F4B24_HOOK_REPLAY_GAIN:
				return 0;
			}
			break;

		case WM_DEVICECHANGE:	// �h���C�u�̐��̕ύX�ɑΉ�
			switch (wp)
			{
				case 0x8000://DBT_DEVICEARRIVAL:
				case 0x8004://DBT_DEVICEREMOVECOMPLETE:
					SendF4b24Message((WPARAM)WM_F4B24_IPC_UPDATE_DRIVELIST, (LPARAM)0);
					//SetDrivesToCombo(m_hCombo);
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

// �Đ����Ԃ��X�e�[�^�X�o�[�ɕ\��
static void StatusBarDisplayTime(){
	TCHAR buf[64]; // 9 + 1 + 9 + 3 + 9 + 1 + 9 + 1
	QWORD qPos = TrackGetPos();
	QWORD qLen = g_cInfo[g_bNow].qDuration;
	int nPos = (int)TrackPosToSec(qPos);
	int nLen = (int)TrackPosToSec(qLen);
	wsprintf(m_szTime, TEXT("\t%02d:%02d / %02d:%02d"), nPos/60, nPos%60, nLen/60, nLen%60);
	SendMessage(m_hStatus, SB_GETTEXT, (WPARAM)2, (LPARAM)buf);
	if (lstrcmpi(buf, m_szTime) != 0)
		SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)2|0, (LPARAM)m_szTime);
	//�V�[�N���łȂ���Ό��݂̍Đ��ʒu���X���C�_�o�[�ɕ\������
	if(!(GetCapture()==m_hSeek || qLen==0))
		PostMessage(m_hSeek, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(SLIDER_DIVIDED * qPos / qLen));
}

static void SelectListItem(struct LISTTAB *pList, LPTSTR lpszPath) {
	int nFileIndex = GetIndexFromPath(pList->pRoot, lpszPath);
	LV_SingleSelectView(pList->hList, nFileIndex);
}

static void SelectFolderItem(LPTSTR lpszPath) {
	SelectListItem(GetFolderListTab(), lpszPath);
}

static BOOL GetTabName(LPTSTR szTabName){
	return DialogBoxParam(m_hInst, TEXT("TAB_NAME_DIALOG"), m_hMainWnd, TabNameDlgProc, (LPARAM)szTabName) != 0;
}

struct LISTTAB *CreateTab(LPTSTR szTabName){
	return GetTabName(szTabName) ? MakeNewTab(m_hTab, szTabName, -1) : 0;
}



static void DoTrayClickAction(int nKind){
	switch(g_cfg.nTrayClick[nKind]){
		case 1:
			SendFittleCommand(IDM_PLAY);
			break;
		case 2:
			SendFittleCommand(IDM_PAUSE);
			break;
		case 3:
			SendFittleCommand(IDM_STOP);
			break;
		case 4:
			SendFittleCommand(IDM_PREV);
			break;
		case 5:
			SendFittleCommand(IDM_NEXT);
			break;
		case 6:
			SendFittleCommand(IDM_TRAY_WINVIEW);
			break;
		case 7:
			SendFittleCommand(IDM_END);
			break;
		case 8:
			SendF4b24Message((WPARAM)WM_F4B24_IPC_TRAYICONMENU, 0);
			break;
		case 9:
			SendFittleCommand(IDM_PLAYPAUSE);
			break;

	}
	return;
}

static void PopupTrayMenu(){
	POINT pt;

	GetCursorPos(&pt);
	SetForegroundWindow(m_hMainWnd);
	TrackPopupMenu(m_hTrayMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, m_hMainWnd, NULL);
	PostMessage(m_hMainWnd, WM_NULL, 0, 0);	/* KB944343 */
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
	mii.dwTypeData = TEXT("���X�g(&L)");
	mii.wID = IDM_PM_LIST;
	if(m_nPlayMode==PM_LIST) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, 0, FALSE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = TEXT("�����_��(&R)");
	mii.wID = IDM_PM_RANDOM;
	if(m_nPlayMode==PM_RANDOM) mii.fState = MFS_CHECKED;
	InsertMenuItem(hPopMenu, IDM_PM_LIST, TRUE, &mii);
	mii.fState = MFS_UNCHECKED;
	mii.dwTypeData = TEXT("�V���O��(&S)");
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

	// �R���g���[���J���[
	g_cfg.nBkColor = WAGetIniInt("Color", "BkColor", (int)GetSysColor(COLOR_WINDOW));
	g_cfg.nTextColor = WAGetIniInt("Color", "TextColor", (int)GetSysColor(COLOR_WINDOWTEXT));
	g_cfg.nPlayTxtCol = WAGetIniInt("Color", "PlayTxtCol", (int)RGB(0xFF, 0, 0));
	g_cfg.nPlayBkCol = WAGetIniInt("Color", "PlayBkCol", (int)RGB(230, 234, 238));
	// �\�����@
	g_cfg.nPlayView = WAGetIniInt("Color", "PlayView", 1);

	// �V�X�e���̗D��x�̐ݒ�
	g_cfg.nHighTask = WAGetIniInt("Main", "Priority", 0);
	// �O���b�h���C��
	g_cfg.nGridLine = WAGetIniInt("Main", "GridLine", 1);
	g_cfg.nSingleExpand = WAGetIniInt("Main", "SingleExp", 0);
	// ���݊m�F
	g_cfg.nExistCheck = WAGetIniInt("Main", "ExistCheck", 0);
	// �v���C���X�g�ōX�V�������擾����
	g_cfg.nTimeInList = WAGetIniInt("Main", "TimeInList", 0);
	// �V�X�e���C���[�W���X�g������
	g_cfg.nTreeIcon = WAGetIniInt("Main", "TreeIcon", TRUE);
	// �^�X�N�g���C�Ɏ��[�̃`�F�b�N
	g_cfg.nTrayOpt = WAGetIniInt("Main", "Tray", 1);
	// �c���[�\���ݒ�
	g_cfg.nHideShow = WAGetIniInt("Main", "HideShow", 0);
	// �^�u�����ɕ\������
	g_cfg.nTabBottom = WAGetIniInt("Main", "TabBottom", 0);
	// ���i�ŕ\������
	g_cfg.nTabMulti = WAGetIniInt("Main", "TabMulti", 1);
	// ���ׂẴt�H���_���`
	g_cfg.nAllSub = WAGetIniInt("Main", "AllSub", 0);
	// �c�[���`�b�v�Ńt���p�X��\��
	g_cfg.nPathTip = WAGetIniInt("Main", "PathTip", 1);
	// �Ȗ����m�点�@�\
	g_cfg.nInfoTip = WAGetIniInt("Main", "Info", 1);
	// �^�O�𔽓]
	g_cfg.nTagReverse = WAGetIniInt("Main", "TagReverse", 0);
	// �w�b�_�R���g���[����\������
	g_cfg.nShowHeader = WAGetIniInt("Main", "ShowHeader", 1);
	// �V�[�N��
	g_cfg.nSeekAmount = WAGetIniInt("Main", "SeekAmount", 5);
	// �V�[�N���Ƀ|�[�Y����������
	g_cfg.nRestartOnSeek = WAGetIniInt("Main", "RestartOnSeek", 1);
	// ���ʕω���(�B���ݒ�)
	g_cfg.nVolAmount = WAGetIniInt("Main", "VolAmount", 5);
	// �I�����ɍĐ����Ă����Ȃ��N�����ɂ��Đ�����
	g_cfg.nResume = WAGetIniInt("Main", "Resume", 0);
	// �I�����̍Đ��ʒu���L�^��������
	g_cfg.nResPosFlag = WAGetIniInt("Main", "ResPosFlag", 0);
	// �I�����ɍĐ����Ă����Ȃ��N�����ɑI������
	g_cfg.nSelLastPlayed = WAGetIniInt("Main", "SelLastPlayed", 0);
	// ����{�^���ōŏ�������
	g_cfg.nCloseMin = WAGetIniInt("Main", "CloseMin", 0);
	// �T�u�t�H���_�������ň��k�t�@�C������������
	g_cfg.nZipSearch = WAGetIniInt("Main", "ZipSearch", 0);
	// �^�u����̎��̓^�u���B��
	g_cfg.nTabHide = WAGetIniInt("Main", "TabHide", 0);
	// �X�^�[�g�A�b�v�t�H���_�ǂݍ���
	WAGetIniStr("Main", "StartPath", &g_cfg.szStartPath);
	// �t�@�C���̃p�X
	WAGetIniStr("Main", "FilerPath", &g_cfg.szFilerPath);

	// ���X�g�r���[�̃\�[�g���@
	g_cfg.nListSort = WAGetIniInt("Column", "ListSort", 0);

	// �z�b�g�L�[�̐ݒ�
	for(i=0;i<HOTKEY_COUNT;i++){
		g_cfg.nHotKey[i] = WAGetIniIntFmt("HotKey", "HotKey%d", i, 0);
	}

	// �N���b�N���̓���
	for(i=0;i<6;i++){
		g_cfg.nTrayClick[i] = WAGetIniIntFmt("TaskTray", "Click%d", i, "\x6\x0\x8\x0\x5\x0"[i]); //�z�b�g�L�[
	}

	// �t�H���g�ݒ�ǂݍ���
	WAGetIniStr("Font", "FontName", &g_cfg.szFontName);	// �t�H���g��""���f�t�H���g�̈�
	g_cfg.nFontHeight = WAGetIniInt("Font", "Height", 10);
	g_cfg.nFontStyle = WAGetIniInt("Font", "Style", 0);

	// �O���c�[��
	WAGetIniStr("Tool", "Path0", &g_cfg.szToolPath);


	// �����o�̓f�o�C�X
	g_cfg.nOutputDevice = WAGetIniInt("Main", "OutputDevice", 0);
	// BASS�����������鎞�̃T���v�����O���[�g
	g_cfg.nOutputRate = WAGetIniInt("Main", "OutputRate", 44100);
	// 32bit(float)�ŏo�͂���
	g_cfg.nOut32bit = WAGetIniInt("Main", "Out32bit", 0);
	// ��~���Ƀt�F�[�h�A�E�g����
	g_cfg.nFadeOut = WAGetIniInt("Main", "FadeOut", 1);
	// ReplayGain�̓K�p���@
	g_cfg.nReplayGainMode = WAGetIniInt("ReplayGain", "Mode", 5);
	// ���ʑ������@
	g_cfg.nReplayGainMixer = WAGetIniInt("ReplayGain", "Mixer", 1);
	// PreAmp(%)
	g_cfg.nReplayGainPreAmp = WAGetIniInt("ReplayGain", "PreAmp", 100);
	// PostAmp(%)
	g_cfg.nReplayGainPostAmp = WAGetIniInt("ReplayGain", "PostAmp", 100);

}

/* �I�����̏�Ԃ�ǂݍ��� */
static void LoadState(){
	// �~�j�p�l���\�����
	g_cfg.nMiniPanelEnd = WAGetIniInt("MiniPanel", "End", 0);
	// �c���[�̕���ݒ�
	g_cfg.nTreeWidth = WAGetIniInt("Main", "TreeWidth", 200);
	g_cfg.nTreeState = WAGetIniInt("Main", "TreeState", TREE_SHOW);
	// �X�e�[�^�X�o�[�\����\��
	g_cfg.nShowStatus =  WAGetIniInt("Main", "ShowStatus", 1);
	// �I�����̍Đ��ʒu���L�^��������
	g_cfg.nResPos = WAGetIniInt("Main", "ResPos", 0);
	// �Ō�ɍĐ����Ă����t�@�C��
	WAGetIniStr("Main", "LastFile", &g_cfg.szLastFile);

	m_hFont = NULL;
}

/* �I�����̏�Ԃ�ۑ����� */
static void SaveState(HWND hWnd){
	int i;
	WASTR lastpath;
	WINDOWPLACEMENT wpl;
	REBARBANDINFO rbbi;

	wpl.length = sizeof(WINDOWPLACEMENT);

	WAstrcpyT(&lastpath, m_szTreePath);
	// �R���p�N�g���[�h���l�����Ȃ���E�B���h�E�T�C�Y��ۑ�

	GetWindowPlacement(hWnd, &wpl);
	WASetIniInt("Main", "Maximized", (wpl.showCmd==SW_SHOWMAXIMIZED || wpl.flags & WPF_RESTORETOMAXIMIZED));	// �ő剻
	WASetIniInt("Main", "Top", wpl.rcNormalPosition.top); //�E�B���h�E�ʒuTop
	WASetIniInt("Main", "Left", wpl.rcNormalPosition.left); //�E�B���h�E�ʒuLeft
	WASetIniInt("Main", "Height", wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top); //�E�B���h�E�ʒuHeight
	WASetIniInt("Main", "Width", wpl.rcNormalPosition.right - wpl.rcNormalPosition.left); //�E�B���h�E�ʒuWidth
	ShowWindow(hWnd, SW_HIDE);	// �I�������������Č����邽�߂ɔ�\��

	WASetIniInt("Main", "TreeWidth", g_cfg.nTreeWidth); //�c���[�̕�
	WASetIniInt("Main", "TreeState", (g_cfg.nTreeState==TREE_SHOW));
	WASetIniInt("Main", "Mode", m_nPlayMode); //�v���C���[�h
	WASetIniInt("Main", "Repeat", m_nRepeatFlag); //���s�[�g���[�h
	WASetIniInt("Main", "Volumes", (int)GetVolumeBar()); //�{�����[��
	WASetIniInt("Main", "ShowStatus", g_cfg.nShowStatus);
	WASetIniInt("Main", "ResPos", g_cfg.nResPos);

	WASetIniInt("Main", "TabIndex", (m_nPlayTab==-1?TabGetListSel():m_nPlayTab));	//Tab��Index
	WASetIniInt("Main", "MainMenu", (GetMenu(hWnd)?1:0));

	WASetIniStr("Main", "LastPath", &lastpath); //���X�g�p�X
	WASetIniStr("Main", "LastFile", &g_cfg.szLastFile);

	WASetIniInt("MiniPanel", "End", g_cfg.nMiniPanelEnd);

	for(i=0;i<GetColumnNum();i++){
		int t = GetColumnType(i);
		if (t >= 0 && t < 4){
			WASetIniIntFmt("Column", "Width%d", t, ListView_GetColumnWidth(GetSelListTab()->hList, i));
		}
	}
	WASetIniInt("Column", "Sort", GetSelListTab()->nSortState);

	//�@���o�[�̏�Ԃ�ۑ�
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE | RBBIM_ID;
	for(i=0;i<BAND_COUNT;i++){
		SendMessage(GetDlgItem(hWnd, ID_REBAR), RB_GETBANDINFO, i, (WPARAM)&rbbi);
		WASetIniIntFmt("Rebar2", "fStyle%d", i, rbbi.fStyle);
		WASetIniIntFmt("Rebar2", "cx%d", i, rbbi.cx);
		WASetIniIntFmt("Rebar2", "wID%d", i, rbbi.wID);
	}

	WAFlushIni();

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

// BASS Addon�����[�h�� �Ή��g���q���X�g�ɒǉ�
static BOOL CALLBACK FileProc(LPWASTR lpPath, LPVOID user){
	DWORD hPlugin = WAIsUnicode() ? BASS_PluginLoad((LPSTR)lpPath->W, BASS_UNICODE) : BASS_PluginLoad(lpPath->A, 0);
	if(hPlugin){
		const BASS_PLUGININFO *info = BASS_PluginGetInfo(hPlugin);
		for(DWORD d=0;d<info->formatc;d++){
			AddTypes(info->formats[d].exts);
		}
	}
	return TRUE;
}

// BASS Addon��T�� �Ή��g���q���X�g���쐬����
static void InitFileTypes(){
	ClearTypelist();
	AddTypes("mp3;mp2;mp1;wav;ogg;aif;aiff");
	AddTypes("mod;mo3;xm;it;s3m;mtm;umx");

	WAEnumFiles(NULL, "Plugins\\BASS\\", "bass*.dll", FileProc, 0);
}

static int XATOI(LPCTSTR p){
	int r = 0;
	while (*p == TEXT(' ') || *p == TEXT('\t')) p++;
	while (*p >= TEXT('0') && *p <= TEXT('9')){
		r = r * 10 + (*p - TEXT('0'));
		p++;
	}
	return r;
}

static QWORD GetSoundLength(DWORD hChan) {
	QWORD qLen = BASS_ChannelGetLength(hChan, BASS_POS_BYTE);
	return qLen != -1 ? qLen : 0;
}

/* xx:xx:xx�Ƃ����\�L�ƃn���h�����玞�Ԃ�QWORD�Ŏ擾 */
static QWORD GetByteFromSecStr(DWORD hChan, LPCTSTR pszSecStr){
	int min, sec, frame;

	if(*pszSecStr == TEXT('\0')){
		return GetSoundLength(hChan);
	}

	min = XATOI(pszSecStr);
	sec = 0;
	frame = 0;

	while (*pszSecStr && *pszSecStr != TEXT(':')) pszSecStr++;

	if (*pszSecStr){
		sec = XATOI(++pszSecStr);
		while (*pszSecStr && *pszSecStr != TEXT(':')) pszSecStr++;
		if (*pszSecStr){
			frame = XATOI(++pszSecStr);
		}
	}

	return BASS_ChannelSeconds2Bytes(hChan, (float)((min * 60 + sec) * 75 + frame) / 75.0);
}

static void LoadGainInfo(CHANNELINFO *pCh){
	DWORD dwGain = SendF4b24Message(WM_F4B24_HOOK_REPLAY_GAIN, (LPARAM)pCh->hChan);
	pCh->sGain = dwGain ? *(float *)&dwGain : (float)1;
}

static void FreeSoundFile(CHANNELINFO *pCh, BOOL fOpenForPlay){
	DWORD hChan = pCh->hChan;
	if (hChan){
		if (fOpenForPlay){
			BASS_ChannelStop(hChan);
			SendF4b24Message(WM_F4B24_HOOK_FREE_DECODE_STREAM, (LPARAM)hChan);
		}
		if (!BASS_StreamFree(hChan)){
			BASS_MusicFree(hChan);
		}
		pCh->hChan = 0;
	}

	if(pCh->pBuff){
		HeapFree(GetProcessHeap(), 0, (LPVOID)pCh->pBuff);
		pCh->pBuff = NULL;
	}
	pCh->sGain = 1;
}

static BOOL OpenSoundFile(CHANNELINFO *pCh, LPTSTR lpszOpenSoundPath, BOOL fOpenForPlay){
	TCHAR szFilePath[MAX_FITTLE_PATH];
	QWORD qStart, qEnd;
	TCHAR szStart[100], szEnd[100];

	szStart[0] = 0;
	szEnd[0] = 0;

	lstrcpyn(pCh->szFilePath, lpszOpenSoundPath, MAX_FITTLE_PATH);
	pCh->pBuff = 0;
	pCh->qStart = 0;

	if(IsArchivePath(lpszOpenSoundPath)){
		AnalyzeArchivePath(pCh, szFilePath, szStart, szEnd);
	}else{
		lstrcpyn(szFilePath, lpszOpenSoundPath, MAX_FITTLE_PATH);
		pCh->qDuration = 0;
	}

	pCh->sGain = 1;
	pCh->sAmp = fBypassVol;
	pCh->hChan = BASS_StreamCreateFile(pCh->pBuff != 0,
												(pCh->pBuff?(void *)pCh->pBuff:(void *)szFilePath),
												0, (DWORD)pCh->qDuration,
#ifdef UNICODE
												(pCh->pBuff?0:BASS_UNICODE) |
#endif
												BASS_STREAM_DECODE | m_bFloat*BASS_SAMPLE_FLOAT);
	if(!pCh->hChan){
		pCh->hChan = BASS_MusicLoad(pCh->pBuff != 0,
												(pCh->pBuff?(void *)pCh->pBuff:(void *)szFilePath),
												0, (DWORD)pCh->qDuration,
#ifdef UNICODE
												(pCh->pBuff?0:BASS_UNICODE) |
#endif
												BASS_MUSIC_DECODE | m_bFloat*BASS_SAMPLE_FLOAT | BASS_MUSIC_PRESCAN, 0);
	}
	if(pCh->hChan){
		if(szStart[0]){
			qStart = GetByteFromSecStr(pCh->hChan, szStart);
			qEnd = GetByteFromSecStr(pCh->hChan, szEnd);
			pCh->qStart = qStart;
			pCh->qDuration = qEnd - qStart;
		}else{
			pCh->qDuration = GetSoundLength(pCh->hChan);
		}
		if (fOpenForPlay){
			if(!IsArchivePath(lpszOpenSoundPath) || !GetArchiveGain(lpszOpenSoundPath, &pCh->sGain, pCh->hChan)){
				LoadGainInfo(pCh);
			}
			SendF4b24Message(WM_F4B24_HOOK_CREATE_DECODE_STREAM, (LPARAM)pCh->hChan);
			if(szStart[0]){
				BASS_ChannelSetPosition(pCh->hChan, qStart, BASS_POS_BYTE);
				BASS_ChannelSetSync(pCh->hChan, BASS_SYNC_POS, qStart + (pCh->qDuration)*99/100, &EventSync, 0);
				BASS_ChannelSetSync(pCh->hChan, BASS_SYNC_POS, qEnd, &EventSync, (void *)1);
			} else {
				// �Ȃ�99%�Đ���A�C�x���g���N����悤�ɁB
				if(lstrcmpi(PathFindExtension(szFilePath), TEXT(".CDA"))){
					BASS_ChannelSetSync(pCh->hChan, BASS_SYNC_POS, (pCh->qDuration)*99/100, &EventSync, 0);
				}
			}
		}
		return TRUE;
	}else{
#ifdef UNICODE
		CHAR szAnsi[MAX_FITTLE_PATH];
		lstrcpyntA(szAnsi, szFilePath, MAX_FITTLE_PATH);
		BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 60000); // 5sec�͒Z���̂�60sec�ɂ���
		pCh->hChan = BASS_StreamCreateURL(szAnsi, 0, BASS_STREAM_BLOCK | BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT * m_bFloat, NULL, 0);
#else
		BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 60000); // 5sec�͒Z���̂�60sec�ɂ���
		pCh->hChan = BASS_StreamCreateURL(szFilePath, 0, BASS_STREAM_BLOCK | BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT * m_bFloat, NULL, 0);
#endif
		if(pCh->hChan){
			pCh->qDuration = GetSoundLength(pCh->hChan);
			if (fOpenForPlay){
				LoadGainInfo(pCh);
				SendF4b24Message(WM_F4B24_HOOK_CREATE_DECODE_STREAM, (LPARAM)pCh->hChan);
				BASS_ChannelSetSync(pCh->hChan, BASS_SYNC_END, 0, &EventSync, 0);
			}
			return TRUE;
		}
		return FALSE;
	}
}

static BOOL GetTagInfo(CHANNELINFO *pCh, TAGINFO *pTagInfo){
	pTagInfo->dwLength = (DWORD)ChPosToSec(pCh->hChan, pCh->qDuration);
	return GetArchiveTagInfo(pCh->szFilePath, pTagInfo) || BASS_TAG_Read(pCh->hChan, pTagInfo);
}

LPVOID CALLBACK LXLoadMusic(LPVOID lpszPath){
	CHANNELINFO *pCh = (CHANNELINFO *)HZAlloc(sizeof(CHANNELINFO));
	if (pCh){
		if (!OpenSoundFile(pCh, (LPTSTR)lpszPath, FALSE)) {
			HFree(pCh);
			pCh = NULL;
		}
	}
	return pCh;
}

void CALLBACK LXFreeMusic(LPVOID pMusic){
	CHANNELINFO *pCh = (CHANNELINFO *)pMusic;
	if (pCh) {
		FreeSoundFile(pCh, FALSE);
		HFree(pCh);
	}
}

BOOL CALLBACK LXGetTag(LPVOID pMusic, LPVOID pTagInfo){
	CHANNELINFO *pCh = (CHANNELINFO *)pMusic;
	TAGINFO *pTag = (TAGINFO *)pTagInfo;
	return pCh ? GetTagInfo(pCh, pTag) : FALSE;
}



static BOOL SetChannelInfo(BOOL bFlag, struct FILEINFO *pInfo){
	// �t�@�C�����I�[�v���A�\���̂�ݒ�
	return OpenSoundFile(&g_cInfo[bFlag], pInfo->szFilePath, TRUE);
}

static void FreeChannelInfo(BOOL bFlag){
	FreeSoundFile(&g_cInfo[bFlag], TRUE);
}


// �w��t�@�C�����Đ�
static BOOL PlayByUser(HWND hWnd, struct FILEINFO *pPlayFile){
	BASS_CHANNELINFO info;
//	DWORD d;

	// �Đ��Ȃ��~�A�J��
	StopOutputStream();
	if(SetChannelInfo(g_bNow, pPlayFile)){
		// �X�g���[���쐬
		BASS_ChannelGetInfo(g_cInfo[g_bNow].hChan, &info);
		OPStart(&info, GetVolumeBar(), m_bFloat);

		OnStatusChangePlugins();

		// �V�[�N�o�[�p�^�C�}�[�쐬
		SetTimer(hWnd, ID_SEEKTIMER, 500, NULL);

		// �\���ؑ�
		g_pNextFile = pPlayFile;
		m_nGaplessState = GS_OK;
		OnChangeTrack();

		return TRUE;
	}else{
		SetWindowText(hWnd, TEXT("�t�@�C���̃I�[�v���Ɏ��s���܂����I"));
		return FALSE;
	}
}

// user:0 �g���b�N�̍Đ�����99%�Ŕ�������C�x���g �M���b�v���X�Đ����~���ɍs�����ߎ��̃t�@�C����\�߃I�[�v�����Ă���
// user:1 cue�Đ����Ƀg���b�N�̍Đ��I�����ԂŔ�������C�x���g ���̋Ȃ�
static void CALLBACK EventSync(DWORD /*handle*/, DWORD /*channel*/, DWORD /*data*/, void *user){
	if(user==0){
		PostF4b24Message(WM_F4B24_INTERNAL_PREPARE_NEXT_MUSIC, 0);
	}else{
		m_bCueEnd = TRUE;
	}
	return;
}

// �\���ؑ�
static void OnChangeTrack(){
	int nPlayIndex;
	TCHAR szShortTag[64] = {0};
	TCHAR szTitleCap[524] = {0};
	BASS_CHANNELINFO info;
	struct LISTTAB *pPlayList = GetPlayListTab();
	CHANNELINFO *pCh = &g_cInfo[g_bNow];

	m_bCueEnd = FALSE;

	// 99���܂ł����Ȃ������ꍇ
	if(m_nGaplessState != GS_END && (m_nGaplessState==GS_NOEND || !g_pNextFile)){
		g_pNextFile = SelectNextFile(TRUE);
		if(g_pNextFile){
			SetChannelInfo(g_bNow, g_pNextFile);
		}else{
			StopOutputStream();
			return;
		}
	}

	// ���s�[�g�I��
	if(!g_pNextFile){
		StopOutputStream();
		return;
	}

	//���g�����ς��Ƃ��͊J���Ȃ���
	BASS_ChannelGetInfo(pCh->hChan, &info);
	if(m_nGaplessState==GS_NEWFREQ){
		OPStop();
		OPStart(&info, GetVolumeBar(), m_bFloat);
	}else{
		OPSetVolume(GetVolumeBar());
	}

	// �؂�ւ�����t�@�C���̃C���f�b�N�X���擾
	nPlayIndex = GetIndexFromPtr(pPlayList->pRoot, g_pNextFile);

	// �\����؂�ւ�
	LV_SingleSelectView(pPlayList->hList, nPlayIndex);
	InvalidateRect(GetSelListTab()->hList, NULL, TRUE); // CUSTOMDRAW�C�x���g����

	// �t�@�C���̃I�[�v���Ɏ��s����
	if(m_nGaplessState==GS_FAILED){
		StopOutputStream();
		SetWindowText(m_hMainWnd, TEXT("�t�@�C���̃I�[�v���Ɏ��s���܂����I"));
		return;
	}

	// �Đ��ς݂ɂ���
	g_pNextFile->bPlayFlag = TRUE;
	WAstrcpyT(&g_cfg.szLastFile, g_pNextFile->szFilePath);

	// �X�e�[�^�X�N���A
	m_nGaplessState = GS_NOEND;

	// ���ݍĐ��ȂƈႤ�ȂȂ�Đ������ɒǉ�����
	if(GetPlaying(pPlayList)!=g_pNextFile)
		PushPlaying(pPlayList, g_pNextFile);
	
	// �J�����g�t�@�C����ύX
	pPlayList->nPlaying = nPlayIndex;


	// �^�O��
	if(GetTagInfo(pCh, &m_taginfo)){
		if(!g_cfg.nTagReverse){
			wsprintf(m_szTag, TEXT("%s / %s"), m_taginfo.szTitle, m_taginfo.szArtist);
		}else{
			wsprintf(m_szTag, TEXT("%s / %s"), m_taginfo.szArtist, m_taginfo.szTitle);
		}
	}else{
		lstrcpyn(m_szTag, GetFileName(pCh->szFilePath), MAX_FITTLE_PATH);
		lstrcpyn(m_taginfo.szTitle, m_szTag, 256);
	}
	lstrcpyntA(m_taginfoold.szTitle, m_taginfo.szTitle, 256);
	lstrcpyntA(m_taginfoold.szArtist, m_taginfo.szArtist, 256);
	lstrcpyntA(m_taginfoold.szAlbum, m_taginfo.szAlbum, 256);
	lstrcpyntA(m_taginfoold.szTrack, m_taginfo.szTrack, 10);

	//�^�C�g���o�[�̏���
	wsprintf(szTitleCap, TEXT("%s - %s"), m_szTag, FITTLE_TITLE);
	SetWindowText(m_hMainWnd, szTitleCap);

	float time = TrackPosToSec(GetSoundLength(pCh->hChan)); // playback duration
	DWORD dLen = (DWORD)BASS_StreamGetFilePosition(pCh->hChan, BASS_FILEPOS_END); // file length
	DWORD bitrate;
	if(dLen>0 && time>0){
		bitrate = (DWORD)(dLen/(125*time)+0.5); // bitrate (Kbps)
		wsprintf(szTitleCap, TEXT("%d Kbps  %d Hz  %s"), bitrate, info.freq, (info.chans!=1?TEXT("Stereo"):TEXT("Mono")));
	}else{
		wsprintf(szTitleCap, TEXT("? Kbps  %d Hz  %s"), info.freq, (info.chans!=1?TEXT("Stereo"):TEXT("Mono")));
	}

	//�X�e�[�^�X�o�[�̏���
	if(g_cfg.nTreeIcon){
		SetStatusbarIcon(pCh->szFilePath, TRUE);
	}
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)0|0, (LPARAM)szTitleCap);
	SendMessage(m_hStatus, SB_SETTIPTEXT, (WPARAM)0, (LPARAM)szTitleCap);

	wsprintf(szTitleCap, TEXT("\t%d / %d"), pPlayList->nPlaying + 1, LV_GetCount(pPlayList->hList));
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)1|0, (LPARAM)szTitleCap);

	//�V�[�N�o�[
	if(pCh->qDuration>0){
		EnableWindow(m_hSeek, TRUE);
	}else{
		EnableWindow(m_hSeek, FALSE);
	}

	//�^�X�N�g���C�̏���
	lstrcpyn(szShortTag, m_szTag, 54);
	szShortTag[52] = TEXT('.');
	szShortTag[53] = TEXT('.');
	szShortTag[54] = TEXT('.');
	szShortTag[55] = TEXT('\0');
	wsprintf(m_ni.szTip, TEXT("%s - Fittle"), szShortTag);

	if(m_bTrayFlag)
		Shell_NotifyIcon(NIM_MODIFY, &m_ni); //ToolTip�̕ύX
	if(g_cfg.nInfoTip == 2 || (g_cfg.nInfoTip == 1 && m_bTrayFlag))
		ShowToolTip(m_hMainWnd, m_hTitleTip, m_ni.szTip);

	OnTrackChagePlugins();

	g_pNextFile = NULL;	// ����
	return;
}

// ���ɍĐ����ׂ��t�@�C���̃|�C���^�擾
static struct FILEINFO *SelectNextFile(BOOL bTimer){
	int nLBCount;
	int nPlayIndex;
	int nTmpIndex;
	struct LISTTAB *pPlayList = GetPlayListTab();

	nLBCount = LV_GetCount(pPlayList->hList);
	if(nLBCount<=0){
		return NULL;
	}
	nPlayIndex = pPlayList->nPlaying;
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
				if(!m_nRepeatFlag && bTimer && GetUnPlayedFile(pPlayList->pRoot)==0){
					SetUnPlayed(pPlayList->pRoot);
					return NULL;
				}
			}else{
				nUnPlayCount = GetUnPlayedFile(pPlayList->pRoot);
				if(nUnPlayCount==0){
					if(m_nRepeatFlag || !bTimer){
						// ���ׂčĐ����Ă����ꍇ
						nBefore = pPlayList->nPlaying;
						nUnPlayCount = SetUnPlayed(pPlayList->pRoot);
						do{
							nTmpIndex = genrand_int32() % nUnPlayCount;
						}while(nTmpIndex==nBefore);
						nPlayIndex = GetUnPlayedIndex(pPlayList->pRoot, nTmpIndex);
					}else{
						// ���s�[�g���I�t
						SetUnPlayed(pPlayList->pRoot);
						return NULL;
					}
				}else{
					// �Đ��r��
					nTmpIndex = genrand_int32() % nUnPlayCount;
					nPlayIndex = GetUnPlayedIndex(pPlayList->pRoot, nTmpIndex);
				}
			}
			break;

		case PM_SINGLE:
			// lp=1�̂Ƃ��̓^�C�}�[����B����ȊO�̓��[�U�[����
			if(bTimer){
				if(!m_nRepeatFlag){
					return NULL;
				}
				if(pPlayList->nPlaying==-1)
					nPlayIndex = 0;
			}else{
				nPlayIndex++;
				if(pPlayList->nPlaying==--nLBCount)
					nPlayIndex = 0;
			}
			break;

	}
	return GetPtrFromIndex(pPlayList->pRoot, nPlayIndex);
}


#define TB_BTN_NUM 8
#define TB_BMP_NUM 9
// �c�[���o�[�̍쐬
static HWND CreateToolBar(){
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
                    m_hRebar,
                    (HMENU)ID_TOOLBAR, m_hInst, NULL);
	if(!hToolBar) return NULL;
	SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	hBmp = (HBITMAP)LoadImage(m_hInst, TEXT("toolbarex.bmp"), IMAGE_BITMAP, 16*TB_BMP_NUM, 16, LR_LOADFROMFILE | LR_SHARED);
	if(hBmp==NULL)	hBmp = LoadBitmap(m_hInst, (LPTSTR)IDR_TOOLBAR1);
	
	//SendMessage(hToolBar, TB_AUTOSIZE, 0, 0) ;
	
	hToolImage = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, TB_BMP_NUM, 0);
	ImageList_AddMasked(hToolImage, hBmp, RGB(0,255,0)); //�΂�w�i�F��

	DeleteObject(hBmp);
	SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)hToolImage);
	
	SendMessage(hToolBar, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
	SendMessage(hToolBar, TB_ADDBUTTONS, (WPARAM)TB_BTN_NUM, (LPARAM)&tbb);
	return hToolBar;
}

static HWND CreateToolTip(HWND hParent){
	return CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			hParent, NULL, 
			m_hInst, NULL);
}

static HWND CreateTrackBar(int nId, DWORD dwStyle){
	HWND hWnd = CreateWindowEx(0,
		TRACKBAR_CLASS,
		NULL,
		dwStyle,
		0, 0, 0, 0,
		m_hRebar,
		(HMENU)nId,
		m_hInst,
		NULL); 
	PostMessage(hWnd, TBM_SETTHUMBLENGTH, (WPARAM)16, (LPARAM)0);
	PostMessage(hWnd, TBM_SETRANGEMAX, (WPARAM)FALSE, (LPARAM)SLIDER_DIVIDED); //���x�̐ݒ�
	return hWnd;
}


static BOOL SeekToPos(BOOL fFadeOut, QWORD qPos){
	BOOL fRet;
	int nOldStatus = OPGetStatus();
	int nNewStatus;

	if (fFadeOut) OPSetFadeOut(150);

	if(nOldStatus == OUTPUT_PLUGIN_STATUS_PLAY){
		OPStop();
	}

	fRet = TrackSetPos(qPos);

	OPSetVolume(GetVolumeBar());

	if(nOldStatus == OUTPUT_PLUGIN_STATUS_PLAY || g_cfg.nRestartOnSeek){
		OPPlay();
	}

	nNewStatus = OPGetStatus();
	if(nOldStatus != nNewStatus){
		OnStatusChangePlugins();
		if(nNewStatus == OUTPUT_PLUGIN_STATUS_PLAY){
			/* PAUSE���� */
			SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
		}
	}

	return fRet;
}

// �番���Ńt�@�C�����V�[�N
static BOOL _BASS_ChannelSetPosition(DWORD handle, int nPos){
	QWORD qPos;

	qPos = g_cInfo[g_bNow].qDuration * nPos / SLIDER_DIVIDED;
	if (qPos >= g_cInfo[g_bNow].qDuration) qPos = g_cInfo[g_bNow].qDuration - 1;

	return SeekToPos(TRUE, qPos);
}

// �b�P�ʂŃt�@�C�����V�[�N
static void _BASS_ChannelSeekSecond(DWORD handle, float fSecond, int nSign){
	QWORD qPos;
	QWORD qSeek;
	QWORD qSet;

	qPos = TrackGetPos();
	qSeek = BASS_ChannelSeconds2Bytes(handle, fSecond);
	if(nSign<0 && qPos<qSeek){
		qSet = 0;
	}else if(nSign>0 && qPos+qSeek>g_cInfo[g_bNow].qDuration){
		qSet = g_cInfo[g_bNow].qDuration-1;
	}else{
		qSet = qPos + nSign*qSeek;
	}
	SeekToPos(FALSE, qSet);
	return;
}

// �Đ����Ȃ�ꎞ��~�A�ꎞ��~���Ȃ畜�A
static int FilePause(){
	DWORD dMode;

	dMode = OPGetStatus();
	if(dMode==OUTPUT_PLUGIN_STATUS_PLAY){
		OPSetFadeOut(250);
		OPPause();
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(TRUE, 0));
	}else if(dMode==OUTPUT_PLUGIN_STATUS_PAUSE){
		OPPlay();
		OPSetFadeIn(GetVolumeBar(), 250);
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	}else{
		OPSetVolume(GetVolumeBar());
		SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	}

	// �v���O�C�����Ă�
	OnStatusChangePlugins();

	return 0;
}

// �A�E�g�v�b�g�X�g���[�����~�A�\����������
static int StopOutputStream(){
	OPSetFadeOut(250);

	// �X�g���[�����폜
	KillTimer(m_hMainWnd, ID_SEEKTIMER);
	OPStop();
	OPEnd();

	FreeChannelInfo(!g_bNow);
	FreeChannelInfo(g_bNow);

	//�X���C�_�o�[
	EnableWindow(m_hSeek, FALSE);
	SendMessage(m_hSeek, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);
	//�c�[���o�[
	SendMessage(m_hToolBar,  TB_CHECKBUTTON, (WPARAM)IDM_PAUSE, (LPARAM)MAKELONG(FALSE, 0));
	
	//������\���֌W
	SetWindowText(m_hMainWnd, FITTLE_TITLE);
	lstrcpy(m_ni.szTip, FITTLE_TITLE);
	if(m_bTrayFlag)
		Shell_NotifyIcon(NIM_MODIFY, &m_ni); //ToolTip�̕ύX

	SendFittleMessage(WM_TIMER, MAKEWPARAM(ID_TIPTIMER, 0), 0);

	lstrcpy(m_szTag, TEXT(""));
	lstrcpy(m_szTime, TEXT("\t"));

	// �X�e�[�^�X�o�[�̏�����
	SetStatusbarIcon(NULL, FALSE);
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)0|0, (LPARAM)TEXT(""));
	SendMessage(m_hStatus, SB_SETTIPTEXT, (WPARAM)0, (LPARAM)TEXT(""));
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)1|0, (LPARAM)TEXT(""));
	SendMessage(m_hStatus, SB_SETTEXT, (WPARAM)2|0, (LPARAM)TEXT(""));
	// ���X�g�r���[���ĕ`��
	InvalidateRect(GetSelListTab()->hList, NULL, TRUE); 

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
		hIcon = (HICON)LoadImage(m_hInst, TEXT("MYICON"), IMAGE_ICON, 16, 16, LR_SHARED);
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
static int ShowToolTip(HWND hWnd, HWND hTitleTip, LPTSTR pszToolText){
	TOOLINFO tin;
	RECT rcTask, rcTip;
	HWND hTaskBar;
	POINT ptDraw;

	//�^�X�N�o�[�̈ʒu�A�������擾
	hTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
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
BOOL CheckFileType(LPTSTR szFilePath){
	int i=0;
	LPTSTR szCheckType;
	LPTSTR lpExt;

	szCheckType = PathFindExtension(szFilePath);
	if(!szCheckType || !*szCheckType) return FALSE;
	szCheckType++;
	do {
		lpExt = GetTypelist(i++);
		if (!lpExt[0]) return FALSE;
	} while(lstrcmpi(lpExt, szCheckType) != 0);
	return TRUE;
}

// �R�}���h���C���I�v�V�������l������Execute
static void MyShellExecute(HWND hWnd, LPTSTR pszExePath, LPTSTR pszFilePathes){
	LPTSTR pszArgs = PathGetArgs(pszExePath);

	int nSize = lstrlen(pszArgs) + lstrlen(pszFilePathes) + 5;

	LPTSTR pszBuff = (LPTSTR)HZAlloc(sizeof(TCHAR) * nSize);

	if(*pszArgs){	// �R�}���h���C���I�v�V�������l��
		*(pszArgs-1) = TEXT('\0');
		wsprintf(pszBuff, TEXT("%s %s"), pszArgs, pszFilePathes);
	}else{
		wsprintf(pszBuff, TEXT("%s"), pszFilePathes);
	}
	ShellExecute(hWnd, TEXT("open"), pszExePath, pszBuff, NULL, SW_SHOWNORMAL);
	if(*pszArgs){	// �߂���
		*(pszArgs-1) = TEXT(' ');
	}

	HFree(pszBuff);
	return;
}

// ���X�g�őI�����ꂽ�A�C�e���̃p�X��A�����ĕԂ��B�ǂ�����Free���Ă��������B
static LPTSTR MallocAndConcatPath(LISTTAB *pListTab){
	DWORD s = sizeof(TCHAR) * 2;
	LPTSTR pszRet = (LPTSTR)HZAlloc(s);
	LPTSTR pszNew;

	int i = -1;
	while( (i = LV_GetNextSelect(pListTab->hList, i)) != -1 ){
		LPTSTR pszTarget = GetPtrFromIndex(pListTab->pRoot, i)->szFilePath;
		if(!IsURLPath(pszTarget) && !IsArchivePath(pszTarget)){	// �폜�ł��Ȃ��t�@�C���łȂ����
			s += (lstrlen(pszTarget) + 3) * sizeof(TCHAR);
			pszNew = (LPTSTR)HRealloc(pszRet, s);
			if (!pszNew) {
				HFree(pszRet);
				return NULL;
			}
			pszRet = pszNew;
			lstrcat(pszRet, TEXT("\""));
			lstrcat(pszRet, pszTarget);
			lstrcat(pszRet, TEXT("\" "));
		}
	}
	return pszRet;
}

// �h���b�O�C���[�W�쐬�A�\��
static void OnBeginDragList(HWND hLV, POINT pt){
	m_hDrgImg = ListView_CreateDragImage(hLV, LV_GetNextSelect(hLV, -1), &pt);
	ImageList_BeginDrag(m_hDrgImg, 0, 0, 0);
	ImageList_DragEnter(hLV, pt.x, pt.y);
	SetCapture(hLV);
	return;
}

// �h���b�O�C���[�W�𓮂����Ȃ���LVIS_DROPHILITED��\��
static void OnDragList(HWND hLV, POINT pt){
	int nHitItem;
	RECT rcLV;

	ImageList_DragShowNolock(FALSE);
	LV_ClearHilite(hLV);
	nHitItem = LV_HitTest(hLV, MAKELONG(pt.x, pt.y));
	if(nHitItem!=-1) LV_SetState(hLV, nHitItem, LVIS_DROPHILITED);
	UpdateWindow(hLV);
	ImageList_DragShowNolock(TRUE);
	ImageList_DragMove(pt.x, pt.y);

	GetClientRect(hLV, &rcLV);
	if(pt.y >=0 && pt.y <= GetSystemMetrics(SM_CYHSCROLL)){
		//��X�N���[��
		SetTimer(m_hTab, ID_SCROLLTIMERUP, 100, NULL);
	}else if(pt.y >= rcLV.bottom - GetSystemMetrics(SM_CYHSCROLL)){
		//���X�N���[��
		SetTimer(m_hTab, ID_SCROLLTIMERDOWN, 100, NULL);
	}else{
		KillTimer(m_hTab, ID_SCROLLTIMERUP);
		KillTimer(m_hTab, ID_SCROLLTIMERDOWN);
	}
}

// �h���b�O�C���[�W�폜
static void OnEndDragList(HWND hLV){
	KillTimer(m_hTab, ID_SCROLLTIMERUP);
	KillTimer(m_hTab, ID_SCROLLTIMERDOWN);
	LV_ClearHilite(hLV);
	ImageList_DragLeave(hLV);
	ImageList_EndDrag();
	ImageList_Destroy(m_hDrgImg);
	m_hDrgImg = NULL;
	ReleaseCapture();
}

static void DrawTabFocus(int nIdx, BOOL bFlag){
	RECT rcItem;
	HDC hDC;

	struct LISTTAB *pList = GetListTab(m_hTab, nIdx);

	if((bFlag && !pList->bFocusRect)
	|| (!bFlag && pList->bFocusRect)){
		TabGetItemRect(nIdx, &rcItem);
		rcItem.left += 1;
		rcItem.top += 1;
		rcItem.right -= 1;
		rcItem.bottom -= 1;
		hDC = GetDC(m_hTab);
		DrawFocusRect(hDC, &rcItem);
		pList->bFocusRect = !pList->bFocusRect;
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
	if(WAstrlen(&g_cfg.szFontName)){
		TCHAR fontname[LF_FACESIZE];
		WAstrcpyt(fontname, &g_cfg.szFontName, LF_FACESIZE);
		hDC = GetDC(m_hTree);
		nPitch = -MulDiv(g_cfg.nFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		ReleaseDC(m_hTree, hDC);
		m_hFont = CreateFont(nPitch, 0, 0, 0, (g_cfg.nFontStyle&BOLD_FONTTYPE?FW_BOLD:0),
			(DWORD)(g_cfg.nFontStyle&ITALIC_FONTTYPE?TRUE:FALSE), FALSE, FALSE, (DWORD)DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
			fontname);
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
	for(int i=0;i<TabGetListCount();i++){
		SendMessage(GetListTab(m_hTab, i)->hList, WM_SETFONT, (WPARAM)m_hFont, (LPARAM)FALSE);
	}
	// �R���{�{�b�N�X�̃T�C�Y����
	UpdateWindowSize(m_hMainWnd);
	return TRUE;
}

static void SetStatusbarIcon(LPTSTR pszPath, BOOL bShow){
	static HICON s_hIcon = NULL;
	SHFILEINFO shfinfo = {0};
	TCHAR szIconPath[MAX_FITTLE_PATH] = {0};

	if(s_hIcon){
		DestroyIcon(s_hIcon);
		s_hIcon = NULL;
	}
	if(bShow){
		s_hIcon = IsArchivePath(pszPath) ? GetArchiveItemIcon(pszPath) : NULL;
		if (!s_hIcon){
			lstrcpyn(szIconPath, pszPath, MAX_FITTLE_PATH);
			SHGetFileInfo(szIconPath, FILE_ATTRIBUTE_NORMAL, &shfinfo, sizeof(shfinfo), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON); 
			s_hIcon = shfinfo.hIcon;
		}
		SendMessage(m_hStatus, SB_SETICON, (WPARAM)0, (LPARAM)s_hIcon);
	}else{
		SendMessage(m_hStatus, SB_SETICON, (WPARAM)0, (LPARAM)NULL);
	}
	return;
}

static BOOL SetUIColor(void){
	HWND hList;
	for(int i=0;i<TabGetListCount();i++){
		hList = GetListTab(m_hTab, i)->hList;
		SetListColor(hList);
	}
	TreeSetColor();
	InitTreeIconIndex(m_hCombo, m_hTree, g_cfg.nTreeIcon != 0);
	InvalidateRect(GetSelListTab()->hList, NULL, TRUE);
	return TRUE;
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
				SendFittleMessage(WM_CONTEXTMENU, (WPARAM)m_hRebar, lp);
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
			TCHAR szDrawBuff[32];

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
					fLen = TrackPosToSec(g_cInfo[g_bNow].qDuration);
					wsprintf(szDrawBuff, TEXT("%02d:%02d"), (nPos * (int)fLen / SLIDER_DIVIDED)/60, (nPos * (int)fLen / SLIDER_DIVIDED)%60);
				}else if(hSB==m_hVolume){
					wsprintf(szDrawBuff, TEXT("%02d%%"), nPos/10);
				}

				//�c�[���`�b�v�����A�\��
				ZeroMemory(&tin, sizeof(tin));
				tin.cbSize = sizeof(tin);
				tin.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
				tin.hwnd = m_hMainWnd;
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
					OPSetVolume(nPos);
				}

			}
			break;

		case WM_MOUSEWHEEL:
			if(hSB==m_hSeek){
				if((short)HIWORD(wp)<0){
					SendFittleCommand(IDM_SEEKFRONT);
				}else{
					SendFittleCommand(IDM_SEEKBACK);
				}
			}else if(hSB==m_hVolume){
				if((short)HIWORD(wp)<0){
					SetVolumeBar(GetVolumeBar() - SLIDER_DIVIDED/50);
				}else{
					SetVolumeBar(GetVolumeBar() + SLIDER_DIVIDED/50);
				}
			}else
				break;
			return 0;

		case WM_HSCROLL:
			return 0; // �`���g�z�C�[���}�E�X���~����

		case WM_MBUTTONDOWN:	// ���{�^���N���b�N
			if(hSB==m_hSeek){
				FilePause();	// �|�[�Y�ɂ��Ă݂�
			}else if(hSB==m_hVolume){
				SetVolumeBar(0);	// �~���[�g�ɂ��Ă݂�
			}
			return 0; // �t�H�[�J�X�𓾂�̂�h��

		case TBM_SETPOS:
			if(hSB==m_hVolume){
				OPSetVolume(lp);
			}
			break;

		default:
			break;
	}
	return SubClassCallNext(hSB, msg, wp, lp);
}

// �����o�[�̐V�����v���V�[�W��
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
				UpdateWindowSize(m_hMainWnd);
				s_nPreX = pt.x;
			}
			break;

		case WM_MOUSELEAVE:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			break;

		case WM_LBUTTONDOWN:
			if(g_cfg.nTreeState==TREE_SHOW){
				// �}�E�X�̈ړ��͈͂𐧌�
				GetWindowRect(m_hMainWnd, &rcDisp);
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
			UpdateWindowSize(m_hMainWnd);
			MyScroll(m_hTree);
			*/
			SendFittleCommand(IDM_TOGGLETREE);
			break;
	}
	return SubClassCallNext(hSB, msg, wp, lp);
}

static int GetListIndex(HWND hwndList){
	int i;
	for(i=0;i<TabGetListCount();i++){
		if (hwndList == GetListTab(m_hTab, i)->hList)
			return i;
	}
	return -1;
}

// �^�u�̐V�����v���V�[�W��
static LRESULT CALLBACK NewTabProc(HWND hTC, UINT msg, WPARAM wp, LPARAM lp){
	static int s_nDragTab = -1;

	switch(msg)
	{
		case WM_LBUTTONDOWN:
			s_nDragTab = TabHitTest(lp, TCHT_NOWHERE);
			if(s_nDragTab>0 && (TabGetRowCount() < 2 || TabGetListSel() == s_nDragTab)){
				SetCapture(hTC);
			}
			break;

		case WM_LBUTTONUP:
			s_nDragTab = -1;
			if(GetCapture()==hTC){
				ReleaseCapture();
			}
			break;

		case WM_MOUSEMOVE:
			if(GetCapture()==hTC){
				int nHitTab = TabHitTest(lp, TCHT_NOWHERE);
				if(nHitTab>0 && nHitTab!=s_nDragTab){
					MoveTab(hTC, s_nDragTab, nHitTab-s_nDragTab);
					s_nDragTab = nHitTab;
				}
			}
			break;

		case WM_CONTEXTMENU:	// ���X�g�E�N���b�N���j���[
			{
				TCHAR szMenu[MAX_FITTLE_PATH+4];
				RECT rcItem;
				HMENU hPopMenu;
				int nItem;
				int i;

				switch(GetDlgCtrlID((HWND)wp)){
					case ID_LIST:
						if(lp==-1){	//�L�[�{�[�h
							nItem = LV_GetNextSelect((HWND)wp, -1);
							if(nItem<0) return 0; 
							ListView_GetItemRect((HWND)wp, nItem, &rcItem, LVIR_SELECTBOUNDS);
							MapWindowPoints((HWND)wp, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
							lp = MAKELPARAM(rcItem.left, rcItem.bottom);
						}
						hPopMenu = GetSubMenu(LoadMenu(m_hInst, TEXT("LISTMENU")), 0);

						for(i=0;i<TabGetListCount();i++){
							wsprintf(szMenu, TEXT("&%d: %s"), i, GetListTab(hTC, i)->szTitle);
							AppendMenu(GetSubMenu(hPopMenu, GetMenuPosFromString(hPopMenu, TEXT("�v���C���X�g�ɑ���(&S)"))), MF_STRING, IDM_SENDPL_FIRST+i, szMenu); 
						}

						/*if(ListView_GetSelectedCount((HWND)wp)!=1){
							EnableMenuItem(hPopMenu, IDM_LIST_DELFILE, MF_GRAYED | MF_DISABLED);
						}*/
						TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, (short)LOWORD(lp), (short)HIWORD(lp), 0, m_hMainWnd, NULL);
						DestroyMenu(hPopMenu);
						return 0;	// �^�u�R���g���[���̕���WM_CONTEXTMENU��������̂�h��
				}
			}
			break;

		case WM_NOTIFY:
			{
				LPNMLISTVIEW lplv;
				NMHDR *pnmhdr = (LPNMHDR)lp;

				switch(pnmhdr->idFrom)
				{
					case ID_LIST:
						lplv = (LPNMLISTVIEW)lp;
						switch(pnmhdr->code)
						{

							// ���X�g�r���[����̗v��
							/* Windows�̋������ُ�(A/W���K�؂ɌĂ΂�Ȃ�) */
							case LVN_GETDISPINFOA:
							case LVN_GETDISPINFOW:
								{
									LVITEM *item = &((NMLVDISPINFO*)lp)->item;
									// �e�L�X�g���Z�b�g
									if(item->mask & LVIF_TEXT){
										LISTTAB *pList = GetListTab(m_hTab, GetListIndex(pnmhdr->hwndFrom));
										if (pList){
											GetColumnText(pList->pRoot, item->iItem, item->iSubItem, item->pszText, item->cchTextMax);
										}
									}
								}
								break;

							case LVN_GETINFOTIPA:
								if(g_cfg.nPathTip){
									NMLVGETINFOTIPA *lvgit = (NMLVGETINFOTIPA *)lp;
									LPTSTR pszFilePath = GetPtrFromIndex(GetSelListTab()->pRoot, lvgit->iItem)->szFilePath;
									lvgit->dwFlags = 0;
									lstrcpyntA(lvgit->pszText, pszFilePath, lvgit->cchTextMax);
								}
								break;
							case LVN_GETINFOTIPW:
								if(g_cfg.nPathTip){
									NMLVGETINFOTIPW *lvgit = (NMLVGETINFOTIPW *)lp;
									LPTSTR pszFilePath = GetPtrFromIndex(GetSelListTab()->pRoot, lvgit->iItem)->szFilePath;
									lvgit->dwFlags = 0;
									lstrcpyntW(lvgit->pszText, pszFilePath, lvgit->cchTextMax);
								}
								break;
							case NM_CUSTOMDRAW:
								NMLVCUSTOMDRAW *lplvcd;
								LOGFONT lf;

								lplvcd = (LPNMLVCUSTOMDRAW)lp;
								if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
									return CDRF_NOTIFYITEMDRAW;
								if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
									if(m_nPlayTab==TabGetListSel()
										&& GetSelListTab()->nPlaying==(signed int)lplvcd->nmcd.dwItemSpec
										&& OPGetStatus() != OUTPUT_PLUGIN_STATUS_STOP){
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

								if(lplv->iSubItem+1==abs(GetSelListTab()->nSortState)){
									nSort = -1*GetSelListTab()->nSortState;
								}else{
									nSort = lplv->iSubItem+1;
								}
								SortListTab(GetSelListTab(), nSort);
								break;

							case NM_DBLCLK:
								SendFittleCommand(IDM_PLAY);
								break;

						}
						break;
				}
			}
			break;

		case WM_COMMAND:
			SendFittleMessage(msg, wp, lp); //�e�E�B���h�E�ɂ��̂܂ܑ��M
			break;

		/* case WM_UNICHAR: */
		case WM_CHAR:
			if((int)wp == '\t'){
				if(GetKeyState(VK_SHIFT) < 0){
					SetFocus(m_hTree);
				}else{
					SetFocus(GetSelListTab()->hList);
				}
				return 0;
			}
			break;

		case WM_TIMER:
			switch (wp){
				case ID_SCROLLTIMERUP:		// ��X�N���[��
					ListView_Scroll(GetSelListTab()->hList, 0, -10);
					break;

				case ID_SCROLLTIMERDOWN:	// ���X�N���[��
					ListView_Scroll(GetSelListTab()->hList, 0, 10);
					break;
			}
			break;

		case WM_MOUSEWHEEL:
			{
				int ic = TabGetListCount();
				int cs = TabGetListSel();
				TabSetListFocus(((short)HIWORD(wp)<0) ? (ic == cs+1 ? 0 : cs+1) : (cs==0 ? ic-1 : cs-1));
			}
			break;

		case WM_LBUTTONDBLCLK:
			m_nPlayTab = TabGetListSel();
			SendFittleCommand(IDM_NEXT);
			break;

		case WM_DROPFILES:
			{
				TCHAR szLabel[MAX_FITTLE_PATH];
				TCHAR szPath[MAX_FITTLE_PATH];
				RECT rcItem = {0};
				POINT pt;
				struct FILEINFO *pSub;
				int i;

				// �h���b�O���ꂽ�t�@�C����ǉ�
				int nFileCount = GetDropFiles((HDROP)wp, &pSub, &pt, szPath);

				// �����̃^�u�ɒǉ�
				int nItem = TabGetListCount();
				for(i=0;i<nItem;i++){
					TabGetItemRect(i, &rcItem);
					if(PtInRect(&rcItem, pt)){
						AppendToList(GetListTab(hTC, i), pSub);
						TabSetListFocus(i);
						return 0;
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
					lstrcpy(szLabel, TEXT("Default"));
				}
				if(pt.x>rcItem.right && pt.y>=rcItem.top && pt.y<=rcItem.bottom){
					struct LISTTAB *pList;
					MakeNewTab(hTC, szLabel, -1);
					pList = GetListTab(hTC, nItem);
					InsertList(pList, -1, pSub);
					LV_SingleSelectView(pList->hList, 0);
					TabSetListFocus(nItem);
				}
			}
			break;

		default:
			break;
	}
	return SubClassCallNext(hTC, msg, wp, lp);
}

// �^�u�t�H�[�J�X�̕`��
static void DrawTabsFocus(POINT pt){
	int i;
	RECT rcItem;
	ScreenToClient(m_hTab, &pt);
	for(i=0;i<TabGetListCount();i++){
		TabGetItemRect(i, &rcItem);
		if(PtInRect(&rcItem, pt)){
			DrawTabFocus(i, TRUE);
		}else{
			DrawTabFocus(i, FALSE);
		}
	}
}

// ���X�g�r���[�̐V�����v���V�[�W��
LRESULT CALLBACK NewListProc(HWND hLV, UINT msg, WPARAM wp, LPARAM lp){
	switch (msg) {
		case WM_MOUSEMOVE:
			if(GetCapture()==hLV){
				POINT pt;
				HWND hWnd;
				// �J�[�\����ύX
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);
				if(hWnd==m_hTab){
					SetOLECursor(3);
				}else if(hWnd==GetSelListTab()->hList){
					SetOLECursor(2);
				}else{
					SetOLECursor(1);
				}
				// �^�u�t�H�[�J�X�̕`��
				DrawTabsFocus(pt);
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
			if(GetCapture()==hLV){
				int nBefore, nAfter;
				OnEndDragList(hLV); //�h���b�O�I��
				//�ړ��O�A�C�e���ƈړ���A�C�e���̃C���f�b�N�X���擾
				nBefore = LV_GetNextSelect(hLV, -1);
				nAfter = LV_HitTest(hLV, lp);
				if(nAfter!=-1 && nBefore!=nAfter){
					ChangeOrder(GetSelListTab(), nAfter);
				}else{
					//�^�u�փh���b�O
					int i;
					POINT pt;
					GetCursorPos(&pt);
					ScreenToClient(m_hTab, &pt);
					for(i=0;i<TabGetListCount();i++){
						RECT rcItem;
						TabGetItemRect(i, &rcItem);
						if(PtInRect(&rcItem, pt)){
							DrawTabFocus(i, FALSE);
							SendToPlaylist(GetSelListTab(), GetListTab(m_hTab, i));
							//nCount = LV_GetCount(GetListTab(m_hTab ,i)->hList) - 1;
							//LV_SingleSelectView(GetListTab(m_hTab ,i)->hList, nCount);
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
					SendFittleCommand(IDM_PLAY);
					break;

				case VK_DELETE:
					DeleteFiles(GetSelListTab());
					break;

				case VK_TAB:
					if(g_cfg.nTreeState!=TREE_SHOW) break;
					if(GetKeyState(VK_SHIFT) < 0){
						SetFocus(m_hTab);
					}else{
						SetFocus(m_hCombo);
					}
					break;

				case VK_DOWN:
					int nToIndex;
					if(GetKeyState(VK_CONTROL) < 0){
						nToIndex = LV_GetNextSelect(hLV, -1);
						do{
							nToIndex++;
						}while(ListView_GetItemState(hLV, nToIndex, LVIS_SELECTED));
						ChangeOrder(GetSelListTab(), nToIndex);
						return 0;
					}
					break;

				case VK_UP:
					if(GetKeyState(VK_CONTROL) < 0){
						nToIndex = LV_GetNextSelect(hLV, -1);
						if(nToIndex>0){
							ChangeOrder(GetSelListTab(), nToIndex-1);
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
						TreeView_EnsureVisible(m_hTree, TreeGetSelection());
						MyScroll(m_hTree);
					}
					break;

				default:
					break;
			}
			break;

		case WM_DROPFILES:
			{
				struct FILEINFO *pSub = NULL;
				TCHAR szPath[MAX_FITTLE_PATH];
				GetDropFiles((HDROP)wp, &pSub, 0, szPath);

				AppendToList(GetSelListTab(), pSub);
				SetForegroundWindow(m_hMainWnd);
			}
			break;

		default:
			break;
	}
	return SubClassCallNext(hLV, msg, wp, lp);
}

// �c���[�r���[�̃v���V�[�W��
static LRESULT CALLBACK NewTreeProc(HWND hTV, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_LBUTTONDBLCLK:	// �q�������Ȃ��m�[�h�_�u���N���b�N�ōĐ�
			if(!TreeView_GetChild(hTV, TreeGetSelection())){
				m_nPlayTab = 0;
				SendFittleCommand(IDM_NEXT);
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
					if(LV_GetCount(GetFolderListTab()->hList)>0){
						m_nPlayTab = 0;
						SendFittleCommand(IDM_NEXT);
					}else{
						TreeView_Expand(hTV, TreeGetSelection(), TVE_TOGGLE);
					}
					return 0;

				default:
					break;
			}
			break;

		case WM_KEYDOWN:
			if(wp==VK_RIGHT){	// �q�m�[�h���Ȃ� or �W�J��ԂȂ��List�Ƀt�H�[�J�X���ڂ�
				TVITEM tvi;
				HTREEITEM hti = TreeGetSelection();
				tvi.mask = TVIF_STATE;
				tvi.hItem = hti;
				tvi.stateMask = TVIS_EXPANDED;
				TreeView_GetItem(hTV, &tvi);

				if(!TreeView_GetChild(hTV, hti) || (tvi.state & TVIS_EXPANDED)){
					SetFocus(GetSelListTab()->hList);
					return 0;
				}
				break;
			}
			break;

		case WM_LBUTTONUP:
			if(GetCapture()==hTV){
				POINT pt;
				HWND hWnd;
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);
				if(hWnd==m_hTab){
					int nHitTab;
					struct FILEINFO *pSub = NULL;
					TCHAR szNowDir[MAX_FITTLE_PATH];
					GetPathFromNode(hTV, m_hHitTree, szNowDir);
					SearchFiles(&pSub, szNowDir, TRUE);
					nHitTab = TabHitTest(POINTTOPOINTS(pt), 0x40 | TCHT_ONITEM);
					if(nHitTab!=-1){
						DrawTabFocus(nHitTab, FALSE);
						AppendToList(GetListTab(m_hTab, nHitTab), pSub);
					}
				}else if(hWnd==GetSelListTab()->hList){
					SendFittleCommand(IDM_TREE_ADD);
				}else if(hWnd==m_hMainWnd){
					RECT rcTab;
					RECT rcList;
					// �V�K�^�u
					GetWindowRect(GetSelListTab()->hList, &rcList);
					GetWindowRect(m_hTab, &rcTab);

					if(g_cfg.nTabBottom==0 && pt.y>=rcTab.top && pt.y<=rcList.top && pt.x>=rcTab.left
					|| g_cfg.nTabBottom==1 && pt.y<=rcTab.bottom && pt.y>=rcList.bottom && pt.x>=rcTab.left){
						SendFittleCommand(IDM_TREE_NEW);
					}
				}
				ReleaseCapture();
			}
			break;

		case WM_RBUTTONUP:
			if(GetCapture()==hTV){
				int i;
				ReleaseCapture();
				for(i=0;i<TabGetListCount();i++){
					DrawTabFocus(i, FALSE);
				}
			}
			break;

		case WM_MOUSEMOVE:
			if(GetCapture()==hTV){
				POINT pt;
				HWND hWnd;
				HWND hCurList = GetSelListTab()->hList;
				GetCursorPos(&pt);
				hWnd = WindowFromPoint(pt);

				if(hWnd==m_hTab || hWnd==hCurList){
					SetOLECursor(3);
				}else if(hWnd == m_hMainWnd){
					RECT rcTab;
					RECT rcList;
					// �V�K�^�u
					GetWindowRect(hCurList, &rcList);
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
				
				DrawTabsFocus(pt);
			}
			break;

		case WM_DROPFILES:
			{
				HDROP hDrop;
				TCHAR szPath[MAX_FITTLE_PATH];
				TCHAR szParDir[MAX_PATH];

				hDrop = (HDROP)wp;
				DragQueryFile(hDrop, 0, szPath, MAX_FITTLE_PATH);
				DragFinish(hDrop);

				GetParentDir(szPath, szParDir);
				MakeTreeFromPath(hTV, m_hCombo, szParDir);

				SetForegroundWindow(m_hMainWnd);
			}
			break;

	}
	return SubClassCallNext(hTV, msg, wp, lp);
}

// �ݒ�t�@�C����ǂݒ����K�p����(�ꕔ�ݒ������)
static void ApplyConfig(HWND hWnd){
	TCHAR szNowDir[MAX_FITTLE_PATH];
	BOOL fIsIconic = IsIconic(hWnd);
	BOOL fOldTrayVisible = m_bTrayFlag;
	BOOL fNewTrayVisible = FALSE;
	int nCurTab;

	UnRegHotKey(hWnd);

	OnConfigChangePlugins();
	LoadConfig();


	if(g_cfg.nHighTask){
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);		
	}else{
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);	
	}

	if(g_cfg.nTreeIcon) RefreshComboIcon(m_hCombo);
	InitTreeIconIndex(m_hCombo, m_hTree, g_cfg.nTreeIcon != 0);

	SetUIColor();
	SetUIFont();

	if (g_cfg.nTrayOpt == 0){
		fNewTrayVisible = FALSE;
		if (!IsWindowVisible(hWnd))
			ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
	} else {
		if (g_cfg.nTrayOpt == 1){
			fNewTrayVisible = !IsWindowVisible(hWnd) || fIsIconic;
		}else{
			fNewTrayVisible = TRUE;
		}
		if (fIsIconic)
			ShowWindow(hWnd, SW_HIDE);
	}

	if (fOldTrayVisible && !fNewTrayVisible){
		Shell_NotifyIcon(NIM_DELETE, &m_ni);
		m_bTrayFlag = FALSE;
	}
	if (!fOldTrayVisible && fNewTrayVisible){
			SetTaskTray(m_hMainWnd);
	}

	nCurTab = TabGetListSel();

	lstrcpyn(szNowDir, m_szTreePath, MAX_FITTLE_PATH);
	MakeTreeFromPath(m_hTree, m_hCombo, szNowDir); 

	TabSetListFocus(nCurTab);

	RegHotKey(hWnd);
	InvalidateRect(hWnd, NULL, TRUE);

	OPSetVolume(GetVolumeBar());
}

// �}�E�X�z�C�[����������J�[�\���ʒu�̑��Ɉ����n��
static LRESULT CALLBACK MyHookProc(int nCode, WPARAM wp, LPARAM lp){
	MSG *msg;
	msg = (MSG *)lp;

	if(nCode<0)
		return CallNextHookEx(m_hHook, nCode, wp, lp);

	if(msg->message==WM_MOUSEWHEEL){
#define WHEEL_TYPE 2
#if (WHEEL_TYPE == 1)
		/* Fittle��݊� taskvol���ʈ��� */
		HWND hPosWnd = WindowFromPoint(msg->pt);
		if (hPosWnd){
			BOOL fMatch = FALSE;
			DWORD dwPosProcessId = 0;
			DWORD dwPosThreadId = GetWindowThreadProcessId(hPosWnd, &dwPosProcessId);
			if (GetCurrentProcessId() == dwPosProcessId) {
				fMatch = TRUE;
			} else if (GetModuleHandleA("taskvol.dll")){
				HWND hWndTaskTray = FindWindowA("Shell_TrayWnd", NULL);
				if (hWndTaskTray && (hWndTaskTray == hPosWnd || IsChild(hWndTaskTray, hPosWnd)))
					fMatch = TRUE;
			}
			if (fMatch) {
				SendMessage(hPosWnd, WM_MOUSEWHEEL, msg->wParam, msg->lParam);
				msg->message = WM_NULL;
			}
		}
#elif (WHEEL_TYPE == 2)
		/* Fittle�݊� �������[�v���� */
		HWND hPosWnd = WindowFromPoint(msg->pt);
		if (hPosWnd){
			DWORD dwPosProcessId = 0;
			DWORD dwPosThreadId = GetWindowThreadProcessId(hPosWnd, &dwPosProcessId);
			msg->message = WM_NULL;
			if (GetCurrentProcessId() == dwPosProcessId) {
				SendMessage(hPosWnd, WM_MOUSEWHEEL, msg->wParam, msg->lParam);
			} else {
				PostMessage(hPosWnd, WM_MOUSEWHEEL, msg->wParam, msg->lParam);
			}
		}
#else
		/* Fittle�݊� */
		SendMessage(WindowFromPoint(msg->pt), WM_MOUSEWHEEL, msg->wParam, msg->lParam);
		msg->message = WM_NULL;
#endif
	}

	return CallNextHookEx(m_hHook, nCode, wp, lp);
}

// �I�������t�@�C�������ۂɍ폜����
static void RemoveFiles(){
	// ������
	SHFILEOPSTRUCT fops;
	TCHAR szMsg[MAX_FITTLE_PATH];
	int i, j, q, s;
	LPTSTR pszTarget;
	BOOL bPlaying;
	LPTSTR p, np;
	struct LISTTAB *pCurList = GetSelListTab();

	bPlaying = FALSE;
	j = 0;
	q = 0;
	s = 2 * sizeof(TCHAR);
	p = (LPTSTR)HZAlloc(s);
	if (!p) return;

	// �p�X��A���Ȃ�
	i = LV_GetNextSelect(pCurList->hList, -1);
	while(i!=-1){
		pszTarget = GetPtrFromIndex(pCurList->pRoot, i)->szFilePath;
		if(i==pCurList->nPlaying) bPlaying = TRUE;	// �폜�t�@�C�������t��
		if(!IsURLPath(pszTarget) && !IsArchivePath(pszTarget)){	// �폜�ł��Ȃ��t�@�C���łȂ����
			int l = lstrlen(pszTarget);
			j++;
			s += (l + 1) * sizeof(TCHAR);
			np = (LPTSTR)HRealloc(p, s);
			if (!np) {
				HFree(p);
				return;
			}
			p = np;
			lstrcpy(p+q, pszTarget);
			q += l + 1;
			*(p+q) = TEXT('\0');
		}
		i = LV_GetNextSelect(pCurList->hList, i);
	}
	if(j==1){
		// �t�@�C����I��
		wsprintf(szMsg, TEXT("%'%s%' �����ݔ��Ɉڂ��܂����H"), p);
	}else if(j>1){
		// �����t�@�C���I��
		wsprintf(szMsg, TEXT("������ %d �̍��ڂ����ݔ��Ɉڂ��܂����H"), j);
	}
	if(j>0 && MessageBox(m_hMainWnd, szMsg, TEXT("�t�@�C���̍폜�̊m�F"), MB_YESNO | MB_ICONQUESTION)==IDYES){
		// ���ۂɍ폜
		fops.hwnd = m_hMainWnd;
		fops.wFunc = FO_DELETE;
		fops.pFrom = p;
		fops.pTo = NULL;
		fops.fFlags = FOF_FILESONLY | FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_MULTIDESTFILES;
		if(bPlaying) SendFittleCommand(IDM_STOP);
		if(SHFileOperation(&fops)==0){
			if(bPlaying) PopPrevious(pCurList);	// ��������Ƃ肠�����Ōゾ���폜
			DeleteFiles(pCurList);
		}
		if(bPlaying) SendFittleCommand(IDM_NEXT);
	}
	HFree(p);
}


static LRESULT CALLBACK NewComboEditProc(HWND hEB, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_CHAR:
			if((int)wp=='\t'){
				if(GetKeyState(VK_SHIFT) < 0){
					SetFocus(GetWindow(m_hTab, GW_CHILD));
				}else{
					SetFocus(m_hTree);
				}
				return 0;
			}
			break;
	}
	return SubClassCallNext(hEB, msg, wp, lp);
}



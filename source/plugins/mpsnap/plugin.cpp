/*
 * mpsnap
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "plugin.h"
#include <vector>

BOOL OnInit();
void OnQuit();
void OnTrackChange();
void OnStatusChange();
void OnConfig();

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

#define IDM_MINIPANEL 40159

static WNDPROC hOldProc;
static WNDPROC hOldMiniPanelProc;

typedef struct _LINE{
	POINT	start;
	int		len;
} LINE;

#define Abs(v) (((v) >= 0) ? (v) : -(v))

static std::vector<LINE> TopEdge;
static std::vector<LINE> BottomEdge;
static std::vector<LINE> LeftEdge;
static std::vector<LINE> RightEdge;

static int		g_nSnapRange = 0;
static RECT	g_Offsetrc = { 0 };
static BOOL	g_bSnapHide = FALSE;
static BOOL	g_bDesktopOut = FALSE;

static HWND g_MovinWnd = NULL;	//移動中のウィンドウ
static HWND g_hMDIwnd = NULL;	//移動中のウィンドウがMDIの子ウィンドウならMDIのクライアントウィンドウ

void ScreenToClientRect(HWND hWnd, LPRECT lprc)
{
	POINT pt;

	pt.x = lprc->left;
	pt.y = lprc->top;
	ScreenToClient(hWnd, &pt);
	lprc->left = pt.x;
	lprc->top = pt.y;

	pt.x = lprc->right;
	pt.y = lprc->bottom;
	ScreenToClient(hWnd, &pt);
	lprc->right = pt.x;
	lprc->bottom = pt.y;
}

void ClientToScreenRect(HWND hWnd, LPRECT lprc)
{
	POINT pt;

	pt.x = lprc->left;
	pt.y = lprc->top;
	ClientToScreen(hWnd, &pt);
	lprc->left = pt.x;
	lprc->top = pt.y;

	pt.x = lprc->right;
	pt.y = lprc->bottom;
	ClientToScreen(hWnd, &pt);
	lprc->right = pt.x;
	lprc->bottom = pt.y;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	char szWindowText[128],szClassName[128];
	GetWindowText(hWnd, szWindowText, 128);
	GetClassName(hWnd, szClassName, 128);

	if( (IsWindowVisible(hWnd))					&&	//  可視状態か
		(!IsIconic(hWnd))						&&	//  最小化状態でないか
		(lstrlen(szWindowText) > 0)				&&	//  キャプションを持っているか
		(lstrcmp(szClassName, "Progman") != 0)	&&	//  シェルでないか
		(g_MovinWnd != hWnd)						//	移動中のウィンドウでないか
	){
		if(g_hMDIwnd)
		{
			if(g_hMDIwnd != GetParent(hWnd))
				return TRUE;
		}

		if(IsZoomed(hWnd))
		{
			if(g_bSnapHide == FALSE)
				return FALSE;
			else
				return TRUE;
		}

		((std::vector<HWND> *)lParam)->push_back(hWnd);
	}

	return TRUE;
}

BOOL GetAllWindowEdge(HWND hWnd)
{
	RECT rc;
	LINE ln;
	std::vector<HWND> wnd;

	if(g_hMDIwnd == NULL)
		EnumWindows(EnumWindowsProc, (LPARAM)&wnd);
	else
		EnumChildWindows(GetParent(hWnd), EnumWindowsProc, (LPARAM)&wnd);

	std::vector<HWND>::reverse_iterator ritr;
	
	for(ritr = wnd.rbegin(); ritr != wnd.rend(); ++ritr)
	{
		GetWindowRect(*ritr, &rc);
		if((rc.bottom - rc.top) == 0 && (rc.right - rc.left) == 0)
			continue;

		if(g_hMDIwnd)
			ScreenToClientRect(g_hMDIwnd, &rc);

		if(g_bSnapHide == FALSE)
		{
			BOOL fErase;

			for(int i = 0, size = TopEdge.size(); i < size; i++)
			{
				fErase = FALSE;

				if(rc.top <= TopEdge[i].start.y && TopEdge[i].start.y <= rc.bottom)
				{
					if(rc.left <= TopEdge[i].start.x && TopEdge[i].start.x + TopEdge[i].len <= rc.right)
						fErase = TRUE;

					if(TopEdge[i].start.x <= rc.left && rc.left <= TopEdge[i].start.x + TopEdge[i].len)
					{
						ln.start.x = TopEdge[i].start.x;
						ln.start.y = TopEdge[i].start.y;
						ln.len = rc.left - TopEdge[i].start.x;
						TopEdge.push_back(ln);
						fErase = TRUE;
					}

					if(TopEdge[i].start.x <= rc.right && rc.right <= TopEdge[i].start.x + TopEdge[i].len)
					{
						ln.start.x = rc.right;
						ln.start.y = TopEdge[i].start.y;
						ln.len = TopEdge[i].start.x + TopEdge[i].len - rc.right;
						TopEdge.push_back(ln);
						fErase = TRUE;
					}

					if(fErase)
						TopEdge.erase(TopEdge.begin() + i), i--, size--;
				}
			}

			for(int i = 0, size = BottomEdge.size(); i < size; i++)
			{
				fErase = FALSE;

				if(rc.top <= BottomEdge[i].start.y && BottomEdge[i].start.y <= rc.bottom)
				{
					if(rc.left <= BottomEdge[i].start.x && BottomEdge[i].start.x + BottomEdge[i].len <= rc.right)
						fErase = TRUE;

					if(BottomEdge[i].start.x <= rc.left && rc.left <= BottomEdge[i].start.x + BottomEdge[i].len)
					{
						ln.start.x = BottomEdge[i].start.x;
						ln.start.y = BottomEdge[i].start.y;
						ln.len = rc.left - BottomEdge[i].start.x;
						BottomEdge.push_back(ln);
						fErase = TRUE;
					}

					if(BottomEdge[i].start.x <= rc.right && rc.right <= BottomEdge[i].start.x + BottomEdge[i].len)
					{
						ln.start.x = rc.right;
						ln.start.y = BottomEdge[i].start.y;
						ln.len = BottomEdge[i].start.x + BottomEdge[i].len - rc.right;
						BottomEdge.push_back(ln);
						fErase = TRUE;
					}

					if(fErase)
						BottomEdge.erase(BottomEdge.begin() + i), i--, size--;
				}
			}

			for(int i = 0, size = LeftEdge.size(); i < size; i++)
			{
				fErase = FALSE;

				if(rc.left <= LeftEdge[i].start.x && LeftEdge[i].start.x <= rc.right)
				{
					if(rc.top <= LeftEdge[i].start.y && LeftEdge[i].start.y + LeftEdge[i].len <= rc.bottom)
						fErase = TRUE;

					if(LeftEdge[i].start.y <= rc.top && rc.top <= LeftEdge[i].start.y + LeftEdge[i].len)
					{
						ln.start.x = LeftEdge[i].start.x;
						ln.start.y = LeftEdge[i].start.y;
						ln.len = rc.top - LeftEdge[i].start.y;
						LeftEdge.push_back(ln);
						fErase = TRUE;
					}

					if(LeftEdge[i].start.y <= rc.bottom && rc.bottom <= LeftEdge[i].start.y + LeftEdge[i].len)
					{
						ln.start.x = LeftEdge[i].start.x;
						ln.start.y = rc.bottom;
						ln.len = LeftEdge[i].start.y + LeftEdge[i].len - rc.bottom;
						LeftEdge.push_back(ln);
						fErase = TRUE;
					}

					if(fErase)
						LeftEdge.erase(LeftEdge.begin() + i), i--, size--;
				}
			}

			for(int i = 0, size = RightEdge.size(); i < size; i++)
			{
				fErase = FALSE;

				if(rc.left <= RightEdge[i].start.x && RightEdge[i].start.x <= rc.right)
				{
					if(rc.top <= RightEdge[i].start.y && RightEdge[i].start.y + RightEdge[i].len <= rc.bottom)
						fErase = TRUE;

					if(RightEdge[i].start.y <= rc.top && rc.top <= RightEdge[i].start.y + RightEdge[i].len)
					{
						ln.start.x = RightEdge[i].start.x;
						ln.start.y = RightEdge[i].start.y;
						ln.len = rc.top - RightEdge[i].start.y;
						RightEdge.push_back(ln);
						fErase = TRUE;
					}

					if(RightEdge[i].start.y <= rc.bottom && rc.bottom <= RightEdge[i].start.y + RightEdge[i].len)
					{
						ln.start.x = RightEdge[i].start.x;
						ln.start.y = rc.bottom;
						ln.len = RightEdge[i].start.y + RightEdge[i].len - rc.bottom;
						RightEdge.push_back(ln);
						fErase = TRUE;
					}

					if(fErase)
						RightEdge.erase(RightEdge.begin() + i), i--, size--;
				}
			}
		}

		ln.start.y = rc.top;
		ln.start.x = rc.left;
		ln.len = rc.right - rc.left;
		TopEdge.push_back(ln);

		ln.len = rc.bottom - rc.top;
		LeftEdge.push_back(ln);

		ln.start.x = rc.right;
		RightEdge.push_back(ln);

		ln.start.y = rc.bottom;
		ln.start.x = rc.left;
		ln.len = rc.right - rc.left;
		BottomEdge.push_back(ln);
	}

	if(g_hMDIwnd == NULL)
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
	else
		GetClientRect(GetParent(hWnd), &rc);

	ln.start.y = rc.top;
	ln.start.x = rc.left;
	ln.len = rc.right - rc.left;
	TopEdge.push_back(ln);

	ln.len = rc.bottom - rc.top;
	LeftEdge.push_back(ln);

	ln.start.x = rc.right;
	RightEdge.push_back(ln);

	ln.start.y = rc.bottom;
	ln.start.x = rc.left;
	ln.len = rc.right - rc.left;
	BottomEdge.push_back(ln);

	return TRUE;
}
void OnSizing(HWND /*hWnd*/, WPARAM edge, LPRECT prc)
{
	std::vector<LINE>::iterator itr;
	int dist = g_nSnapRange;

	if(g_hMDIwnd)
		ScreenToClientRect(g_hMDIwnd, prc);


	if(edge == WMSZ_TOP || edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT)
	{
		dist = g_nSnapRange;
		for(itr = TopEdge.begin(); itr != TopEdge.end(); ++itr)
		{
			if((*itr).start.x < (prc->right + g_nSnapRange) && (prc->left - g_nSnapRange) < ((*itr).start.x + (*itr).len))
			{
				if(Abs((*itr).start.y - prc->top) < Abs(dist))
				{
					dist = (*itr).start.y - prc->top;
				}
			}
		}

		for(itr = BottomEdge.begin(); itr != BottomEdge.end(); ++itr)
		{
			if((*itr).start.x < (prc->right + g_nSnapRange) && (prc->left - g_nSnapRange) < ((*itr).start.x + (*itr).len))
			{
				if(Abs((*itr).start.y - prc->top - g_Offsetrc.top) < Abs(dist))
				{
					dist = (*itr).start.y - prc->top - g_Offsetrc.top;
				}
			}
		}

		if(dist != g_nSnapRange)
			prc->top += dist;
	}


	if(edge == WMSZ_BOTTOM || edge == WMSZ_BOTTOMLEFT || edge == WMSZ_BOTTOMRIGHT)
	{
		dist = g_nSnapRange;
		for(itr = TopEdge.begin(); itr != TopEdge.end(); ++itr)
		{
			if((*itr).start.x < (prc->right + g_nSnapRange) && (prc->left - g_nSnapRange) < ((*itr).start.x + (*itr).len))
			{
				if(Abs((*itr).start.y - prc->bottom + g_Offsetrc.bottom) < Abs(dist))
				{
					dist = (*itr).start.y - prc->bottom + g_Offsetrc.bottom;
				}
			}
		}

		for(itr = BottomEdge.begin(); itr != BottomEdge.end(); ++itr)
		{
			if((*itr).start.x < (prc->right + g_nSnapRange) && (prc->left - g_nSnapRange) < ((*itr).start.x + (*itr).len))
			{
				if(Abs((*itr).start.y - prc->bottom) < Abs(dist))
				{
					dist = (*itr).start.y - prc->bottom;
				}
			}
		}

		if(dist != g_nSnapRange)
			prc->bottom += dist;
	}

	if(edge == WMSZ_LEFT || edge == WMSZ_TOPLEFT || edge == WMSZ_BOTTOMLEFT)
	{
		dist = g_nSnapRange;
		for(itr = LeftEdge.begin(); itr != LeftEdge.end(); ++itr)
		{
			if((*itr).start.y < (prc->bottom + g_nSnapRange) && (prc->top - g_nSnapRange) < ((*itr).start.y + (*itr).len))
			{
				if(Abs((*itr).start.x - prc->left) < Abs(dist))
				{
					dist = (*itr).start.x - prc->left;
				}
			}
		}

		for(itr = RightEdge.begin(); itr != RightEdge.end(); ++itr)
		{
			if((*itr).start.y < (prc->bottom + g_nSnapRange) && (prc->top - g_nSnapRange) < ((*itr).start.y + (*itr).len))
			{
				if(Abs((*itr).start.x - prc->left - g_Offsetrc.left) < Abs(dist))
				{
					dist = (*itr).start.x - prc->left - g_Offsetrc.left;
				}
			}
		}

		if(dist != g_nSnapRange)
			prc->left += dist;
	}

	if(edge == WMSZ_RIGHT || edge == WMSZ_TOPRIGHT || edge == WMSZ_BOTTOMRIGHT)
	{
		dist = g_nSnapRange;
		for(itr = LeftEdge.begin(); itr != LeftEdge.end(); ++itr)
		{
			if((*itr).start.y < (prc->bottom + g_nSnapRange) && (prc->top - g_nSnapRange) < ((*itr).start.y + (*itr).len))
			{
				if(Abs((*itr).start.x - prc->right + g_Offsetrc.right) < Abs(dist))
				{
					dist = (*itr).start.x - prc->right + g_Offsetrc.right;
				}
			}
		}

		for(itr = RightEdge.begin(); itr != RightEdge.end(); ++itr)
		{
			if((*itr).start.y < (prc->bottom + g_nSnapRange) && (prc->top - g_nSnapRange) < ((*itr).start.y + (*itr).len))
			{
				if(Abs((*itr).start.x - prc->right) < Abs(dist))
				{
					dist = (*itr).start.x - prc->right;
				}
			}
		}

		if(dist != g_nSnapRange)
			prc->right += dist;
	}

	if(g_hMDIwnd)
		ClientToScreenRect(g_hMDIwnd, prc);
}

void OnWindowPosChanging(HWND hWnd, WINDOWPOS* pwp)
{
	std::vector<LINE>::iterator itr;
	int dist = g_nSnapRange;
	for(itr = TopEdge.begin(); itr != TopEdge.end(); ++itr)
	{
		if((*itr).start.x < (pwp->x + pwp->cx + g_nSnapRange) && (pwp->x - g_nSnapRange) < ((*itr).start.x + (*itr).len))
		{
			if(Abs((*itr).start.y - pwp->y) < Abs(dist))
			{
				dist = (*itr).start.y - pwp->y;
			}
			if(Abs((*itr).start.y - (pwp->y + pwp->cy) + g_Offsetrc.bottom) < Abs(dist))
			{
				dist = (*itr).start.y - (pwp->y + pwp->cy) + g_Offsetrc.bottom;
			}
		}
	}

	for(itr = BottomEdge.begin(); itr != BottomEdge.end(); ++itr)
	{
		if((*itr).start.x < (pwp->x + pwp->cx + g_nSnapRange) && (pwp->x - g_nSnapRange) < ((*itr).start.x + (*itr).len))
		{
			if(Abs((*itr).start.y - pwp->y - g_Offsetrc.top) < Abs(dist))
			{
				dist = (*itr).start.y - pwp->y - g_Offsetrc.top;
			}
			if(Abs((*itr).start.y - (pwp->y + pwp->cy)) < Abs(dist))
			{
				dist = (*itr).start.y - (pwp->y + pwp->cy);
			}
		}
	}

	if(dist != g_nSnapRange)
		pwp->y += dist;

	dist = g_nSnapRange;

	for(itr = LeftEdge.begin(); itr != LeftEdge.end(); ++itr)
	{
		if((*itr).start.y < (pwp->y + pwp->cy + g_nSnapRange) && (pwp->y - g_nSnapRange) < ((*itr).start.y + (*itr).len))
		{
			if(Abs((*itr).start.x - pwp->x) < Abs(dist))
			{
				dist = (*itr).start.x - pwp->x;
			}
			if(Abs((*itr).start.x - (pwp->x + pwp->cx) + g_Offsetrc.right) < Abs(dist))
			{
				dist = (*itr).start.x - (pwp->x + pwp->cx) + g_Offsetrc.right;
			}
		}
	}

	for(itr = RightEdge.begin(); itr != RightEdge.end(); ++itr)
	{
		if((*itr).start.y < (pwp->y + pwp->cy + g_nSnapRange) && (pwp->y - g_nSnapRange) < ((*itr).start.y + (*itr).len))
		{
			if(Abs((*itr).start.x - pwp->x - g_Offsetrc.left) < Abs(dist))
			{
				dist = (*itr).start.x - pwp->x - g_Offsetrc.left;
			}
			if(Abs((*itr).start.x - (pwp->x + pwp->cx)) < Abs(dist))
			{
				dist = (*itr).start.x - (pwp->x + pwp->cx);
			}
		}
	}

	if(dist != g_nSnapRange)
		pwp->x += dist;


	//デスクトップからはみ出さないように
	if(g_bDesktopOut)
	{
		RECT rc;
		if(g_hMDIwnd == NULL)
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
		else
			GetClientRect(GetParent(hWnd), &rc);

		if(pwp->y < rc.top)
			pwp->y = rc.top;
		if(rc.bottom < pwp->y + pwp->cy)
			pwp->y = rc.bottom - pwp->cy;
		if(pwp->x < rc.left)
			pwp->x = rc.left;
		if(rc.right < pwp->x + pwp->cx)
			pwp->x = rc.right - pwp->cx;
	}
}

// サブクラス化したミニパネルプロシージャ
LRESULT CALLBACK MiniPanelProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	static BOOL bSizing = FALSE;

	switch(msg){
		case WM_ENTERSIZEMOVE:
			g_MovinWnd = hWnd;
			GetAllWindowEdge(hWnd);
			break;
		case WM_MOVING:
			bSizing = FALSE;
			break;
		case WM_SIZING:
			bSizing = TRUE;
 			if(GetKeyState(VK_SHIFT) < 0)
				break;
			OnSizing(hWnd, wp, (LPRECT)lp);
			break;
		case WM_WINDOWPOSCHANGING:
 			if(bSizing || GetKeyState(VK_SHIFT) < 0)
				break;

			OnWindowPosChanging(hWnd, (WINDOWPOS*)lp);
			break;

		case WM_EXITSIZEMOVE:
			//Subclassify(hWnd, g_DefWndProc);

			if(!TopEdge.empty())
				TopEdge.clear();
			if(!BottomEdge.empty())
				BottomEdge.clear();
			if(!LeftEdge.empty())
				LeftEdge.clear();
			if(!RightEdge.empty())
				RightEdge.clear();

			break;

		case WM_COMMAND:
			if(LOWORD(wp)==IDCANCEL){
				// サブクラス化の解除
				SetWindowLong(hWnd, GWL_WNDPROC, (LONG)hOldMiniPanelProc);
			}
			break;
	}
	return CallWindowProc(hOldMiniPanelProc, hWnd, msg, wp, lp);
}

// サブクラス化したメインウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp){
	switch(msg){
		case WM_COMMAND:
			if(LOWORD(wp)==IDM_MINIPANEL){
				LRESULT lRet = CallWindowProc(hOldProc, hWnd, msg, wp, lp);

				HWND hFound = NULL;
				// ミニパネルのハンドルを取得し、サブクラス化
				hFound = FindWindowEx(NULL, hFound, "#32770", NULL);
				while(hFound){
					if(fpi.hParent==GetWindow(hFound, GW_OWNER)){
						hOldMiniPanelProc = (WNDPROC)SetWindowLong(hFound, GWL_WNDPROC, (LONG)MiniPanelProc);
						break;
					}
					hFound = FindWindowEx(NULL, hFound, "#32770", NULL);
				}
				return lRet;
			}
			break;
	}
	return CallWindowProc(hOldProc, hWnd, msg, wp, lp);
}

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE /*hModule*/, DWORD /*dwFunction*/, LPVOID /*lpNot*/){
    return TRUE;
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	char iniPath[MAX_PATH];
	int len = GetModuleFileName(fpi.hDllInst, iniPath, MAX_PATH);
	iniPath[len - 3] = 'i';
	iniPath[len - 2] = 'n';
	iniPath[len - 1] = 'i';

	g_Offsetrc.left		= GetPrivateProfileInt("Setting", "OffsetLeft", 0, iniPath);
	g_Offsetrc.right	= GetPrivateProfileInt("Setting", "OffsetRight", 0, iniPath);
	g_Offsetrc.top		= GetPrivateProfileInt("Setting", "OffsetTop", 0, iniPath);
	g_Offsetrc.bottom	= GetPrivateProfileInt("Setting", "OffsetBottom", 0, iniPath);
	g_nSnapRange		= GetPrivateProfileInt("Setting", "SnapRange", 15, iniPath);
	g_bSnapHide			= GetPrivateProfileInt("Setting", "SnapHide", 0, iniPath);
	g_bDesktopOut		= GetPrivateProfileInt("Setting", "DesktopOut", 1, iniPath);


	hOldProc = (WNDPROC)SetWindowLong(fpi.hParent, GWL_WNDPROC, (LONG)WndProc);
	return (BOOL)hOldProc;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	return;
}

/* 再生状態が変わる時に呼ばれます */
void OnStatusChange(){
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
/*
 * Fund.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _FUNC_H_
#define _FUNC_H_

#include "fittle.h"

enum {OTHERS=0, FOLDERS, FILES, LISTS, ARCHIVES, NA_CUESHEETS_, URLS};

//--É}ÉNÉç--
#define FILE_EXIST(X) (GetFileAttributes(X)==0xFFFFFFFF ? FALSE : TRUE)
#define IsURLPath(X) StrStr(X, TEXT("://"))
#define IsURLPathA(X) StrStrA(X, "://")

LPTSTR GetFileName(LPTSTR);
int GetParentDir(LPCTSTR, LPTSTR);
BOOL IsPlayList(LPTSTR);
BOOL IsPlayListFast(LPTSTR);
void FormatDateTime(LPTSTR pszBuf, LPFILETIME pft);
void FormatLocalDateTime(LPTSTR, LPFILETIME);
BOOL GetTimeAndSize(LPCTSTR, LPTSTR, LPTSTR);
void SetOLECursor(int);
LPTSTR MyPathAddBackslash(LPTSTR);
void GetFolderPart(LPTSTR);
void SubClassControl(HWND hWnd, WNDPROC Proc);
LRESULT SubClassCallNext(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
int GetMenuPosFromString(HMENU hMenu, LPTSTR lpszText);
LONG GetToolbarTrueWidth(HWND hToolbar);
void UpdateWindowSize(HWND);
int GetDropFiles(HDROP hDrop, struct FILEINFO **ppSub, LPPOINT ppt, LPTSTR szPath);
void ShowSettingDialog(HWND, int);
int SaveM3UDialog(LPTSTR, LPTSTR);
HMODULE ExpandArgs(int *pARGC, LPTSTR **pARGV);

void LV_SetState(HWND hLV, int nItem, int nState);
void LV_ClearSelect(HWND hLV);
void LV_ClearHilite(HWND hLV);
void LV_SingleSelect(HWND hLV, int nIndex);
void LV_SingleSelectView(HWND hLV, int nIndex);
void LV_SingleSelectViewP(HWND hLV, int nIndex);
int LV_GetNextSelect(HWND hLV, int nIndex);
int LV_GetCount(HWND hLV);
int LV_HitTest(HWND hLV, LONG lPos);

void lstrcpyntA(LPSTR lpDst, LPCTSTR lpSrc, int nDstMax);
void lstrcpyntW(LPWSTR lpDst, LPCTSTR lpSrc, int nDstMax);
void lstrcpynAt(LPTSTR lpDst, LPCSTR lpSrc, int nDstMax);
void lstrcpynWt(LPTSTR lpDst, LPCWSTR lpSrc, int nDstMax);
void ClearTypelist();
LPTSTR GetTypelist(int nIndex);
void AddTypes(LPCSTR lpszTypes);
void SendSupportList(HWND hWnd);

float ChPosToSec(DWORD hCh, QWORD qPos);
float TrackPosToSec(QWORD qPos);
QWORD TrackGetPos();
BOOL TrackSetPos(QWORD qPos);

LPVOID HAlloc(DWORD dwSize);
LPVOID HZAlloc(DWORD dwSize);
LPVOID HRealloc(LPVOID pPtr, DWORD dwSize);
void HFree(LPVOID pPtr);

#endif

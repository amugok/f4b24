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
int WritePrivateProfileInt(LPTSTR, LPTSTR, int, LPTSTR);
BOOL GetTimeAndSize(LPCTSTR, LPTSTR, LPTSTR);
void SetOLECursor(int);
void GetModuleParentDir(LPTSTR);
LPTSTR MyPathAddBackslash(LPTSTR);

LPVOID HAlloc(DWORD dwSize);
LPVOID HZAlloc(DWORD dwSize);
LPVOID HRealloc(LPVOID pPtr, DWORD dwSize);
void HFree(LPVOID pPtr);

#endif

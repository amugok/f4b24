/*
 * Fund.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _FUNC_H_
#define _FUNC_H_

#include "fittle.h"

enum {OTHERS=0, FOLDERS, FILES, LISTS, ARCHIVES, CUESHEETS, URLS};

//--É}ÉNÉç--
#define FILE_EXIST(X) (GetFileAttributes(X)==0xFFFFFFFF ? FALSE : TRUE)
#define IsURLPath(X) StrStr(X, "://")

char *GetFileName(char *);
int GetParentDir(char *, char *);
BOOL IsPlayList(char *);
int WritePrivateProfileInt(char *, char *, int, char *);
BOOL GetTimeAndSize(const char *, char *, char *);
void SetOLECursor(int);
void GetModuleParentDir(char *);
LPSTR MyPathAddBackslash(LPSTR);
#endif
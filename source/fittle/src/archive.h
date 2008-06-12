/*
 * Archive.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

#include "fittle.h"

BOOL InitArchive(char *pszPath, HWND hWnd);
BOOL ReadArchive(struct FILEINFO **, char *);
BOOL AnalyzeArchivePath(CHANNELINFO *, char *, char *, char *);
BOOL IsArchive(char *);
BOOL IsArchivePath(char *);
BOOL GetArchiveTagInfo(LPSTR, TAGINFO *);
HICON GetArchiveItemIcon(char *);
int GetArchiveIconIndex(char *);
BOOL GetArchiveItemType(char *, char *, int);
char *GetArchiveItemFileName(char *);

#endif

/*
 * Archive.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

#include "fittle.h"

BOOL InitArchive(LPTSTR pszPath, HWND hWnd);
BOOL ReadArchive(struct FILEINFO **, LPTSTR);
BOOL AnalyzeArchivePath(CHANNELINFO *, LPTSTR, LPTSTR, LPTSTR);
BOOL IsArchive(LPTSTR);
BOOL IsArchivePath(LPTSTR);
BOOL GetArchiveTagInfo(LPTSTR, TAGINFO *);
HICON GetArchiveItemIcon(LPTSTR);
int GetArchiveIconIndex(LPTSTR);
BOOL GetArchiveItemType(LPTSTR, LPTSTR, int);
LPTSTR GetArchiveItemFileName(LPTSTR);
BOOL GetArchiveGain(LPTSTR pszPath, float *pGain, DWORD hBass);

#endif

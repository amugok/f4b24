/*
 * CueSheet.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _CUESHEET_H_
#define _CUESHEET_H_

#include "fittle.h"
#include "bass_tag.h"

BOOL ReadCueSheet(struct FILEINFO **, char *);
BOOL IsCueSheet(char *);
int IsCueSheetPath(char *);
QWORD GetByteFromSecStr(HCHANNEL, char *);
BOOL GetCueSheetRealPath(LPCSTR, LPSTR, LPSTR, LPSTR);
BOOL GetCueSheetTagInfo(LPCSTR, TAGINFO *);

#endif

/*
 * List.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _LIST_H_
#define _LIST_H_

#include "fittle.h"
#include <commctrl.h>

struct FILEINFO **AddList(struct FILEINFO **, char *, char *, char *);
int DeleteAList(struct FILEINFO *, struct FILEINFO **);
int DeleteAllList(struct FILEINFO **);
int GetListCount(struct FILEINFO *);
struct FILEINFO *GetPtrFromIndex(struct FILEINFO *, int);
int GetIndexFromPtr(struct FILEINFO *, struct FILEINFO *);
int GetIndexFromPath(struct FILEINFO *,char *);
int SetUnPlayed(struct FILEINFO *);
int GetUnPlayedFile(struct FILEINFO *);
int GetUnPlayedIndex(struct FILEINFO *, int);
BOOL SearchFolder(struct FILEINFO **, char *, BOOL);
int ReadM3UFile(struct FILEINFO **, char *);
BOOL WriteM3UFile(struct FILEINFO *, char *, int);
BOOL ReadArchive(struct FILEINFO **, char *);
void MergeSort(struct FILEINFO **, int);
int SearchFiles(struct FILEINFO **, char *, BOOL);
int LinkCheck(struct FILEINFO **);

#endif
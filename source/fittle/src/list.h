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

struct FILEINFO **AddList(struct FILEINFO **, LPTSTR, LPTSTR, LPTSTR);
int DeleteAList(struct FILEINFO *, struct FILEINFO **);
int DeleteAllList(struct FILEINFO **);
int GetListCount(struct FILEINFO *);
struct FILEINFO *GetPtrFromIndex(struct FILEINFO *, int);
int GetIndexFromPtr(struct FILEINFO *, struct FILEINFO *);
int GetIndexFromPath(struct FILEINFO *,LPTSTR);
int SetUnPlayed(struct FILEINFO *);
int GetUnPlayedFile(struct FILEINFO *);
int GetUnPlayedIndex(struct FILEINFO *, int);
BOOL SearchFolder(struct FILEINFO **, LPTSTR, BOOL);
int ReadM3UFile(struct FILEINFO **, LPTSTR);
BOOL WriteM3UFile(struct FILEINFO *, LPTSTR, int);
BOOL ReadArchive(struct FILEINFO **, LPTSTR);
void MergeSort(struct FILEINFO **, int);
int SearchFiles(struct FILEINFO **, LPTSTR, BOOL);
int LinkCheck(struct FILEINFO **);

int GetColumnNum();
int GetColumnType(int nColumn);
void GetColumnText(struct FILEINFO *pTmp, int nRow, int nColumn, LPTSTR pWork, int nWorkMax);
void AddColumns(HWND hList);
void LoadColumnsOrder();
void SaveColumnsOrder(HWND hList);
LPVOID GetLXIf();

#endif
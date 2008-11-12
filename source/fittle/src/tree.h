/*
 * Tree.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _TREE_H_
#define _TREE_H_

#include <windows.h>
#include <commctrl.h>

int SetDrivesToCombo(HWND);
int InitTreeIconIndex(HWND, HWND, BOOL);
HTREEITEM MakeDriveNode(HWND, HWND);
HTREEITEM MakeTwoTree(HWND, HTREEITEM);
HTREEITEM MakeTreeFromPath(HWND, HWND, LPCTSTR);
int GetPathFromNode(HWND, HTREEITEM, LPTSTR);
int MyScroll(HWND);
void OnBeginDragTree(HWND);
LRESULT CALLBACK NewComboProc(HWND, UINT, WPARAM, LPARAM);
void RefreshComboIcon(HWND);

#endif
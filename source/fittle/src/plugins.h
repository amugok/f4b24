/*
 * Plugins.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include <windows.h>

void InitPlugins(HWND);
void QuitPlugins();
void OnStatusChangePlugins();
void OnTrackChagePlugins();
BOOL EnumPlugins(HMODULE hParent, LPCTSTR lpszSubDir, LPCTSTR lpszMask, BOOL (CALLBACK * lpfnPluginProc)(HMODULE hPlugin, HWND hWnd), HWND hWnd);
BOOL EnumFiles(HMODULE hParent, LPCTSTR lpszSubDir, LPCTSTR lpszMask, BOOL (CALLBACK * lpfnFileProc)(LPCTSTR lpszPath, HWND hWnd), HWND hWnd);


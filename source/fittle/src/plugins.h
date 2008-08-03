/*
 * Plugins.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include <windows.h>

void InitGeneralPlugins(HWND);
void InitFittlePlugins(HWND);
void QuitPlugins();
void OnStatusChangePlugins();
void OnTrackChagePlugins();
void OnConfigChangePlugins();
BOOL OnWndProcPlugins(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *lplresult);
BOOL EnumPlugins(HMODULE hParent, LPCTSTR lpszSubDir, LPCTSTR lpszMask, BOOL (CALLBACK * lpfnPluginProc)(HMODULE hPlugin, HWND hWnd), HWND hWnd);
BOOL EnumFiles(HMODULE hParent, LPCTSTR lpszSubDir, LPCTSTR lpszMask, BOOL (CALLBACK * lpfnFileProc)(LPCTSTR lpszPath, HWND hWnd), HWND hWnd);


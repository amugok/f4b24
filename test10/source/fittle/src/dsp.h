/*
 * dsp.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _DSP_H_
#define _DSP_H_

#include "fittle.h"

typedef DWORD HWADSP;

typedef LRESULT (CALLBACK WINAMPWINPROC)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


typedef void (WINAPI *LPBASS_WADSP_Init)(HWND hwndMain);
typedef HWADSP (WINAPI *LPBASS_WADSP_Load)(const char *dspfile, int x, int y, int width, int height, WINAMPWINPROC *proc);
typedef void (WINAPI *LPBASS_WADSP_Start)(HWADSP plugin, DWORD module, DWORD hchan);
typedef HDSP (WINAPI *LPBASS_WADSP_ChannelSetDSP)(HWADSP plugin, DWORD hchan, int priority);
typedef void (WINAPI *LPBASS_WADSP_Stop)(HWADSP plugin);
typedef void (WINAPI *LPBASS_WADSP_Free)(void);

BOOL InitBassWaDsp(HWND hWnd);
BOOL SetBassWaDsp(HCHANNEL hChan);
void FreeBassWaDsp();

#endif
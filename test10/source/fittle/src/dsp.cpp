/*
 * dsp.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "dsp.h"

static LPBASS_WADSP_Init BASS_WADSP_Init = NULL;
static LPBASS_WADSP_Load BASS_WADSP_Load = NULL;
static LPBASS_WADSP_Start BASS_WADSP_Start = NULL;
static LPBASS_WADSP_ChannelSetDSP BASS_WADSP_ChannelSetDSP = NULL;
static LPBASS_WADSP_Stop BASS_WADSP_Stop = NULL;
static LPBASS_WADSP_Free BASS_WADSP_Free = NULL;

HWADSP wadsp[10] = {0};
int nCount = 0;

BOOL InitBassWaDsp(HWND hWnd){
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	HMODULE hWaDsp = LoadLibrary("bass_wadsp.dll");
	if(!hWaDsp){
		return FALSE;
	}

	BASS_WADSP_Init = (LPBASS_WADSP_Init)GetProcAddress(hWaDsp,"BASS_WADSP_Init");
	BASS_WADSP_Load = (LPBASS_WADSP_Load)GetProcAddress(hWaDsp,"BASS_WADSP_Load");
	BASS_WADSP_Start = (LPBASS_WADSP_Start)GetProcAddress(hWaDsp, "BASS_WADSP_Start");
	BASS_WADSP_ChannelSetDSP = (LPBASS_WADSP_ChannelSetDSP)GetProcAddress(hWaDsp, "BASS_WADSP_ChannelSetDSP");
	BASS_WADSP_Stop = (LPBASS_WADSP_Stop)GetProcAddress(hWaDsp, "BASS_WADSP_Stop");
	BASS_WADSP_Free = (LPBASS_WADSP_Free)GetProcAddress(hWaDsp,"BASS_WADSP_Free");

	BASS_WADSP_Init(hWnd);
	
	int i = 0;

	char szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	lstrcpyn(PathFindFileName(szPath), _T("dsp_*.dll"), MAX_PATH);

	hFind = FindFirstFile(szPath, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			wadsp[i] = BASS_WADSP_Load(wfd.cFileName, 0, 0, 5, 5, NULL);
			if(wadsp[i]){
				BASS_WADSP_Start(wadsp[i], 0, 0);
				i++;
				if(i==10) break;
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	nCount = i;
	return TRUE;
}

BOOL SetBassWaDsp(HCHANNEL hChan){
	if(!BASS_WADSP_ChannelSetDSP) return FALSE;
	for(int i=0;i<nCount;i++){
		BASS_WADSP_ChannelSetDSP(wadsp[i], hChan, 0);
	}
	return TRUE;
}		

void FreeBassWaDsp(){
	if(BASS_WADSP_Stop){
		for(int i=0;i<nCount;i++){
			BASS_WADSP_Stop(wadsp[i]);
		}
	}
	if(BASS_WADSP_Free) BASS_WADSP_Free();
}